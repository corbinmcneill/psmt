#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "psmt.h"
#include "mcio.h"
#include "debug.h"

#define FNAME_LEN 50

int main() {

	//this is terrible, we'll fix it later
    debug("repipe \n");
	if (system("scripts/rec_repipe cslab25") < 0) {
		printf("repipe error");
		exit(-1);
	}
    debug("finished repipe \n");

	int i;
	int rfds[N], wfds[N];
	for (i=0; i<N; i++) {
        //fprintf(stderr, "setting up pipe %d\n",i);
		char rfilename[FNAME_LEN], wfilename[FNAME_LEN];;
		snprintf(rfilename,FNAME_LEN,"/tmp/pipes/pipe_b_%d",i);
	 	snprintf(wfilename,FNAME_LEN,"/tmp/pipes/pipe_a_%d",i);
	 	/* debug printf's 
	 	printf("rfilename: %s\n", rfilename);
	 	printf("wfilename: %s\n", wfilename);
	 	fflush(stdout);
	 	*/
        debug("opening pipes\n");
		if ((wfds[i] = open(wfilename,O_WRONLY)) < 0) {
			perror("receive.c: write open fail");
            
			exit(-1);
		}
		if ((rfds[i] = open(rfilename,O_RDONLY)) < 0) {
			perror("receive.c: read open fail");
			exit(-1);
		}
	}

	debug("calling mc_init\n");
	mc_init(rfds,wfds,N);
	psmt_init();
    char* message;
	receive_spin(NULL);
    debug("ending receive\n");
    sleep(1);
    psmt_destroy();
    mc_destroy();
	return 0;
}
