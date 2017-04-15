#include "psmt.h"
#include "mcio.h"
#include "debug.h"
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define FNAME_LEN 50
	int i;
	int* rfds, *wfds;

int main() {
    rfds = malloc(sizeof(int)*N);
    wfds = malloc(sizeof(int)*N);
    debug("opening pipes\n");
	for (i=0; i<N; i++) {
		char rfilename[FNAME_LEN], wfilename[FNAME_LEN];;
		snprintf(rfilename,FNAME_LEN,"pipes/pipe_a_%d",i);
		snprintf(wfilename,FNAME_LEN,"pipes/pipe_b_%d",i);
		if ((rfds[i] = open(rfilename,O_RDONLY)) < 0) {
			printf("mciotestsend.c: read open fail");
			exit(-1);
		}
		if ((wfds[i] = open(wfilename,O_WRONLY)) < 0) {
			printf("mciotestsend.c: write open fail");
			exit(-1);
		}
	}
    debug("done opening pipes\n");

	debug("sending stuff\n");

    mc_init(rfds, wfds, N);
    trans_packet tp; 
    tp.seq_num = 1;
    tp.round_num = 3;
    tp.h_vals[1][1] = 42;
    mc_write(&tp,-1);
    mc_write(&tp,-1);
    mc_write(&tp,-1);
    return 0;
}



void testround1() {
    trans_packet toSend;
    toSend.seq_num = 3;
    toSend.round_num = 1;
    toSend.h_vals[1][1] = 42;
    mc_write(&toSend,3);

    toSend.seq_num = 2;
    toSend.round_num = 1;
    toSend.h_vals[1][1] = 9;
    mc_write(&toSend,3);

    toSend.seq_num = 1;
    toSend.round_num = 1;
    toSend.h_vals[1][1] = 5;
    mc_write(&toSend,3);

toSend.seq_num = 0;
    toSend.round_num = 1;
    toSend.h_vals[1][1] = 5;
    mc_write(&toSend,3);


toSend.seq_num = 1;
    toSend.round_num = 1;
    toSend.h_vals[1][1] = 5;
    mc_write(&toSend,3);
toSend.seq_num = 1;
    toSend.round_num = 1;
    toSend.h_vals[1][1] = 5;
    mc_write(&toSend,3);
toSend.seq_num = 1;
    toSend.round_num = 1;
    toSend.h_vals[1][1] = 5;
    mc_write(&toSend,3);
toSend.seq_num = 1;
    toSend.round_num = 1;
    toSend.h_vals[1][1] = 5;
    mc_write(&toSend,3);

toSend.seq_num = 500;
    toSend.round_num = 1;
    toSend.h_vals[1][1] = 5;
    mc_write(&toSend,3);
}
