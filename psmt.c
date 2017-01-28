#include <stdio.h>
#include <unistd.h>
#include <stddef.h>

#include "psmt.h"

#define SEC_LEN 50

int send_info(char *secret, size_t secret_n, int *fds, size_t fds_n) {
	size_t i;
	for (i=0; i<fds_n; i++) {
		write(fds[i],secret,secret_n);
	}
	return 0;
}

int receive_info(int *fds, size_t fds_n) {
	size_t i;
	char output[SEC_LEN];
	for (i=0; i<fds_n; i++) {
		read(fds[i], output, SEC_LEN);
		printf(output);
	}
	return 0;
}

