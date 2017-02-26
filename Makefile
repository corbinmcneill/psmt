CFLAGS= -ggdb -Wall -Wextra -std=gnu11 -Wno-unused-variable -Wno-unused-parameter

all: send receive fieldpoly

fieldpoly:
	make -C fieldpoly	

psmt.o: psmt.c psmt.h fieldpoly
	gcc $(CFLAGS) -c psmt.c

send: send.c psmt.o debug.o fieldpoly
	gcc $(CFLAGS) -c send.c
	gcc $(CFLAGS) send.o psmt.o debug.o fieldpoly/fieldpoly.o fieldpoly/ff256.o fieldpoly/element.o -o send

receive: receive.c psmt.o debug.o fieldpoly
	gcc $(CFLAGS) -c receive.c
	gcc $(CFLAGS) receive.o psmt.o debug.o fieldpoly/fieldpoly.o fieldpoly/ff256.o fieldpoly/element.o -o receive

debug.o: debug.c debug.h
	gcc $(CFLAGS) -c debug.c

clean:
	rm *.o 
	rm send 
	rm receive
	rm tests
	rm pipes/*
