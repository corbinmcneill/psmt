CFLAGS= -ggdb -Wall -Wextra -std=gnu11


all: send receive tests fieldpoly

fieldpoly:
	make -C fieldpoly	

psmt.o: psmt.c psmt.h
	gcc $(CFLAGS) -c psmt.c

send: send.c psmt.o naive_poly.o libgfshare.o debug.o
	gcc $(CFLAGS) -c send.c
	gcc $(CFLAGS) send.o psmt.o naive_poly.o libgfshare.o debug.o -o send

receive: receive.c psmt.o naive_poly.o libgfshare.o debug.o
	gcc $(CFLAGS) -c receive.c
	gcc $(CFLAGS) receive.o psmt.o naive_poly.o libgfshare.o debug.o -o receive

debug.o: debug.c debug.h
	gcc $(CFLAGS) -c debug.c

libgfshare.o: libgfshare.c libgfshare.h
	gcc $(CFLAGS) -c libgfshare.c


clean:
	rm *.o 
	rm send 
	rm receive
	rm tests
	rm pipes/*

