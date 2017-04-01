#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include "mcio.h"
#include "pq.h"

#define MAX_BUF 1000

int* wires;
int numwires;
tppq pq;

pthread_mutex_t pq_lock;


/* function declarations */
void listener();

int init_mcio(int* fds, int n){
    pthread_mutex_init(buf_lock);
    wires = fd;
    numwires = n;

    initpq(&pq,MAX_BUF);    

    /* set the fd to nonblock mode*/
    int flags; 
    for (int i = 0; i<n; i++) {
        flags = fcntl(wires[i],F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(wires[i],F_SETFL,flags);
    }
        
}


int mc_read(trans_packet* data, int* wire) {
}

int mc_write(trans_packet* data, int wire) {
}

int mc_destroy() {
    free(wires);
}

void listener() {
    transpacket* input = malloc(sizeof(transpacket));
    int bytesread;
    while(1) {
       for (int i = 0; i < numwires; i++){
           bytesread = 
           


}



