#ifndef PSMT_H
#define PSMT_H

#include <stddef.h>

#define N 5

int send_info(char *secret, size_t secret_n, int *fds, size_t fds_n);

int receive_info(int *fds, size_t fds_n);

#endif
