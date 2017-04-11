#include "psmt.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

char *message = "test message\n"; 
unsigned int message_n = 13;

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

	psmt_init();

	/* Note: with proper synchronization, this code could
	 * (and should) be extended later on to support multiple
	 * spinning threads. */
	pthread_t sender_spin;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (pthread_create(&sender_spin, &attr, send_spin, NULL)) {
		printf("send error: thread_create - %s", strerror(errno));
		return 1;
	}

	for (unsigned int i=0; i<message_n; i++) {
		send_char(message[i]);
	}
	
	pthread_cancel(&sender_spin);
	pthread_attr_destroy(&attr);

	return 0;
}
