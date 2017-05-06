#ifndef PARAMS_H

#define PARAMS_H

#define N 5					/* the number of wires */
#define T 2					/* smallest integer strictly greater than N/2 */
#define HISTORY_SIZE 502768	/* the size of the psmt sequence buffer */
#define TIMEOUT 50000		/* timeout in milliseconds */
#define FNAME_LEN 50		/* the max filename length */
#define FIRST_SEQ 0         /* the first sequence # */
#define MAX_BUF 60000
#define MAX_THREADS 8
// the maximum number of out of order packets a good wire can send
#define MAX_MISORDERED 4000

// how big can a message be. (This only exists until I have time to implement a dynamic array)
#define MAX_MESSAGE 5000000
#define END_TRANSMISSION 4

#endif
