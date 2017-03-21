#ifndef PSMT_H
#define PSMT_H

#include <stddef.h>

#define N 5
#define T 2
#define SEC_LEN 100

typedef struct 
{
	uint8_t h_vals[N*T+1][T+1];
	uint8_t c_vals[N*T+1][N];
}__attribute__((packed))__ trans_packet;

typedef struct 
{
	poly_t *h[N*T+1];
	uint8_t c[N*T+1][N];
} trans_contents;

int send_info(char *secret, size_t secret_n, int *rfds, int *wfds, size_t fds_n);

int receive_info(int *rfds, int *wfds, size_t fds_n);

#endif
