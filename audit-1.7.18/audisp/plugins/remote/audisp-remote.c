/* audisp-remote.c --
 * Copyright 2008,2009 Red Hat Inc., Durham, North Carolina.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors:
 *   Steve Grubb <sgrubb@redhat.com>
 *
 */

#include "config.h"
#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#ifdef USE_GSSAPI
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_generic.h>
#include <krb5.h>
#endif
#include "libaudit.h"
#include "private.h"
#include "remote-config.h"
#include "queue.h"

#define CONFIG_FILE "/etc/audisp/audisp-remote.conf"
#define BUF_SIZE 32

/* Error types */
#define ET_SUCCESS	 0
#define ET_PERMANENT	-1
#define ET_TEMPORARY	-2

/* Global Data */
static volatile int stop = 0;
static volatile int hup = 0;
static volatile int suspend = 0;
static volatile int transport_ok = 0;
static volatile int sock=-1;
static volatile int remote_ended = 0, quiet = 0;
static int ifd;
static FILE *in;

static const char *SINGLE = "1";
static const char *HALT = "0";
static remote_conf_t config;

/* Local function declarations */
static int check_message(void);
static int relay_event(const char *s, size_t len);
static int init_transport(void);
static int stop_transport(void);

static int ar_read (int, void *, int);
static int ar_write (int, const void *, int);

#ifdef USE_GSSAPI
/* We only ever talk to one server, so we don't need per-connection
   credentials.  These are the ones we talk to the server with.  */
gss_ctx_id_t my_context;

#define REQ_FLAGS GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG | GSS_C_INTEG_FLAG | GSS_C_CONF_FLAG
#define USE_GSS (config.enable_krb5)
#endif

/*
 * SIGTERM handler
 */
static void term_handler( int sig )
{
        stop = 1;
}

/*
 * SIGHUP handler: re-read config
 */
static void hup_handler( int sig )
{
        hup = 1;
}

static void reload_config(void)
{
	stop_transport(); // FIXME: We should only stop transport if necessary
	hup = 0;
}

/*
 * SIGSUR2 handler: resume logging
 */
static void user2_handler( int sig )
{
        suspend = 0;
}

/*
 * SIGCHLD handler: reap exiting processes
 */
static void child_handler(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		; /* empty */
}
/*
 * Handlers for various events coming back from the remote server.
 * Return -1 if the remote dispatcher should exit.
 */

/* Loss of sync - got an invalid response.  */
static int sync_error_handler (const char *why)
{
	/* "why" has human-readable details on why we've lost (or will
	   be losing) sync.  Sync errors are transient - if a retry
	   doesn't fix it, we eventually call network_failure_handler
	   which has all the user-tweakable actions.  */
	if (config.network_failure_action == FA_SYSLOG)
		syslog (LOG_ERR, "lost/losing sync, %s", why);
	return 0;
}

static void change_runlevel(const char *level)
{
	char *argv[3];
	int pid;
	static const char *init_pgm = "/sbin/init";

	pid = fork();
	if (pid < 0) {
		syslog(LOG_ALERT, 
		       "audisp-remote failed to fork switching runlevels");
		return;
	}
	if (pid)	/* Parent */
		return;

	/* Child */
	argv[0] = (char *)init_pgm;
	argv[1] = (char *)level;
	argv[2] = NULL;
	execve(init_pgm, argv, NULL);
	syslog(LOG_ALERT, "audisp-remote failed to exec %s", init_pgm);
	exit(1);
}

static void safe_exec(const char *exe, const char *message)
{
	char *argv[3];
	int pid;

	pid = fork();
	if (pid < 0) {
		syslog(LOG_ALERT,
			"audisp-remote failed to fork doing safe_exec");
		return;
	}
	if (pid)	/* Parent */
		return;

	/* Child */
	argv[0] = (char *)exe;
	argv[1] = (char *)message;
	argv[2] = NULL;
	execve(exe, argv, NULL);
	syslog(LOG_ALERT, "audisp-remote failed to exec %s", exe);
	exit(1);
}

static int do_action (const char *desc, const char *message,
		       int log_level,
		       failure_action_t action, const char *exe)
{
	switch (action)
	{
	case FA_IGNORE:
		return 0;
	case FA_SYSLOG:
		syslog (log_level, "%s, %s", desc, message);
		return 0;
	case FA_EXEC:
		safe_exec (exe, message);
		return 0;
	case FA_SUSPEND:
		syslog (log_level,
			"suspending remote logging due to %s", desc);
		suspend = 1;
		return 0;
	case FA_RECONNECT:
		syslog (log_level,
	"remote logging disconnected due to %s, will attempt reconnection",
			desc);
		return 0;
	case FA_SINGLE:
		syslog (log_level,
	"remote logging is switching system to single user mode due to %s",
			desc);
		change_runlevel(SINGLE);
		return -1;
	case FA_HALT:
		syslog (log_level,
			"remote logging halting system due to %s", desc);
		change_runlevel(HALT);
		return -1;
	case FA_STOP:
		syslog (log_level, "remote logging stopping due to %s, %s",
			desc, message);
		stop = 1;
		return -1;
	}
	syslog (log_level, "unhandled action %d for %s", action, desc);
	return -1;
}

