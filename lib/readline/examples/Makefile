# This is the Makefile for the examples subdirectory of readline. -*- text -*-
#
EXECUTABLES = fileman rltest rl
CFLAGS  = -g 
LDFLAGS = -g 

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(EXECUTABLES)


rl: rl.o
	$(CC) $(LDFLAGS) -o $@ rl.o -lreadline -lncurses

fileman: fileman.o
	$(CC) $(LDFLAGS) -o $@ fileman.o -lreadline -lncurses

rltest: rltest.o
	$(CC) $(LDFLAGS) -o $@ rltest.o -lreadline -lncurses

fileman.o: fileman.c
rltest.o: rltest.c
rl.o: rl.c
