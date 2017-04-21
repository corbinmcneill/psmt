#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "psmt.h"
#include "mcio.h"
#include "debug.h"

char *message = "to be, or not to be: that is the question: Whether 'tis nobler in the mind to suffer The slings and arrows of outrageous fortune, Or to take arms against a sea of troubles, And by opposing end them? To die: to sleep; No more; and by a sleep to say we end The heart-ache and the thousand natural shocks That flesh is heir to, 'tis a consummation Devoutly to be wish'd. To die, to sleep; To sleep: perchance to dream: ay, there's the rub; For in that sleep of death what dreams may come When we have shuffled off this mortal coil, Must give us pause: there's the respect That makes calamity of so long life; For who would bear the whips and scorns of time, The oppressor's wrong, the proud man's contumely, The pangs of despised love, the law's delay, The insolence of office and the spurns That patient merit of the unworthy takes, When he himself might his quietus make With a bare bodkin? who would fardels bear, To grunt and sweat under a weary life, But that the dread of something after death, The undiscover'd country from whose bourn No traveller returns, puzzles the will And makes us rather bear those ills we have Than fly to others that we know not of? Thus conscience does make cowards of us all; And thus the native hue of resolution Is sicklied o'er with the pale cast of thought, And enterprises of great pith and moment With this regard their currents turn awry, And lose the name of action.--Soft you now! The fair Ophelia! Nymph, in thy orisons Be all my sins remember'd."; 
unsigned int message_n;

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

    //find message_n
    for (message_n = 0; message[message_n] != 0; message_n++){}

	for (unsigned int i=0; i<message_n; i++) {
		debug("%c\n", message[i]);
		send_char(message[i]);
	}
	
	sleep(500);

	pthread_cancel(send_spin_thread);
	pthread_attr_destroy(&attr);
	mc_destroy();

	return 0;
}
