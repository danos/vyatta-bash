# This is the Makefile for the examples subdirectory of readline. -*- text -*-
#
EXECUTABLES = fileman rltest
CFLAGS  = -g
LDFLAGS = -g

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(EXECUTABLES)

fileman: fileman.o
	$(CC) $(LDFLAGS) -o $@ fileman.o -lreadline -lncurses

rltest: rltest.o
	$(CC) $(LDFLAGS) -o $@ rltest.o -lreadline -lncurses

fileman.o: fileman.c
rltest.o: rltest.c
