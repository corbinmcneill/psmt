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
	int rfds[N], wfds[N];
	for (i=0; i<N; i++) {
		char rfilename[FNAME_LEN], wfilename[FNAME_LEN];;
		snprintf(rfilename,FNAME_LEN,"pipes/pipe_a_%d",i);
		snprintf(wfilename,FNAME_LEN,"pipes/pipe_b_%d",i);
		if ((rfds[i] = open(rfilename,O_RDONLY)) < 0) {
			printf("send.c: read open fail");
			exit(-1);
		}
		if ((wfds[i] = open(wfilename,O_WRONLY)) < 0) {
			printf("send.c: write open fail");
			exit(-1);
		}
	}

	char *message = "test message\n"; 

	/* thread spawn here */

	return 0;
}