static int network_failure_handler (const char *message)
{
	return do_action ("network failure", message,
			  LOG_WARNING,
			  config.network_failure_action,
			  config.network_failure_exe);
}

static int remote_disk_low_handler (const char *message)
{
	return do_action ("remote disk low", message,
			  LOG_WARNING,
			  config.disk_low_action, config.disk_low_exe);
}

static int remote_disk_full_handler (const char *message)
{
	return do_action ("remote disk full", message,
			  LOG_ERR,
			  config.disk_full_action, config.disk_full_exe);
}

static int remote_disk_error_handler (const char *message)
{
	return do_action ("remote disk error", message,
			  LOG_ERR,
			  config.disk_error_action, config.disk_error_exe);
}

static int remote_server_ending_handler (const char *message)
{
	stop_transport();
	remote_ended = 1;
	return do_action ("remote server ending", message,
			  LOG_NOTICE,
			  config.remote_ending_action,
			  config.remote_ending_exe);
}

static int generic_remote_error_handler (const char *message)
{
	return do_action ("unrecognized remote error", message,
			  LOG_ERR, config.generic_error_action,
			  config.generic_error_exe);
}

static int generic_remote_warning_handler (const char *message)
{
	return do_action ("unrecognized remote warning", message,
			  LOG_WARNING,
			  config.generic_warning_action,
			  config.generic_warning_exe);
}

static void send_heartbeat (void)
{
	relay_event (NULL, 0);
}

int main(int argc, char *argv[])
{
	event_t *e;
	struct sigaction sa;
	int rc, q_len;

	/* Register sighandlers */
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	/* Set handler for the ones we care about */
	sa.sa_handler = term_handler;
	sigaction(SIGTERM, &sa, NULL);
	sa.sa_handler = hup_handler;
	sigaction(SIGHUP, &sa, NULL);
	sa.sa_handler = user2_handler;
	sigaction(SIGUSR2, &sa, NULL);
	sa.sa_handler = child_handler;
	sigaction(SIGCHLD, &sa, NULL);
	if (load_config(&config, CONFIG_FILE))
		return 6;

	// ifd = open("test.log", O_RDONLY);
	// in = fdopen(ifd, "r");
	ifd = 0;
	in = stdin;

	/* We fail here if the transport can't be initialized because of some
	 * permenent (i.e. operator) problem, such as misspelled host name. */
	rc = init_transport();
	if (rc == ET_PERMANENT)
		return 1;
	init_queue(config.queue_depth);

	do {
		fd_set rfd;
		struct timeval tv;
		int n, fds = ifd + 1;

		/* Load configuration */
		if (hup) 
			reload_config();

		FD_ZERO(&rfd);
		FD_SET(ifd, &rfd);	// input fd
		if (sock > 0) {
			FD_SET(sock, &rfd); // remote socket
			if (sock > ifd)
				fds = sock + 1;
		}
		if (config.heartbeat_timeout > 0) {
			tv.tv_sec = config.heartbeat_timeout;
			tv.tv_usec = 0;
			n = select(fds, &rfd, NULL, NULL, &tv);
		} else
			n = select(fds, &rfd, NULL, NULL, NULL);
		if (n < 0)
			// If we are here, we had some kind of problem
			continue;

		if ((config.heartbeat_timeout > 0) && n == 0 && !remote_ended) {
			/* We attempt a hearbeat if select fails, which
			 * may give us more heartbeats than we need. This
			 * is safer than too few heartbeats.  */
			quiet = 1;
			send_heartbeat();
			quiet = 0;
			continue;
		}

		// See if we got a shutdown message from the server
		if (sock > 0 && FD_ISSET(sock, &rfd)) {
			check_message();
		}

		// See if input fd is also set, otherwise we are done
		if (!FD_ISSET(ifd, &rfd))
			continue;

		e = (event_t *)malloc(sizeof(event_t));
		if (fgets_unlocked(e->data, MAX_AUDIT_MESSAGE_LENGTH, in) &&
							hup==0 && stop==0) {
			if (!transport_ok && remote_ended && 
				config.remote_ending_action == FA_RECONNECT) {
				quiet = 1;
				if (init_transport() == ET_SUCCESS)
					remote_ended = 0;
				quiet = 0;
			}
			/* Strip out EOE records */
			if (strstr(e->data,"type=EOE msg=audit(")) {
				free(e);
				continue;
			}
			enqueue(e);
			rc = 0;
			while (!suspend && rc >= 0 && transport_ok &&
							(e = dequeue(1))) {
				rc = relay_event(e->data, 
					strnlen(e->data,
					MAX_AUDIT_MESSAGE_LENGTH));
				if (rc >= 0) {
					dequeue(0); // delete it
					free(e);
				}
			}
		}
		if (feof(in))
			break;
	} while (stop == 0);
	close(sock);
	free_config(&config);
	q_len = queue_length();
	destroy_queue();
	if (stop)
		syslog(LOG_NOTICE, "audisp-remote is exiting on stop request");

	return q_len ? 1 : 0;
}

