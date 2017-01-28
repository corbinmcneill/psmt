all: send receive

psmt.o: psmt.c psmt.h
	gcc -Werror -Wall -Wextra -c psmt.c

send: send.c psmt.o
	gcc -Werror -Wall -Wextra -c send.c
	gcc -Werror -Wall -Wextra send.o psmt.o -o send

receive: receive.c psmt.o
	gcc -Werror -Wall -Wextra -c receive.c
	gcc -Werror -Wall -Wextra receive.o psmt.o -o receive

clean:
	rm *.o send receive
	rm pipes/*

