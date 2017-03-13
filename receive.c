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
	//this is terrible, we'll fix it later
	if (system("scripts/repipe") < 0) {
		printf("repipe error");
		exit(-1);
	}

	int i;
	int rfds[N], wfds[N];
	for (i=0; i<N; i++) {
		char rfilename[FNAME_LEN], wfilename[FNAME_LEN];;
		snprintf(rfilename,FNAME_LEN,"pipes/pipe_b_%d",i);
	 	snprintf(wfilename,FNAME_LEN,"pipes/pipe_a_%d",i);
	 	/* debug printf's 
	 	printf("rfilename: %s\n", rfilename);
	 	printf("wfilename: %s\n", wfilename);
	 	fflush(stdout);
	 	*/
		if ((wfds[i] = open(wfilename,O_WRONLY)) < 0) {
			printf("receive.c: write open fail");
			exit(-1);
		}
		if ((rfds[i] = open(rfilename,O_RDONLY)) < 0) {
			printf("receive.c: read open fail");
			exit(-1);
		}
	}

	printf("calling receive_info\n");
	receive_info(rfds, wfds, N);
	return 0;
}