#ifdef USE_GSSAPI

/* Communications under GSS is done by token exchanges. Each "token" may
   contain a message, perhaps signed, perhaps encrypted. The messages within
   are what we're interested in, but the network sees the tokens. The
   protocol we use for transferring tokens is to send the length first,
   four bytes MSB first, then the token data. We return nonzero on error. */
static int recv_token(int s, gss_buffer_t tok)
{
	int ret;
	unsigned char lenbuf[4];
	unsigned int len;

	ret = ar_read(s, (char *) lenbuf, 4);
	if (ret < 0) {
		syslog(LOG_ERR, "GSS-API error reading token length");
		return -1;
	} else if (!ret) {
		return 0;
	} else if (ret != 4) {
		syslog(LOG_ERR, "GSS-API error reading token length");
		return -1;
	}

	len = (   ((uint32_t)(lenbuf[0] & 0xFF) << 24)
		| ((uint32_t)(lenbuf[1] & 0xFF) << 16)
		| ((uint32_t)(lenbuf[2] & 0xFF) << 8)
		|  (uint32_t)(lenbuf[3] & 0xFF));
	if (len > MAX_AUDIT_MESSAGE_LENGTH) {
		syslog(LOG_ERR,
			"GSS-API error: event length exceeds MAX_AUDIT_LENGTH");
		return -1;
	}
	tok->length = len;
	tok->value = (char *) malloc(tok->length ? tok->length : 1);
	if (tok->length && tok->value == NULL) {
		syslog(LOG_ERR, "Out of memory allocating token data %zd %zx",
				tok->length, tok->length);
		return -1;
	}

	ret = ar_read(s, (char *) tok->value, tok->length);
	if (ret < 0) {
		syslog(LOG_ERR, "GSS-API error reading token data");
		free(tok->value);
		return -1;
	} else if (ret != (int) tok->length) {
		syslog(LOG_ERR, "GSS-API error reading token data");
		free(tok->value);
		return -1;
	}

	return 0;
}

/* Same here.  */
int send_token(int s, gss_buffer_t tok)
{
	int ret;
	unsigned char lenbuf[4];
	unsigned int len;

	if (tok->length > 0xffffffffUL)
		return -1;

	len = tok->length;
	lenbuf[0] = (len >> 24) & 0xff;
	lenbuf[1] = (len >> 16) & 0xff;
	lenbuf[2] = (len >> 8) & 0xff;
	lenbuf[3] = len & 0xff;

	ret = ar_write(s, (char *) lenbuf, 4);
	if (ret < 0) {
		syslog(LOG_ERR, "GSS-API error sending token length");
		return -1;
	} else if (ret != 4) {
		syslog(LOG_ERR, "GSS-API error sending token length");
		return -1;
	}

	ret = ar_write(s, tok->value, tok->length);
	if (ret < 0) {
		syslog(LOG_ERR, "GSS-API error sending token data");
		return -1;
	} else if (ret != (int) tok->length) {
		syslog(LOG_ERR, "GSS-API error sending token data");
		return -1;
	}

	return 0;
}

static void gss_failure_2 (const char *msg, int status, int type)
{
	OM_uint32 message_context = 0;
	OM_uint32 min_status = 0;
	gss_buffer_desc status_string;

	do {
		gss_display_status (&min_status,
				    status,
				    type,
				    GSS_C_NO_OID,
				    &message_context,
				    &status_string);

		syslog (LOG_ERR, "GSS error: %s: %s",
			msg, (char *)status_string.value);

		gss_release_buffer(&min_status, &status_string);
	} while (message_context != 0);
}

static void gss_failure (const char *msg, int major_status, int minor_status)
{
	gss_failure_2 (msg, major_status, GSS_C_GSS_CODE);
	if (minor_status)
		gss_failure_2 (msg, minor_status, GSS_C_MECH_CODE);
}

#define KCHECK(x,f) if (x) { \
		syslog (LOG_ERR, "krb5 error: %s in %s\n", krb5_get_error_message (kcontext, x), f); \
		return -1; }

