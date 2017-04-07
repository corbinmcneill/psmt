#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include "mcio.h"
#include "pq.h"
#include "debug.h"

#define MAX_BUF 1000
#define MAX_THREADS 8

int* wfds;
int* rfds;
int numwires;
tppq pq;

pthread_mutex_t pq_lock;


/* function declarations */
void *listener(void *threadid);

int mc_init(int* rfds_in,int* wfds_in, int n){
    pthread_mutex_init(&pq_lock,NULL);
    wfds = wfds_in;
    rfds = rfds_in;
    numwires = n;
    long t = 0;
    pthread_t threads[MAX_THREADS];
    int retval  = pthread_create(&threads[t], NULL, listener, (void*) t); 
    assert(!retval);

    initpq(&pq,MAX_BUF);    

    /* set the fd to nonblock mode*/
    int flags; 
    for (int i = 0; i<n; i++) {
        flags = fcntl(rfds[i],F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(rfds[i],F_SETFL,flags);
    }
    return 0;
        
}

int mc_destroy() {
    free(rfds);
    free(wfds);
    return 0;
}



int mc_read(trans_packet* data, int* wire) {
    if (pq.size == 0){
        return 0;
    }
    mc_packet received = pop(&pq);
    *data = received.tp;
    *wire = received.wire;
    return 1;
}

int mc_write(trans_packet* data, int wire) {
   debug("writing packet, sequence %d, round %d, wire %d\n", 
           data->seq_num, data->round_num, wire);
   int byteswritten = write(wfds[wire], data, sizeof(trans_packet));
   assert(byteswritten == sizeof(trans_packet));
   if (wire == -1) {
       for (int i = 0; i < numwires; i++) {
           byteswritten = write(wfds[i], data, sizeof(trans_packet));
           assert(byteswritten == sizeof(trans_packet));
       }
   }
   return 1;
}

//TODO: add epoll
void* listener(void *threadid) {
    mc_packet* input = malloc(sizeof(mc_packet));
    int bytesread;
    debug("starting listener\n");
    while(1) {
       for (int i = 0; i < numwires; i++){
           usleep(50);
           bytesread = read(rfds[i],&(input->tp),sizeof(trans_packet)); 
           if (bytesread == sizeof(trans_packet) ) {
                input->wire = i;
                debug("listener got packet, sequence %d, round %d\n", input->tp.seq_num, input->tp.round_num);
                pthread_mutex_lock(&pq_lock);
                insert(&pq, input);
                pthread_mutex_unlock(&pq_lock);
           }
           else if ((bytesread != 0) && (bytesread != -1 && errno != EAGAIN)){
               debug("Read error, read %d bytes\n",bytesread);
           }
       }
    }
}
       
           




