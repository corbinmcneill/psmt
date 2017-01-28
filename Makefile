all: send receive

psmt.o: psmt.c psmt.h
	gcc -Wall -Wextra -c psmt.c

send: send.c psmt.o naive_poly.o
	gcc -Wall -Wextra -c send.c
	gcc -Wall -Wextra send.o psmt.o naive_poly.o -o send

receive: receive.c psmt.o naive_poly.o
	gcc -Wall -Wextra -c receive.c
	gcc -Wall -Wextra receive.o psmt.o naive_poly.o -o receive

naive_poly.o: naive_poly.c poly.h
	gcc -Wall -Wextra -c naive_poly.c

clean:
	rm *.o 
	rm send 
	rm receive
	rm pipes/*