#define KEYTAB_NAME "/etc/audisp/audisp-remote.key"
#define CCACHE_NAME "MEMORY:audisp-remote"

/* Each time we connect to the server, we negotiate a set of credentials and
   a security context. To do this, we need our own credentials first. For
   other Kerberos applications, the user will have called kinit (or otherwise
   authenticated) first, but we don't have that luxury. So, we implement part
   of kinit here. When our tickets expire, the usual close/open/retry logic
   has us calling here again, where we re-init and get new tickets. */
static int negotiate_credentials (void)
{
	gss_buffer_desc empty_token_buf = { 0, (void *) "" };
	gss_buffer_t empty_token = &empty_token_buf;
	gss_buffer_desc send_tok, recv_tok, *token_ptr;
	gss_ctx_id_t *gss_context = &my_context;
	gss_buffer_desc name_buf;
	gss_name_t service_name_e;
	OM_uint32 major_status, minor_status, init_sec_min_stat;
	OM_uint32 ret_flags;

	/* Getting an initial ticket is outside the scope of GSS, so
	   we use Kerberos calls here.  */

	int krberr;
	krb5_context kcontext = NULL;
	char *realm_name;
	krb5_principal audit_princ;
	krb5_ccache ccache = NULL;
	krb5_creds my_creds;
        krb5_get_init_creds_opt options;
	krb5_keytab keytab = NULL;
	const char *krb5_client_name;
	char *slashptr;
	char host_name[255];
	struct stat st;
	const char *key_file;

	token_ptr = GSS_C_NO_BUFFER;
	*gss_context = GSS_C_NO_CONTEXT;

	krberr = krb5_init_context (&kcontext);
	KCHECK (krberr, "krb5_init_context");

	if (config.krb5_key_file)
		key_file = config.krb5_key_file;
	else
		key_file = KEYTAB_NAME;
	unsetenv ("KRB5_KTNAME");
	setenv ("KRB5_KTNAME", key_file, 1);

	if (stat (key_file, &st) == 0) {
		if ((st.st_mode & 07777) != 0400) {
			if (!quiet)
				syslog (LOG_ERR,
			"%s is not mode 0400 (it's %#o) - compromised key?",
					key_file, st.st_mode & 07777);
			return -1;
		}
		if (st.st_uid != 0) {
			if (!quiet)
				syslog (LOG_ERR,
			"%s is not owned by root (it's %d) - compromised key?",
					key_file, st.st_uid);
			return -1;
		}
	}

	/* This looks up the default real (*our* realm) from
	   /etc/krb5.conf (or wherever)  */
	krberr = krb5_get_default_realm (kcontext, &realm_name);
	KCHECK (krberr, "krb5_get_default_realm");

	krb5_client_name = config.krb5_client_name ?
				config.krb5_client_name : "auditd";
	if (gethostname(host_name, sizeof(host_name)) != 0) {
		if (!quiet)
			syslog (LOG_ERR,
			"gethostname: host name longer than %ld characters?",
				sizeof (host_name));
		return -1;
	}

	syslog (LOG_ERR, "kerberos principal: %s/%s@%s\n",
		krb5_client_name, host_name, realm_name);
	/* Encode our own "name" as auditd/remote@EXAMPLE.COM.  */
	krberr = krb5_build_principal (kcontext, &audit_princ,
				       strlen(realm_name), realm_name,
				       krb5_client_name, host_name, NULL);
	KCHECK (krberr, "krb5_build_principal");

	/* Locate our machine's key table, where our private key is
	 * held.  */
	krberr = krb5_kt_resolve (kcontext, key_file, &keytab);
	KCHECK (krberr, "krb5_kt_resolve");

	/* Identify a cache to hold the key in.  The GSS wrappers look
	   up our credentials here.  */
	krberr = krb5_cc_resolve (kcontext, CCACHE_NAME, &ccache);
	KCHECK (krberr, "krb5_cc_resolve");

	setenv("KRB5CCNAME", CCACHE_NAME, 1);

	memset(&my_creds, 0, sizeof(my_creds));
	memset(&options, 0, sizeof(options));
	krb5_get_init_creds_opt_set_address_list(&options, NULL);
	krb5_get_init_creds_opt_set_forwardable(&options, 0);
	krb5_get_init_creds_opt_set_proxiable(&options, 0);
	krb5_get_init_creds_opt_set_tkt_life(&options, 24*60*60);

	/* Load our credentials from the key table.  */
	krberr = krb5_get_init_creds_keytab(kcontext, &my_creds, audit_princ,
					    keytab, 0, NULL,
					    &options);
	KCHECK (krberr, "krb5_get_init_creds_keytab");

	/* Create the cache... */
	krberr = krb5_cc_initialize(kcontext, ccache, audit_princ);
	KCHECK (krberr, "krb5_cc_initialize");

	/* ...and store our credentials in it.  */
	krberr = krb5_cc_store_cred(kcontext, ccache, &my_creds);
	KCHECK (krberr, "krb5_cc_store_cred");

	/* The GSS code now has a set of credentials for this program.
	   I.e.  we know who "we" are.  Now we talk to the server to
	   get its credentials and set up a security context for encryption. */
	if (config.krb5_principal == NULL) {
		const char *name = config.krb5_client_name ?
					config.krb5_client_name : "auditd";
		config.krb5_principal = (char *) malloc (strlen (name) + 1
					+ strlen (config.remote_server) + 1);
		sprintf((char *)config.krb5_principal, "%s@%s",
			name, config.remote_server);
	}
	slashptr = strchr (config.krb5_principal, '/');
	if (slashptr)
		*slashptr = '@';

	name_buf.value = (char *)config.krb5_principal;
	name_buf.length = strlen(name_buf.value) + 1;
	major_status = gss_import_name(&minor_status, &name_buf,
			       (gss_OID) gss_nt_service_name, &service_name_e);
	if (major_status != GSS_S_COMPLETE) {
		gss_failure("importing name", major_status, minor_status);
		return -1;
	}

	/* Someone has to go first.  In this case, it's us.  */
	if (send_token(sock, empty_token) < 0) {
		(void) gss_release_name(&minor_status, &service_name_e);
		return -1;
	}

	/* The server starts this loop with the token we just sent
	   (the empty one).  We start this loop with "no token".  */
	token_ptr = GSS_C_NO_BUFFER;
	*gss_context = GSS_C_NO_CONTEXT;

	do {
		/* Give GSS a chance to digest what we have so far.  */
		major_status = gss_init_sec_context(&init_sec_min_stat,
			GSS_C_NO_CREDENTIAL, gss_context,
			service_name_e, NULL, REQ_FLAGS, 0,
			NULL,			/* no channel bindings */
			token_ptr, NULL,	/* ignore mech type */
			&send_tok, &ret_flags, NULL);	/* ignore time_rec */

		if (token_ptr != GSS_C_NO_BUFFER)
			free(recv_tok.value);

		/* Send the server any tokens requested of us.  */
		if (send_tok.length != 0) {
			if (send_token(sock, &send_tok) < 0) {
				(void) gss_release_buffer(&minor_status,
						&send_tok);
				(void) gss_release_name(&minor_status,
						&service_name_e);
				return -1;
			}
		}
		(void) gss_release_buffer(&minor_status, &send_tok);

		if (major_status != GSS_S_COMPLETE
		    && major_status != GSS_S_CONTINUE_NEEDED) {
			gss_failure("initializing context", major_status,
				    init_sec_min_stat);
			(void) gss_release_name(&minor_status, &service_name_e);
			if (*gss_context != GSS_C_NO_CONTEXT)
				gss_delete_sec_context(&minor_status,
						gss_context, GSS_C_NO_BUFFER);
			return -1;
		}

		/* Now get any tokens the sever sends back.  We use
		   these back at the top of the loop.  */
		if (major_status == GSS_S_CONTINUE_NEEDED) {
			if (recv_token(sock, &recv_tok) < 0) {
				(void) gss_release_name(&minor_status,
							&service_name_e);
				return -1;
			}
			token_ptr = &recv_tok;
		}
	} while (major_status == GSS_S_CONTINUE_NEEDED);

	(void) gss_release_name(&minor_status, &service_name_e);

#if 0
	major_status = gss_inquire_context (&minor_status, &my_context, NULL,
					    &service_name_e, NULL, NULL,
					    NULL, NULL, NULL);
	if (major_status != GSS_S_COMPLETE) {
		gss_failure("inquiring target name", major_status, minor_status);
		return -1;
	}
	major_status = gss_display_name(&minor_status, service_name_e,
					&recv_tok, NULL);
	gss_release_name(&minor_status, &service_name_e);
	if (major_status != GSS_S_COMPLETE) {
		gss_failure("displaying name", major_status, minor_status);
		return -1;
	}
	syslog(LOG_INFO, "GSS-API Connected to: %s",
		  (char *)recv_tok.value);
#endif
	return 0;
}
#endif

