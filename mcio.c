#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include "mcio.h"
#include "pq.h"

#define MAX_BUF 1000
#define MAX_THREADS 8

int* wires;
int numwires;
tppq pq;

pthread_mutex_t pq_lock;


/* function declarations */
void *listener(void *threadid);

int mc_init(int* fds, int n){
    pthread_mutex_init(&pq_lock,NULL);
    int* wires = fds;
    int numwires = n;
    pthread_t threads[MAX_THREADS];
    int retval  = pthread_create(&threads[0], NULL, listener, NULL); 
    assert(!retval);

    initpq(&pq,MAX_BUF);    

    /* set the fd to nonblock mode*/
    int flags; 
    for (int i = 0; i<n; i++) {
        flags = fcntl(wires[i],F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(wires[i],F_SETFL,flags);
    }
    return 0;
        
}

int mc_destroy() {
    free(wires);
    return 0;
}



int mc_read(trans_packet* data, int* wire) {
    mc_packet toReturn = pop(&pq);
    *data = toReturn.tp;
    return toReturn.wire;
}

int mc_write(trans_packet* data, int wire) {
   int byteswritten = write(wire, data, sizeof(trans_packet));
   assert(byteswritten == sizeof(transpacket));
   if (wire == -1) {
       for (int i = 0; i < numwires; i++) {
           byteswritten = write(wires[i], data, sizeof(trans_packer));
           assert(byteswritten == sizeof(transpacket));
       }
   }
   return 0;
}

//TODO: add epoll
void* listener(void *threadid) {
    mc_packet* input = malloc(sizeof(mc_packet));
    size_t bytesread;
    while(1) {
       for (int i = 0; i < numwires; i++){
           bytesread = read(wires[i],&(input->tp),sizeof(trans_packet)); 
           // TODO make nice
           assert(bytesread == sizeof(trans_packet));
           pthread_mutex_lock(&pq_lock);
           insert(&pq, input);
           pthread_mutex_unlock(&pq_lock);
       }
    }
}
       
           




