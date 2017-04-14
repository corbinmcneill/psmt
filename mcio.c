#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "mcio.h"
#include "pq.h"
#include "debug.h"
#include "params.h"

#define MAX_BUF 1000
#define MAX_THREADS 8
// the maximum number of out of order packets a good wire can send
#define MAX_MISORDERED 100

int* wfds;
int* rfds;

int numwires;
tppq pq;

pthread_mutex_t done;
pthread_mutex_t pq_lock;

/* struct declarations */
typedef struct public_trans_packet{
    trans_packet tp;
    int votes;
    char* already_voted;
    char not_empty;
    char row_finished;
} p_tp;


/* function declarations */
void *listener(void *threadid);
int markreceivedround1(int seqnum, int wire,long* lastseqread,long* firstseqnotread, int* start_received, char* received); 
void process_public(trans_packet tp, int wire, p_tp* public_cache, int start_pc, unsigned long firstpublicnotfinished);



int mc_init(int* rfds_in,int* wfds_in, int n){
    debug("mc_init was called\n");

    // set up the communication for cleanup
    pthread_mutex_init(&done,NULL);
    pthread_mutex_lock(&done);

    // set the wire information
    wfds = wfds_in;
    rfds = rfds_in;
    numwires = n;

    // set up the priority queue
    pthread_mutex_init(&pq_lock,NULL);
    pthread_mutex_lock(&pq_lock);
    initpq(&pq,MAX_BUF);    
    pthread_mutex_unlock(&pq_lock);
    
    
    /* set the fds to nonblock mode*/
    int flags; 
    for (int i = 0; i<n; i++) {
        flags = fcntl(rfds[i],F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(rfds[i],F_SETFL,flags);
    }

   

    // create the listener
    long t = 0;
    pthread_t threads[MAX_THREADS];
    int retval  = pthread_create(&threads[t], NULL, listener, (void*) t); 
    debug("listener thread was created. retval: %d\n", retval);
    assert(!retval);

    return 0;
        
}

int mc_destroy() {
    pthread_mutex_unlock(&done);
    // TODO fix return?
    return 0;
}

// called by listener on exit
void finish_cleanup() {
    free(rfds);
    free(wfds);
    pthread_mutex_lock(&pq_lock);
    destroypq(&pq);
    pthread_mutex_destroy(&done);
    pthread_mutex_destroy(&pq_lock);
}




int mc_read(trans_packet* data, int* wire) {
    if (pq.size == 0){
        return 0;
    }
    pthread_mutex_lock(&pq_lock);
    mc_packet received_packet = pop(&pq);
    pthread_mutex_unlock(&pq_lock);
    *data = received_packet.tp;
    *wire = received_packet.wire;
    return 1;
}

int mc_write(trans_packet* data, int wire) {
   debug("writing packet, sequence %d, round %d, wire %d\n", 
           data->seq_num, data->round_num, wire);
   int byteswritten;
   if (wire == -1) {
       for (int i = 0; i < numwires; i++) {
           byteswritten = write(wfds[i], data, sizeof(trans_packet));
           assert(byteswritten == sizeof(trans_packet));
       }
   }
   else {
       byteswritten = write(wfds[wire], data, sizeof(trans_packet));
       assert(byteswritten == sizeof(trans_packet));
   }
   return 1;
}

void safe_insert(mc_packet* packet) {
    pthread_mutex_lock(&pq_lock);
    insert(&pq, packet);
    pthread_mutex_unlock(&pq_lock);
}


//TODO: add epoll
void* listener(void *threadid) {
	debug("listener was called\n");
 
    // the first round 1 sequence number that hasn't been read by a given wire
    long* firstseqnotread;
    // the last round 1 sequence number read by a given wire
    long* lastseqread;
    /* recieved[wire*((start_recieved + i) % MAX_MISORDERED)] = 1 iff the 
     * (firstseqnotread[wire]+i)th sequence has been received */
    int* start_received;
    char* received;
    mc_packet* input = malloc(sizeof(mc_packet));
    lastseqread = malloc(sizeof(long)*numwires);
    firstseqnotread = calloc(sizeof(long),numwires);
    received = calloc(MAX_MISORDERED*numwires*sizeof(char),1);
    start_received = calloc(numwires,sizeof(int));

    unsigned long firstpublicnotfinished = 0;
    //public_cache[(seq_num - firstpublicnotfinished)*wire] 
    p_tp* public_cache = calloc(sizeof(p_tp)*numwires*MAX_MISORDERED,1);
    int start_pc =0;
    //initialize the public cache to empty
    for (int i = 0; i < MAX_MISORDERED; i++) {
        for (int j = 0; j < numwires; j++) {
            public_cache[i*numwires+j].not_empty = 0;
            public_cache[i*numwires+j].already_voted = calloc(sizeof(int)*numwires,1);
        }
    }

    
    
    for (int i = 0; i < numwires; i++) {
        lastseqread[i] = -1;
    }

    int bytesread;
    debug("starting listener\n");
    while(1) {
        // check if we are done and finish cleanup
       if (!pthread_mutex_trylock(&done)) {
           finish_cleanup();
           break;
       }
       for (int i = 0; i < numwires; i++){
           bytesread = read(rfds[i],&(input->tp),sizeof(trans_packet)); 
           if (bytesread == sizeof(trans_packet) ) {
                input->wire = i;
                debug("listener got packet, sequence %d, round %d\n", input->tp.seq_num, input->tp.round_num);
                if (input->tp.round_num == 1) {
                    if (markreceivedround1(input->tp.seq_num,i,lastseqread,firstseqnotread,start_received,received)) {
                        safe_insert(input);
                    }
                }
                else {
                    process_public(input->tp,i,public_cache,start_pc,firstpublicnotfinished);
                }
                    
           }
           else if ((bytesread != 0) && (bytesread != -1 && errno != EAGAIN)){
               debug("Read error, read %d bytes\n",bytesread);
           }

       }
    }

    for (int i = 0; i < MAX_MISORDERED; i++) {
        for (int j = 0; j < numwires; j++) {
            free(public_cache[i*numwires+j].already_voted);
        }
    }
    free(input);
    free(lastseqread);
    free(firstseqnotread);
    free(received);
    free(start_received);
    free(public_cache);
    pthread_exit(NULL);
}
/*
 * returns 1 if the packet should be kept and 0 if it should be destroyed.
 */
int markreceivedround1(int seqnum, int wire,long* lastseqread,long* firstseqnotread, int* start_received, char* received) {
    debug("markreceived(%d,%d) called\n", seqnum, wire);
    debug("firstseqnotread[wire]=%d, lastseqread[wire]=%d\n", firstseqnotread[wire],lastseqread[wire] );
    int to_return = -1;
    if (seqnum < firstseqnotread[wire]) {
        to_return = 0;
    } else if (seqnum == firstseqnotread[wire]) {
        int next;
        int i;
        int index;
        for (i = 1; i < lastseqread[wire] - firstseqnotread[wire] ; i++) {
            index =  (i+1+start_received[wire])%MAX_MISORDERED;
            if (!received[wire*index]) {
                received[wire*index] = 0;
                break;
            }
            received[wire*index] = 0;
        }
        debug("moving firstseqnotread[%d] from %d to ", wire, firstseqnotread[wire]);
        firstseqnotread[wire] += i;
        debug("%d\n",firstseqnotread[wire]);

        start_received[wire] = index;
        if (seqnum > lastseqread[wire]) {
            lastseqread[wire] = seqnum;
        }
        to_return = 1;
    } else if (seqnum < lastseqread[wire]) {
        int index = wire*((seqnum + start_received[wire]) % MAX_MISORDERED);
        if (received[index] == 1) {
            to_return = 0;
        } else {
        received[index] = 1;
        to_return = 1;
       }
    } else if (seqnum == lastseqread[wire]) {
        to_return = 0;
    } else {
        lastseqread[wire] = seqnum;
        to_return = 1;
    }
    debug("sending dummy if %d > %d\n",lastseqread[wire],firstseqnotread[wire] + MAX_MISORDERED);
    if (lastseqread[wire] > (firstseqnotread[wire] + MAX_MISORDERED)) {
               debug("sending dummy\n");
               //send back a dummy packet to indicate giving up
               mc_packet dummy;
               dummy.tp.seq_num = firstseqnotread[wire];
               dummy.tp.round_num = 1;
               dummy.tp.aux = 1;
               dummy.wire = wire;
               safe_insert(&dummy);
               debug("inserted dummy\n");
               //mark the packet as received
               markreceivedround1(dummy.tp.seq_num,wire, lastseqread,firstseqnotread,start_received,received);
               return to_return;
    }
    return to_return;

}

void cleanuprow(p_tp* public_cache, int row) {
    public_cache[row].row_finished =1;
    for (int i = 0; i < numwires; i++) {
        p_tp* curr = &public_cache[row+i];
        curr->not_empty = 0;
        curr->votes = 0;
        for (int j = 0; j < numwires; j++) {
            curr->already_voted[j] = 0;
        }
    }
}




void process_public(trans_packet tp, int wire, p_tp* public_cache, int start_pc, unsigned long firstpublicnotfinished){
    mc_packet temp;
    temp.wire = -1;
    int offset = tp.seq_num - firstpublicnotfinished;
    // if it is out of range ignore it
    if(offset < 0|| offset > MAX_MISORDERED) {
       return; 
    }

    int start_index = (offset + start_pc)%MAX_MISORDERED;
    for (int i = 0; i < numwires; i++) {
        p_tp* curr = &public_cache[(start_index+i)%MAX_MISORDERED];
        // if this voting block is empty 
        if (!curr->not_empty) {
            curr->not_empty = 1;
            curr->votes = 1;
            curr->tp = tp;
            curr->already_voted[wire] =1;
            break;
        } else {
            // if the packet matches one already in the array
            if (!memcmp(&(curr->tp), &tp, sizeof(trans_packet))){
                // the wire has already voted, drop the packet
                if (curr->already_voted[wire]) {
                    return;
                }
                // cast a vote
                curr->votes++;
                curr->already_voted[wire] = 1;
                // see if we have enough votes
                if (curr->votes > T) {
                    // if we have enough votes, insert the trans_packet
                    temp.tp = tp;  
                    safe_insert(&temp);
                    cleanuprow(public_cache,start_index);
                    // if we are the firstpublicnotfinished, find a new one
                    if (firstpublicnotfinished == tp.seq_num) {
                        int j;
                        int index;
                        
                        for (j = 1; j < MAX_MISORDERED; j++) {
                            index = (start_pc + j*numwires)%MAX_MISORDERED;
                            if (!public_cache[index].row_finished){
                                public_cache[index].row_finished = 0;
                                start_pc = index;
                                break;
                            }
                            public_cache[index].row_finished = 0;
                        }
                    }
                }
                break;
            }
        }
    }
}
       
           