static int stop_sock(void)
{
	close(sock);
	sock = -1;
	transport_ok = 0;

	return 0;
}

static int stop_transport(void)
{
	int rc;

	switch (config.transport)
	{
		case T_TCP:
			rc = stop_sock();
			break;
		default:
			rc = -1;
			break;
	}
	return rc;
}

static int init_sock(void)
{
	int rc;
	struct addrinfo *ai;
	struct addrinfo hints;
	char remote[BUF_SIZE];
	int one=1;

	if (sock >= 0) {
		syslog(LOG_NOTICE, "socket already setup");
		transport_ok = 1;
		return ET_SUCCESS;
	}
	memset(&hints, '\0', sizeof(hints));
	hints.ai_flags = AI_ADDRCONFIG|AI_NUMERICSERV;
	hints.ai_socktype = SOCK_STREAM;
	snprintf(remote, BUF_SIZE, "%u", config.port);
	rc=getaddrinfo(config.remote_server, remote, &hints, &ai);
	if (rc) {
		if (!quiet)
			syslog(LOG_ERR,
				"Error looking up remote host: %s - exiting",
				gai_strerror(rc));
		return ET_PERMANENT;
	}
	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sock < 0) {
		if (!quiet)
			syslog(LOG_ERR, "Error creating socket: %s",
			strerror(errno));
		freeaddrinfo(ai);
		return ET_TEMPORARY;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof (int));

	if (config.local_port != 0) {
		struct sockaddr_in address;
		
		memset (&address, 0, sizeof(address));
		address.sin_family = htons(AF_INET);
		address.sin_port = htons(config.local_port);
		address.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(sock, (struct sockaddr *)&address, sizeof(address))) {
			if (!quiet)
				syslog(LOG_ERR,
			       "Cannot bind local socket to port %d",
					config.local_port);
			stop_sock();
			return ET_TEMPORARY;
		}

	}
	if (connect(sock, ai->ai_addr, ai->ai_addrlen)) {
		if (!quiet)
			syslog(LOG_ERR, "Error connecting to %s: %s",
				config.remote_server, strerror(errno));
		freeaddrinfo(ai);
		stop_sock();
		return ET_TEMPORARY;
	}

	freeaddrinfo(ai);
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&one, sizeof (int));

	/* The idea here is to minimize the time between the message
	   and the ACK, assuming that individual messages are
	   infrequent enough that we can ignore the inefficiency of
	   sending the header and message in separate packets.  */
	if (config.format == F_MANAGED)
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
				(char *)&one, sizeof (int));

