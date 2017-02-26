#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>

#include "psmt.h"
#include "fieldpoly/fieldpoly.h"
#include "fieldpoly/ff256.h"

int send_info(char *secret, size_t secret_n, int *fds, size_t fds_n) {
	//NOTE: Input for this algorithm should be a fifo. This would well 
	//emulate input from a socket which would allow for long-term transparent
	//approach for network rerouting. Also this should be a while loop.

	/* PHASE 1 */
	ff256_t *pads[N*T+1];
	poly_t *f[N];
	poly_t *h[N*T+1][N];
    ff256_t* reference_element = malloc(sizeof(ff256_t));
    ff256_t* iter_element = malloc(sizeof(ff256_t));
    ff256_init(reference_element);
    ff256_init(iter_element);
    ff256_t* eval_element;
	size_t j;
	// This iterates over each pad
	for (int i=0; i<N*T+1; i++) {
        // initialize the polynomial with random values
		f[i] = rand_poly(T,(element_t*) reference_element);
        
		pads[i] = malloc(sizeof(ff256_t)); //NOTE: free this
		assign((element_t*)pads[i],f[i]->coeffs[0]);
		// This iterates over each channel

		for (int j=0; j<N; j++) {
            iter_element->val = j;
            // stare at this later to make sure it's ok
            eval_element  = (ff256_t*) eval_poly(f[i],(element_t*) iter_element);
			h[i][j] = rand_poly_intercept(T, (element_t*) eval_element);
            free(eval_element);
		}
		//each channel
		for (int j=0; j<N; j++) {
			char h_element[SEC_LEN];
			for (int k=0; k<T+1; k++) {
				if (snprintf(h_element,SEC_LEN,"%u",((ff256_t*)h[i][j]->coeffs[k])->val)<=0) {
					printf("parsing error, h_element");
				}
				write(fds[j], h_element, strnlen(h_element, SEC_LEN));
			}
			char c_item[SEC_LEN];
			for (int k=0; k<N; k++) {
                iter_element->val = k;
                eval_element = (ff256_t*)eval_poly(h[i][j],(element_t*)iter_element);
				if (snprintf(c_item,SEC_LEN,"%u",eval_element->val)<=0) {
					printf("parsing error, c_item");
				}
				write(fds[j], c_item, strnlen(c_item, SEC_LEN));
                free(eval_element);
			}
		}
	}
	/* PHASE 2 */
	char conflict_response[N][SEC_LEN];
	for (j=0; j<N; j++) {
		if (read(0, conflict_response[j], SEC_LEN) <0) {
			printf("conflict_response read failure\n");
			fflush(stdout);
		}
	}

	/* PHASE 3 */
	char write_element[SEC_LEN];
	if (conflict_response[0][0] == 'S') {
		for (j=0; j<N; j++) {
			snprintf(write_element, SEC_LEN, "%c",secret[0] ^ pads[(int)conflict_response[0][1]]->val);
			write(fds[j], write_element, strnlen(write_element, SEC_LEN));
		}
	} else if (conflict_response[0][0] == 'F') {
		//we ignore this *important* case for now
	} else {
		printf("conflict_response byte is not valid");
	}

    // free stuff
    free(iter_element);
    free(reference_element);
	for (int i=0; i<N*T+1; i++) {
        poly_free(f[i]);
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
	poly_t* h[N*T+1][N]; //T+1 coefficients of polynomial
	ff256_t* c[N*T+1][N][N];   //N checking pieces

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
				((ff256_t*) h[i][j]->coeffs[k])->val = strtol(read_element, NULL, 10);
			}
			//read checking pieces
			for (k=0; k<N; k++) {
				memset(read_element, 0, SEC_LEN);
				ssize_t n = read(j, read_element, SEC_LEN); 
				((ff256_t*) c[i][j][k])->val = strtol(read_element, NULL, 10);
			}
		}
	}

	/* Phase 2 */
	//NOTE: In the future we will remove inconcistencies, 
	//identify conflicts, and pick the best pad here.
	unsigned int best_pad = 0;
	unsigned int best_pad_failed=0;
	
	if (best_pad_failed) {
		printf("best_pad failed");
		//send lots of error correction info publically
	} else {
		for (j=0; j<N; j++) {
			snprintf(write_element, SEC_LEN,"S%u",best_pad);
			write(fds[j], write_element, strnlen(write_element, SEC_LEN));
		}
	}

	/* Phase 3 */
	//NOTE: in the future this secret message will need a majority vote from
	//all channels
	//NOTE: we need to read all of the channels or else we'll corrupt our
	//subsequent messages. 
	char ciphertext[SEC_LEN];
	for (j=0; j<=N; j++) {
		if (read(j, ciphertext, SEC_LEN) <0) {
			printf("ciphertext read failure\n");
			fflush(stdout);
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
