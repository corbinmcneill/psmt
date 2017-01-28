#include <stdio.h>
#include <unistd.h>
#include <stddef.h>

#include "psmt.h"
#include "poly.h"

int send_info(char *secret, size_t secret_n, int *fds, size_t fds_n) {

	/* PHASE 1 */
	uint8_t pads[N*T];
	uint8_t *f[N];
	uint8_t *h[N][N*T];

	int i,j;
	for (i=0; i<N*T; i++) {
		f[i] = make_poly(T);
		pads[i] = f[i][0];
		for (j=0; j<N; j++) {
			h[i][j] = make_poly_intercept(T, eval_poly(f[i], T, j));
		}
	}
	
	/* PHASE 2 */

	/* PHASE 3 */

	return 0;
}

int receive_info(int *fds, size_t fds_n) {
	size_t i;
	/* Phase 1 */
	char output[SEC_LEN];
	for (i=0; i<fds_n; i++) {
		read(fds[i], output, SEC_LEN);
		printf(output);
	}

	/* Phase 2 */

	/* Phase 3 */

	return 0;
}
