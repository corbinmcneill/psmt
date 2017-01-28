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
	int fds[N];
	int i;

	for (i=0; i<N; i++) {
		char filename[FNAME_LEN];
		snprintf(filename,FNAME_LEN,"pipes/pipe_%d",i);
		if (!mkfifo(filename, 0777)){
			if (errno == EEXIST) {
				/*fail silently */
				/*printf("send.c: pipe exists"); */
			}
			else {
				printf("send.c: mkfifo fail %s", strerror(errno));
				exit(-1);
			}
		}
		if ((fds[i] = open(filename,O_RDWR)) < 0) {
			printf("send.c: open fail");
			exit(-1);
		}
	}

	char *message = "test message\n"; 
	send_info(message, strlen(message), fds, N);
	return 0;
}
