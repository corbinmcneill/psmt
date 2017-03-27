#ifndef PSMT_H
#define PSMT_H

#include <stddef.h>

#define N 5
#define T 2
#define SEC_LEN 100

/* this holds all the polynomials and checking pieces for 
 * a round of communication on a single wire */
typedef struct 
{
	poly_t  *h[N*T+1];
	ff256_t *c[N*T+1][N];
} trans_contents;

/* this holds the uint8_t representation of a trans_contents.
 * Specifically it is exactly what is sent on a wire for a single
 * round that transmits h poly's and checking pieces */
typedef struct 
{
	unsigned long seq_num;
	uint8_t missing_pad; /* this pad is missing in transmissions
							after round 1 */
	uint8_t round_num;
	uint8_t h_vals[N*T+1][T+1];
	uint8_t c_vals[N*T+1][N];
}__attribute__((packed))__ trans_packet;


/* send a secret
 * secret is the secret string
 * secret_n is the length of the secret
 * rfds are the file descriptors used to read messages from the receiver
 * wfds are the file descriptors used to send messages to the receiver
 * fds_n is the number of rfds. this is also the number of wfds */
int send_info(char *secret, size_t secret_n, int *rfds, int *wfds, size_t fds_n);

/* receive a secret
 * rfds are the file descriptors used to read messages from the receiver
 * wfds are the file descriptors used to send messages to the receiver
 * fds_n is the number of rfds. this is also the number of wfds */
int receive_info(int *rfds, int *wfds, size_t fds_n);

#endif
