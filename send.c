#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "psmt.h"
#include "mcio.h"
#include "debug.h"

int main(int argc, char **argv) {
	int i;
	int rfds[N], wfds[N];
    char cmdline[5000]  = "scripts/sen_repipe cslab24 ";
    assert(argc == 2);
    strcat(cmdline,argv[1]);
    printf("cmdline=%s\n",cmdline);
    printf("sizeof(trans_packet)=%d\n",sizeof(trans_packet));
    if (system(cmdline) < 0) {
             printf("repipe error");
             exit(-1);
    }

	for (i=0; i<N; i++) {
		char rfilename[FNAME_LEN], wfilename[FNAME_LEN];;
		snprintf(rfilename,FNAME_LEN,"/tmp/pipes/pipe_a_%d",i);
		snprintf(wfilename,FNAME_LEN,"/tmp/pipes/pipe_b_%d",i);
        debug("opening pipes\n");
		if ((rfds[i] = open(rfilename,O_RDONLY)) < 0) {
			printf("send.c: read open fail");
			exit(-1);
		}
		if ((wfds[i] = open(wfilename,O_WRONLY)) < 0) {
			printf("send.c: write open fail");
			exit(-1);
		}
	}
	mc_init(rfds, wfds, N);
	psmt_init();

	/* Note: with proper synchronization, this code could
	 * (and should) be extended later on to support multiple
	 * spinning threads. */
	pthread_t send_spin_thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (pthread_create(&send_spin_thread, &attr, &send_spin, NULL)) {
		printf("send error: thread_create - %s", strerror(errno));
		return 1;
	}

    char buf;
    while(read(0, &buf,1) > 0) {
        send_char(buf);
    }
    send_char(END_TRANSMISSION);


	pthread_attr_destroy(&attr);
    //mc_destroy done in send_spin
    sleep(500);

	return 0;
}
