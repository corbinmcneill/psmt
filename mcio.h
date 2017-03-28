#include "psmt.h"

/* init_io -
 *
 * Initialize the io functions.
 *
 * fds - An array containing the file descriptors to use
 *
 * n - the number of file desicriptors in the array.
 *
 * Returns 0 on success and -1 on failure
 */
int init_mcio(int* fds, int n);



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
 * returns 0 on success and -1 on error
 */

int mc_read(trans_packet* data, int wire);

/*
 * write -
 * Sends a trans_packet over the specified wire. 
 *
 * trans_packet - the packet to send
 *
 * wire - the wire number to send it on. The public channel is wire -1.
 *
 */
int mc_write(trans_packet* data, int wire);
