#include "psmt.h"
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define FNAME_LEN 50

int main() {
	int i;
	int fds[N];
	for (i=0; i<N; i++) {
		char filename[FNAME_LEN];
		snprintf(filename,FNAME_LEN,"pipes/pipe_%d",i);
		if ((fds[i] = open(filename,O_RDWR)) < 0) {
			printf("send.c: open fail");
			exit(-1);
		}
	}

	receive_info(fds, N);
	return 0;
}
