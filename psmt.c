#include <stdio.h>
#include <unistd.h>
#include <stddef.h>

#include "psmt.h"
#include "poly.h"

int send_info(char *secret, size_t secret_n, int *fds, size_t fds_n) {
	//NOTE: Input for this algorithm should be a fifo. This would well 
	//emulate input from a socket which would allow for long-term transparent
	//approach for network rerouting. Also this should be a while loop.

	/* PHASE 1 */
	uint8_t pads[N*T];
	uint8_t *f[N];
	uint8_t *h[N*T][N];

	size_t j;
	int i,k;
	// This iterates over each pad
	for (i=0; i<N*T; i++) {
		f[i] = make_poly(T);
		pads[i] = f[i][0];
		// This iterates over each channel
		for (j=0; j<N; j++) {
			h[i][j] = make_poly_intercept(T, eval_poly(f[i], T, j));
		}
		//each channel
		for (j=0; j<N; j++) {
			int k;
			for (k=0; k<T+1; k++) {
				char *h_element = sstring("%u",h[i][j][k]);
				write(fds[j], h_element, strnlen(h_element, STRING_LEN));
			}

			for (k=0; k<N; k++) {
				char *c_item = sstring("%u",eval_poly(h[i][j],T,k));
				write(fds[j], h_element, strnlen(h_element, STRING_LEN));
			}
		}
	}
	/* PHASE 2 */
	conflict_response = [N][2][SEC_LEN];
	for (j=N-1; j>=0; j++) {
		if (read(0, conflict_response[j], SEC_LEN) <0) {
			printf("conflict_response read failure\n");
			fflush();
		}
	}

	/* PHASE 3 */
	char *write_element;
	if (conflict_response[0][0] == 'S') {
		for (j=0; j<N; j++) {
			write_element = sstring("%u",secret[0] ^ pads[conflict_response[0][1]]);
			write(fds[j], write_element, strnlen(write_element, SEC_LEN));
		}
	} else if (conflict_response[0][0] == 'F') {
		//we ignore this *important* case for now
	} else {
		printf("conflict_response byte is not valid");
	}
	return 0;
}

int receive_info(int *fds, size_t fds_n) {
	//NOTE: this entire algorithm needs to run on a while loop to keep
	//reading input. Shouldn't this have a fifo as output. That would 
	//well emulate outputting everything to a socket for a more
	//transparent implementation.
	
	int i,k;
	size_t j;

	//indexed by pad, then channel 
	uint8_t h[N*T+1][N][T+1]; //T+1 coefficients of polynomial
	uint8_t c[N*T+1][N][N];   //N checking pieces

	/* Phase 1 */
	char read_element[SEC_LEN];
	char write_element[SEC_LEN];
	//cycle through the pads
	for (i=0; i<N*T+1; i++) {
		//read all the transmitted information from each channel
		
		//NOTE: this currently assumes that all information is 
		//delivered perfectly. This assumption can and will be 
		//relaxed later.
		for (j=0; j<N; j++) {
			//read coefficients
			for (k=0; k<T+1; k++) {
				memset(read_element, 0, SEC_LEN);
				ssize_t n = read(j, read_element, SEC_LEN); 
				h[i][j][k] = strtol(read_element, NULL, 10);
			}
			//read checking pieces
			for (k=0; k<N; k++) {
				memset(read_element, 0, SEC_LEN);
				ssize_t n = read(j, read_element, SEC_LEN); 
				c[i][j][k] = strtol(read_element, NULL, 10);
			}
		}
	}

	/* Phase 2 */
	//NOTE: In the future we will remove inconcistencies, 
	//identify conflicts, and pick the best pad here.
	unsigned int best_pad = 0;
	unsigned int best_pad_failed=0;
	
	if (best_pad_failed) {
		//send lots of error correction info publically
	} else {
		for (j=0; j<N; j++) {
			write_element = sstring("S%u",best_pad);
			write(fds[j], write_element, strnlen(write_element, SEC_LEN));
		}
	}

	/* Phase 3 */
	//NOTE: in the future this secret message will need a majority vote from
	//all channels
	//NOTE: we need to read all of the channels or else we'll corrupt our
	//subsequent messages. 
	char ciphertext[SEC_LEN];
	for (j=N-1; j>=0; j++) {
		if (read(0, ciphertext, SEC_LEN) <0) {
			printf("ciphertext read failure\n");
			fflush();
		}
	}

	char onetimepad;
	if (best_pad_failed) {
		//collect and interperet fault info that was sent back
		//we'll skip this for now
	} else {
		//do polynomial interpolation here to get the y-int of the 
		//f polynomial
		onetimepad = 0;
	}
	//At this point we have a padded message and a pad. Just recreate 
	//the message
	char plaintext = ciphertext[0] ^ onetimepad;
	printf("%c", plaintext);

	return 0;
}
