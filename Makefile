CFLAGS= -ggdb -Wall -Wextra -std=gnu11 -Wno-unused-variable -Wno-unused-parameter

all: send receive fieldpoly

fieldpoly:
	make -C fieldpoly	

psmt.o: psmt.c psmt.h fieldpoly mcio.h
	gcc $(CFLAGS) -c psmt.c

send: send.c psmt.o debug.o fieldpoly
	gcc $(CFLAGS) -c send.c
	gcc $(CFLAGS) send.o psmt.o debug.o fieldpoly/fieldpoly.o fieldpoly/ff256.o fieldpoly/element.o -o send

receive: receive.c psmt.o debug.o fieldpoly
	gcc $(CFLAGS) -c receive.c
	gcc $(CFLAGS) receive.o psmt.o debug.o fieldpoly/fieldpoly.o fieldpoly/ff256.o fieldpoly/element.o -o receive


mcio.o: mcio.c pq.o
	gcc $(CFLAGS) -c mcio.c

pqtest: pqtest.c pq.o debug.o 
	gcc $(CFLAGS) -c pqtest.c
	gcc $(CFLAGS) pq.o pqtest.o debug.o -o pqtest

pq.o: pq.c 
	gcc $(CFLAGS) -c pq.c


	

debug.o: debug.c debug.h
	gcc $(CFLAGS) -c debug.c

clean:
	rm -f *.o 
	rm -f send 
	rm -f receive
	rm -f pipes/*
