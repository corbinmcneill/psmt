#include "psmt.h"
#include "debug.h"
#include "mcio.h"
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FNAME_LEN 50

int main() {
	if (system("scripts/repipe") < 0) {
		printf("repipe error");
		exit(-1);
	}

	int i;
	int* rfds, *wfds;
    rfds = malloc(sizeof(int)*N);
    wfds = malloc(sizeof(int)*N);
	for (i=0; i<N; i++) {
		char rfilename[FNAME_LEN], wfilename[FNAME_LEN];;
		snprintf(rfilename,FNAME_LEN,"pipes/pipe_b_%d",i);
	 	snprintf(wfilename,FNAME_LEN,"pipes/pipe_a_%d",i);
		if ((wfds[i] = open(wfilename,O_WRONLY)) < 0) {
			printf("mciotestreceive.c: write open fail");
			exit(-1);
		}
		if ((rfds[i] = open(rfilename,O_RDONLY)) < 0) {
			printf("mciotestreceive.c: read open fail");
			exit(-1);
		}
	}

	debug("receiving stuff\n");
    mc_init(rfds,wfds,N);
    while (1) {
        usleep(500);
        trans_packet data;
        int wire;
        if (mc_read(&data, &wire)) {
            debug("got packet from wire %d\n", wire);
            debug("\tSequence #%d\n",data.round_num);
            debug("\tRound #%d\n",data.round_num);
            debug("\th[1][1] = %d\n", data.h_vals[1][1]);
        }
        
    }
	return 0;
}