#ifdef USE_GSSAPI
	if (USE_GSS) {
		if (negotiate_credentials ())
			return ET_PERMANENT;
	}
#endif

	transport_ok = 1;
	syslog(LOG_NOTICE, "Connected to %s", config.remote_server);
	return ET_SUCCESS;
}

static int init_transport(void)
{
	int rc;

	switch (config.transport)
	{
		case T_TCP:
			rc = init_sock();
			break;
		default:
			rc = ET_PERMANENT;
			break;
	}
	return rc;
}

static int ar_write (int sk, const void *buf, int len)
{
	int rc = 0, r;
	while (len > 0) {
		do {
			r = write(sk, buf, len);
		} while (r < 0 && errno == EINTR);
		if (r < 0) {
			if (errno == EPIPE)
				stop_sock();
			return r;
		}
		if (r == 0)
			break;
		rc += r;
		buf = (void *)((char *)buf + r);
		len -= r;
	}
	return rc;
}

static int ar_read (int sk, void *buf, int len)
{
	int rc = 0, r, timeout = config.max_time_per_record * 1000;
	struct pollfd pfd;

	pfd.fd=sk;
	pfd.events=POLLIN | POLLPRI | POLLHUP | POLLERR | POLLNVAL;
	while (len > 0) {
		do {
			// reads can hang if cable is disconnected
			int prc = poll(&pfd, (nfds_t) 1, timeout);
			if (prc <= 0)
				return -1;
			r = read(sk, buf, len);
		} while (r < 0 && errno == EINTR);
		if (r < 0) {
			if (errno == EPIPE)
				stop_sock();
			return r;
		}
		if (r == 0)
			break;
		rc += r;
		buf = (void *)((char *)buf + r);
		len -= r;
	}
	return rc;
}

static int relay_sock_ascii(const char *s, size_t len)
{
	int rc;

	if (len == 0)
		return 0;

	if (!transport_ok) {
		if (init_transport ())
			return -1;
	}

	rc = ar_write(sock, s, len);
	if (rc <= 0) {
		stop = 1;
		syslog(LOG_ERR,"Connection to %s closed unexpectedly - exiting",
		       config.remote_server);
		return -1;
	}

	return 0;
}

#ifdef USE_GSSAPI

/* Sending an encrypted message is pretty simple - wrap the message in
   a token, and send the token.  The server unwraps it to get the
   original message.  */
