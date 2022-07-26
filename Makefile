CC = gcc
CFLAGS = -Wall
LIBS =  -lpthread

PROGS =	server-fork server-thread server-select  server-poll et-server-epoll lt-server-epoll client

all:	${PROGS}

server-fork: server-fork.c
		${CC} ${CFLAGS} -o $@ $^ ${LIBS}

server-thread:	server-thread.c
		${CC} ${CFLAGS} -o $@ $^ ${LIBS}

server-select:	server-select.c
		${CC} ${CFLAGS} -o $@ $^ ${LIBS}

server-poll: server-poll.c
		${CC} ${CFLAGS} -o $@ $^ ${LIBS}

et-server-epoll: server-epoll-et.c
		${CC} ${CFLAGS} -o $@ $^ ${LIBS}

lt-server-epoll: server-epoll-lt.c
		${CC} ${CFLAGS} -o $@ $^ ${LIBS}

client:	client.c
		${CC} ${CFLAGS} -o $@ $^ ${LIBS}

clean:
		rm -f ${PROGS} *.o
