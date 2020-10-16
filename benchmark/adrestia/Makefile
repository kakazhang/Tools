CFLAGS = -Wall -lpthread -I $(PWD)/include -g

OBJS = wake.o stats.o

all: adrestia
adrestia: adrestia.c $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

clean: 
	$(RM) adrestia $(OBJS)