static int send_msg_gss (unsigned char *header, const char *msg, uint32_t mlen)
{
	OM_uint32 major_status, minor_status;
	gss_buffer_desc utok, etok;
	int rc;

	utok.length = AUDIT_RMW_HEADER_SIZE + mlen;
	utok.value = malloc (utok.length);

	memcpy (utok.value, header, AUDIT_RMW_HEADER_SIZE);
	memcpy (utok.value+AUDIT_RMW_HEADER_SIZE, msg, mlen);

	major_status = gss_wrap (&minor_status,
				 my_context,
				 1,
				 GSS_C_QOP_DEFAULT,
				 &utok,
				 NULL,
				 &etok);
	if (major_status != GSS_S_COMPLETE) {
		gss_failure("encrypting message", major_status, minor_status);
		free (utok.value);
		return -1;
	}
	rc = send_token (sock, &etok);
	free (utok.value);
	(void) gss_release_buffer(&minor_status, &etok);

	return rc ? -1 : 0;
}

/* Likewise here.  */
static int recv_msg_gss (unsigned char *header, char *msg, uint32_t *mlen)
{
	OM_uint32 major_status, minor_status;
	gss_buffer_desc utok, etok;
	int hver, mver, rc;
	uint32_t type, rlen, seq;

	rc = recv_token (sock, &etok);
	if (rc)
		return -1;

	major_status = gss_unwrap (&minor_status, my_context, &etok,
					&utok, NULL, NULL);
	if (major_status != GSS_S_COMPLETE) {
		gss_failure("decrypting message", major_status, minor_status);
		free (utok.value);
		return -1;
	}

	if (utok.length < AUDIT_RMW_HEADER_SIZE) {
		sync_error_handler ("message too short");
		return -1;
	}
	memcpy (header, utok.value, AUDIT_RMW_HEADER_SIZE);

	if (! AUDIT_RMW_IS_MAGIC (header, AUDIT_RMW_HEADER_SIZE)) {
		sync_error_handler ("bad magic number");
		return -1;
	}

	AUDIT_RMW_UNPACK_HEADER (header, hver, mver, type, rlen, seq);

	if (rlen > MAX_AUDIT_MESSAGE_LENGTH) {
		sync_error_handler ("message too long");
		return -1;
	}

	memcpy (msg, utok.value+AUDIT_RMW_HEADER_SIZE, rlen);

	*mlen = rlen;

	return 0;
}
#endif

static int send_msg_tcp (unsigned char *header, const char *msg, uint32_t mlen)
{
	int rc;

	rc = ar_write(sock, header, AUDIT_RMW_HEADER_SIZE);
	if (rc <= 0) {
		if (config.network_failure_action == FA_SYSLOG)
			syslog(LOG_ERR, "connection to %s closed unexpectedly",
			       config.remote_server);
		return 1;
	}

	if (msg != NULL && mlen > 0) {
		rc = ar_write(sock, msg, mlen);
		if (rc <= 0) {
			if (config.network_failure_action == FA_SYSLOG)
				syslog(LOG_ERR,
				       "connection to %s closed unexpectedly",
				       config.remote_server);
			return 1;
		}
	}
	return 0;
}

static int recv_msg_tcp (unsigned char *header, char *msg, uint32_t *mlen)
{
	int hver, mver, rc;
	uint32_t type, rlen, seq;

	rc = ar_read (sock, header, AUDIT_RMW_HEADER_SIZE);
	if (rc < 16) {
		if (config.network_failure_action == FA_SYSLOG)
			syslog(LOG_ERR, "connection to %s closed unexpectedly",
			       config.remote_server);
		return -1;
	}

	if (! AUDIT_RMW_IS_MAGIC (header, AUDIT_RMW_HEADER_SIZE)) {
		/* FIXME: the right thing to do here is close the socket
		 *  and start a new one.  */
		sync_error_handler ("bad magic number");
		return -1;
	}

	AUDIT_RMW_UNPACK_HEADER (header, hver, mver, type, rlen, seq);

	if (rlen > MAX_AUDIT_MESSAGE_LENGTH) {
		sync_error_handler ("message too long");
		return -1;
	}

	if (rlen > 0 && ar_read (sock, msg, rlen) < rlen) {
		sync_error_handler ("ran out of data reading reply");
		return -1;
	}
	return 0;
}

static int check_message_managed(void)
{
	unsigned char header[AUDIT_RMW_HEADER_SIZE];
	int hver, mver;
	uint32_t type, rlen, seq;
	char msg[MAX_AUDIT_MESSAGE_LENGTH+1];

#ifdef USE_GSSAPI
	if (USE_GSS) {
		if (recv_msg_gss (header, msg, &rlen)) {
			stop_transport();
			return -1;
		}
	} else
#endif
	if (recv_msg_tcp(header, msg, &rlen)) {
		stop_transport();
		return -1;
	}

	AUDIT_RMW_UNPACK_HEADER(header, hver, mver, type, rlen, seq);
	msg[rlen] = 0;

	if (type == AUDIT_RMW_TYPE_ENDING)
		return remote_server_ending_handler(msg);
	if (type == AUDIT_RMW_TYPE_DISKLOW)
		return remote_disk_low_handler(msg);
	if (type == AUDIT_RMW_TYPE_DISKFULL)
		return remote_disk_full_handler(msg);
	if (type == AUDIT_RMW_TYPE_DISKERROR)
		return remote_disk_error_handler(msg);
	return -1;
}

