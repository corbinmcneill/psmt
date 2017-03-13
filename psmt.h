#ifndef PSMT_H
#define PSMT_H

#include <stddef.h>

#define N 5
#define T 2
#define SEC_LEN 100

int send_info(char *secret, size_t secret_n, int *rfds, int *wfds, size_t fds_n);

int receive_info(int *rfds, int *wfds, size_t fds_n);

#endif
