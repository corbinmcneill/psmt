#CFLAGS= -pg -pthread -ggdb -Wall -Wextra -std=gnu11 -Wno-unused-variable -Wno-unused-parameter
CFLAGS= -pthread -Wall -Wextra -std=gnu11 -Wno-unused-variable -Wno-unused-parameter -O3

.PHONY: all fieldpoly clean mc

all: send receive fieldpoly 


mc: mciotestreceive mciotestsend

fieldpoly:
	make -C fieldpoly	

psmt.o: psmt.c psmt.h fieldpoly mcio.h
	gcc $(CFLAGS) -c psmt.c

send: send.c psmt.o mcio.o debug.o pq.o fieldpoly
	gcc $(CFLAGS) -c send.c
	gcc $(CFLAGS) send.o mcio.o pq.o psmt.o debug.o fieldpoly/fieldpoly.o fieldpoly/ff256.o fieldpoly/element.o -o send

receive: receive.c psmt.o mcio.o debug.o pq.o fieldpoly
	gcc $(CFLAGS) -c receive.c
	gcc $(CFLAGS) receive.o mcio.o pq.o psmt.o debug.o fieldpoly/fieldpoly.o fieldpoly/ff256.o fieldpoly/element.o -o receive

mciotestreceive: mciotestreceive.c mcio.o pq.o debug.o
	gcc $(CFLAGS) -c mciotestreceive.c
	gcc $(CFLAGS) mciotestreceive.o mcio.o debug.o pq.o -o mciotestreceive

mciotestsend: mciotestsend.c mcio.o pq.o debug.o
	gcc $(CFLAGS) -c mciotestsend.c
	gcc $(CFLAGS) mciotestsend.o mcio.o debug.o pq.o -o mciotestsend


mcio.o: mcio.c debug.o mcio.h pq.o
	gcc $(CFLAGS) -c mcio.c

pqtest: pqtest.c pq.o debug.o 
	gcc $(CFLAGS) -c pqtest.c
	gcc $(CFLAGS) pq.o pqtest.o debug.o -o pqtest

pq.o: pq.c pq.h debug.c debug.h
	gcc $(CFLAGS) -c pq.c

psmt_test: psmt_test.c psmt.o mcio.o pq.o debug.o
	gcc $(CFLAGS) -c psmt_test.c 
	gcc $(CFLAGS) psmt_test.o psmt.o mcio.o pq.o debug.o fieldpoly/ff256.o fieldpoly/element.o fieldpoly/fieldpoly.o -o psmt_test

debug.o: debug.c debug.h
	gcc $(CFLAGS) -c debug.c

clean:
	rm -f *.o 
	rm -f send 
	rm -f receive
	rm -f psmt_test
	rm -f pipes/*
	rm -f mciotestsend mciotestreceive 
	make -C fieldpoly clean