/* This is to check for async notification like server is shutting down */
static int check_message(void)
{
	int rc;

	switch (config.format)
	{
		case F_MANAGED:
			rc = check_message_managed();
			break;
/*		case F_ASCII:
			rc = check_message_ascii();
			break; */
		default:
			rc = -1;
			break;
	}

	return rc;
}

static int relay_sock_managed(const char *s, size_t len)
{
	static int sequence_id = 1;
	unsigned char header[AUDIT_RMW_HEADER_SIZE];
	int hver, mver;
	uint32_t type, rlen, seq;
	char msg[MAX_AUDIT_MESSAGE_LENGTH+1];
	int n_tries_this_message = 0;
	time_t now, then = 0;

	sequence_id ++;

try_again:
	time (&now);
	if (then == 0)
		then = now;

	/* We want the first retry to be quick, in case the network
	   failed for some fail-once reason.  In this case, it goes
	   "failure - reconnect - send".  Only if this quick retry
	   fails do we start pausing between retries to prevent
	   swamping the local computer and the network.  */
	if (n_tries_this_message > 1)
		sleep (config.network_retry_time);

	if (n_tries_this_message > config.max_tries_per_record) {
		network_failure_handler ("max retries exhausted");
		return -1;
	}
	if ((now - then) > config.max_time_per_record) {
		network_failure_handler ("max retry time exhausted");
		return -1;
	}

	n_tries_this_message ++;

	if (!transport_ok) {
		if (init_transport ())
			goto try_again;
	}

	type = (s != NULL) ? AUDIT_RMW_TYPE_MESSAGE : AUDIT_RMW_TYPE_HEARTBEAT;
	AUDIT_RMW_PACK_HEADER (header, 0, type, len, sequence_id);

#ifdef USE_GSSAPI
	if (USE_GSS) {
		if (send_msg_gss (header, s, len)) {
			stop_transport ();
			goto try_again;
		}
	} else
#endif
	if (send_msg_tcp (header, s, len)) {
		stop_transport ();
		goto try_again;
	}

#ifdef USE_GSSAPI
	if (USE_GSS) {
		if (recv_msg_gss (header, msg, &rlen)) {
			stop_transport ();
			goto try_again;
		}
	} else
#endif
	if (recv_msg_tcp (header, msg, &rlen)) {
		stop_transport ();
		goto try_again;
	}

	AUDIT_RMW_UNPACK_HEADER (header, hver, mver, type, rlen, seq);
	msg[rlen] = 0;

	/* Handle this first. It doesn't matter if seq compares or not
	 * since the other end is going down...deal with it. */
	if (type == AUDIT_RMW_TYPE_ENDING)
		return remote_server_ending_handler (msg);

	if (seq != sequence_id) {
		/* FIXME: should we read another header and
		   see if it matches?  If so, we need to deal
		   with timeouts.  */
		if (sync_error_handler ("mismatched response"))
			return -1;
		stop_transport();
		goto try_again;
	}

	/* Specific errors we know how to deal with.  */
	if (type == AUDIT_RMW_TYPE_DISKLOW)
		return remote_disk_low_handler (msg);
	if (type == AUDIT_RMW_TYPE_DISKFULL)
		return remote_disk_full_handler (msg);
	if (type == AUDIT_RMW_TYPE_DISKERROR)
		return remote_disk_error_handler (msg);

	/* Generic errors.  */
	if (type & AUDIT_RMW_TYPE_FATALMASK)
		return generic_remote_error_handler (msg);
	if (type & AUDIT_RMW_TYPE_WARNMASK)
		return generic_remote_warning_handler (msg);

	return 0;
}

static int relay_sock(const char *s, size_t len)
{
	int rc;

	switch (config.format)
	{
		case F_MANAGED:
			rc = relay_sock_managed (s, len);
			break;
		case F_ASCII:
			rc = relay_sock_ascii (s, len);
			break;
		default:
			rc = -1;
			break;
	}

	return rc;
}

/* Send audit event to remote system */
static int relay_event(const char *s, size_t len)
{
	int rc;

	switch (config.transport)
	{
		case T_TCP:
			rc = relay_sock(s, len);
			break;
		default:
			rc = -1;
			break;
	}

	return rc;
}

