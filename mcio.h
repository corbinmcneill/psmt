#ifndef MCIO_H
#define MCIO_H
#include "psmt.h"


typedef struct {
    trans_packet tp;
    int wire;
} mc_packet;


/* init_io -
 *
 * Initialize the io functions.
 *
 * wfds - An array containing the write file descriptors to use
 *
 * rfds - An array containing the read file descriptors to use
 *
 * n - the number of file desicriptors in the arrays.
 *
 * Returns 0 on success and -1 on failure
 */
int mc_init(int* fds, int* wfds, int n);



/*
 * read -
 * Returns a recieved trans_packet. If multiple packets have been received it
 * returns the packet recieved with the lowest sequence number and lowest round
 * number. If no packets have been recieved it blocks until a packet is
 * recieved. The public channel is treated as wire -1. 
 * 
 *
 * trans_packet - pointer to the place where read should store the recieved
 *                trans_packet
 * wire - the wire number the trans_packet was recieved on. 
 *
 * returns 1 on read, 0 on no data, -1 on error,and 4 on end of transmission
 */

int mc_read(trans_packet* data, int* wire);

unsigned int mc_fpnf();
/*
 * write -
 * Sends a trans_packet over the specified wire. 
 *
 * trans_packet - the packet to send
 *
 * wire - the wire number to send it on. The public channel is wire -1.
 *
 * 1 on success, -1 on error.
 *
 */
int mc_write(trans_packet* data, int wire);

/* cleans up 
 *
 * Returns 0 on success and -1 on failure
 * 
 */

int mc_destroy();

#endif
