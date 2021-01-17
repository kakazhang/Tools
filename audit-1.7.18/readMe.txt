1.source code download reference:
  http://repository.timesys.com/buildsources/a/audit/audit-1.7.18/
2.build:
  1) config and set build out dir
  ./configure CC=arm-linux-gnueabihf-gcc --host=arm-linux-gnueabihf --prefix=/home/kakazhang/out

  2) make
     this will cause audit_rule not found error,fix it ,add audit_rule to libaudit.h
     struct audit_rule {     /* for AUDIT_LIST, AUDIT_ADD, and AUDIT_DEL */
       uint32_t  flags;  /* AUDIT_PER_{TASK,CALL}, AUDIT_PREPEND */
       uint32_t  action; /* AUDIT_NEVER, AUDIT_POSSIBLE, AUDIT_ALWAYS */
       uint32_t  field_count;
       uint32_t  mask[AUDIT_BITMASK_SIZE];
       uint32_t  fields[AUDIT_MAX_FIELDS];
       uint32_t  values[AUDIT_MAX_FIELDS];
     };
  3) make install


3. config kernel
  CONFIG_AUDIT=y
  CONFIG_HAVE_ARCH_AUDITSYSCALL=y
  CONFIG_AUDITSYSCALL=y
  CONFIG_AUDIT_WATCH=y
  CONFIG_AUDIT_TREE=y

4. config environment
  cp -rfp /out/sbin/* /sbin
  cp -rfp /out/bin/* /bin
  cp -rfp /out/lib/libaudit.so* /lib
  cp -rfp /out/lib/libauparse.so* /lib
  cp -rfp /out/etc/audit/ /etc

  mkdir -p /var/log
  chown root /etc/audit/auditd.conf
  chown root /sbin/audispd
  chmod 0750 /sbin/audispd
