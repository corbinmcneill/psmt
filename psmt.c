#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>

#include "psmt.h"
#include "fieldpoly/fieldpoly.h"
#include "fieldpoly/ff256.h"

void validateF(poly_t **f, size_t n) {
	for (size_t i=0; i<n; i++) {
		for (int j=0; j<(f[i]->degree+1); j++) {
			assert(f[i]->coeffs[j]->size == sizeof(ff256_t));
		}
	}
}

int send_info(char *secret, size_t secret_n, int *rfds, int *wfds, size_t fds_n) {
	//NOTE: Input for this algorithm should be a fifo. This would well 
	//emulate input from a socket which would allow for long-term transparent
	//approach for network rerouting. Also this should be a while loop.

	/* PHASE 1 */

    /* ff256 evaluation of a polynomial */
    ff256_t* eval_element;

    /* an blank example of an initialized ff256 element for the fieldpoly 
     * library */
    ff256_t* reference_element = malloc(sizeof(ff256_t));
    ff256_init(reference_element);

    /* element used for evaluation of h's to generate checking pieces */
    ff256_t* iter_element = malloc(sizeof(ff256_t));
    ff256_init(iter_element);

	/*the pads generated for a single psmt iteration */
	ff256_t *pads[N*T+1];
	/*the polynomials with y-intercepts corresponding to the pads */
	poly_t *f[N*T+1];

	/*for each wire, the N*T+1 h polynomials and the associated checking
	 * pieces to be sent for each pad */
	trans_contents data[N];
	/* the uint8_t representation of data. This is precicely the contents
	 * of the packet send on each wire */
	trans_packet data_pack[N];

	/* This iterates over each pad */
	for (int i=0; i<N*T+1; i++) {
        /* initialize the polynomial with random values */
		f[i] = rand_poly(T, (element_t*)reference_element);
        
		/* initialize and populate the pads array */
		pads[i] = malloc(sizeof(ff256_t)); //NOTE: free this
		assign((element_t*)pads[i],f[i]->coeffs[0]);

		/* Iterate over each channel. For each channel, use the evaluation 
		 * of f at a unique point to designate the y-intercept of that h poly.*/
		for (int j=0; j<N; j++) {
            /* pick x */
            iter_element->val = j+1;
            /* calculate y */
            eval_element = (ff256_t*) eval_poly(f[i],(element_t*) iter_element);
			data[j].h[i] = rand_poly_intercept(T, (element_t*) eval_element);

            free(eval_element);
		}
		ssize_t n;
		/* Iterate over each channel. For each channel, generate the checking
		 * pieces to be sent with the h's.*/
		for (int j=0; j<N; j++) {
			uint8_t h_element;
			/* iterate over the coefficients of each h poly */
			for (int k=0; k<T+1; k++) { 
				data_pack[j].h_vals[i][k] = ((ff256_t*)data[j].h[i]->coeffs[k])->val;
			}
			/* evaluate h at N+1 places */
			for (int k=1; k<N+1; k++) {
                iter_element->val = k;
                eval_element = (ff256_t*)eval_poly(h[i][j],(element_t*)iter_element);
				if (write(wfds[j], &eval_element, 1)< 0) {
					printf("psmt.c check write error: %s", strerror(errno));
				}
				//printf("write: pad %d, channel %d, check %d\n", i, j, k);
                free(eval_element);
			}
		}
	}

	/* PHASE 2 */
	validateF(f, N);
	
	//read conflict responses. currently this assumes success on all wires
	char conflict_response[N][2];
	for (j=0; j<N; j++) {
		if (read(rfds[j], &conflict_response[j][0], 1) <0) {
			printf("conflict_response read failure\n");
		}
		if (read(rfds[j], &conflict_response[j][1], 1) <0) {
			printf("conflict_response read failure\n");
		}
	}

	/* PHASE 3 */
	validateF(f, N);
	if (conflict_response[0][0] == 'S') {
		for (j=0; j<N; j++) {
			char towrite = (char)(((uint8_t)secret[0])^pads[(int)conflict_response[0][1]]->val);
			write(wfds[j], &towrite, 1);
		}
	} else if (conflict_response[0][0] == 'F') {
		//we ignore this *important* case for now
	} else {
		printf("conflict_response byte is not valid");
	}
	validateF(f, N);

    // free stuff
    free(iter_element);
    free(reference_element);
	for (int i=0; i<N*T+1; i++) {
        poly_free(f[i]);
    }
	return 0;
}

int receive_info(int *rfds, int *wfds, size_t fds_n) {
	//NOTE: this entire algorithm needs to run on a while loop to keep
	//reading input. Shouldn't this have a fifo as output. That would 
	//well emulate outputting everything to a socket for a more
	//transparent implementation.
	
	int i,k;
	size_t j;

	//indexed by pad, then channel 
	poly_t* h[N*T+1][N];    //T+1 coefficients of polynomial
	ff256_t c[N*T+1][N][N]; //N checking pieces

	ff256_t blankItem;
	ff256_init(&blankItem);

	/* Phase 1 */
	char read_element;
	char write_element;
	//cycle through the pads
	for (i=0; i<N*T+1; i++) {
		//read all the transmitted information from each channel
		
		
		//NOTE: this currently assumes that all information is 
		//delivered perfectly. This assumption can and will be 
		//relaxed later.
		for (j=0; j<N; j++) {
			//initialize each 
			h[i][j] = poly_init(T, (element_t*) &blankItem);
			
			//read coefficients
			for (k=0; k<T+1; k++) {
				if (read(rfds[j], &read_element, 1) < 0) {
					printf("psmt.c coef read error: %s", strerror(errno));
				}
				//printf("read:  pad %d, channel %d, coef %d\n", i, (int)j, k);
				((ff256_t*) h[i][j]->coeffs[k])->val = (uint8_t) read_element;
			}
			//read checking pieces
			for (k=0; k<N; k++) {
				//initialize each element in c
				ff256_init(&c[i][j][k]);
				
				if ( read(rfds[j], &read_element, 1) < 0) {
					printf("psmt.c check read error: %s", strerror(errno));
				}
				//printf("read:  pad %d, channel %d, check %d\n", i, (int)j, k);
				c[i][j][k].val = (uint8_t) read_element;
			}
		}
	}

	/* Phase 2 */
	unsigned int best_pad = find_best_pad(c,h);
	unsigned int best_pad_failed = !!find_pad_conflicts(best_pad, c, h);
	
	if (best_pad_failed) {
		for (int i=0; i<N*T+1; i++) {

		
		printf("best_pad failed");
		//send lots of error correction info publically
	} else {
		for (j=0; j<N; j++) {
			if (write(wfds[j], "S", 1) < 0) {
				printf("send failed");
			}
			if (write(wfds[j], &write_element, 1) < 0) {
				printf("send failed");
			}
		}
	}

	/* Phase 3 */
	//NOTE: in the future this secret message will need a majority vote from
	//all channels
	//NOTE: we need to read all of the channels or else we'll corrupt our
	//subsequent messages. 
	char cipherchar;
	for (j=0; j<N; j++) {
		if (read(rfds[j], &cipherchar, 1) < 0) {
			printf("ciphertext read failure: %s\n", strerror(errno));
			exit(1);
		}
	}
	uint8_t ciphertext = (uint8_t) cipherchar;

	uint8_t onetimepad;
	poly_t *f;
	if (best_pad_failed) {
		//collect and interperet fault info that was sent back
		//we'll skip this for now
	} else {
		//do polynomial interpolation here to get the y-int of the 
		//f polynomial
		ff256_t zero;
		ff256_init(&zero);
		ff256_add_id((element_t*)&zero);

		ff256_t *X[N];
		ff256_t *Y[N];
		
		ff256_t y;
		for (int i=0; i<N; i++) {
		X[i] = malloc(sizeof(ff256_t));
			ff256_init(X[i]);
			ff256_set(i+1, X[i]);
			Y[i] = (ff256_t*)h[best_pad][i]->coeffs[0];
		}
		f = interpolate((element_t**)X, (element_t**)Y, N);
		//one time pad is always set to 0
		onetimepad = ((ff256_t*)f->coeffs[0])->val;
	}
	//At this point we have a padded message and a pad. Just recreate 
	//the message
	char plaintext = ciphertext ^ onetimepad;
	printf("%c", plaintext);
	poly_free(f);

	return 0;
}

/* find the pad whose wire conflicts are subset of the wire conflicts
 * on all the other pads. Such a pad is guaranteed to exist by a 
 * pidgeon hole argument. See PSMT p.41 */
/* NOTE: this may require optimization*/
int find_best_pad(poly_t *c[][][],ff256_t *h[][]) {
	int conflicts[N*T+1];
	for (int i=0; i<N*T+1; i++) {
		conflict[i] = find_pad_conflicts(i, c, h);	
	}
	for (int i=0; i<N*T+1; i++) {
		int otherUnion = 0;
		for (int j=0; j<N*T+1; j++) {
			if (i!=j) {
				otherUnion |= conflicts[j];
			}
		}
		if (otherUnion & conflict[i] == conflict[i]) {
			return i;
		}
	}
}

/* Find the wire-by-wire conflicts known by a single pad, labeled
 * pad. 
 * A set of conflicts are returned in the following manner:
 * A conflict exists on wires n and m where n < m iff
 * result & (n*N + m) > 0 */
int find_pad_conflicts(int pad, poly_t *c[][][],ff256_t *h[][]) {
	assert(N*N <= sizeof int)
	int toReturn = 0;
	for (int i=0; i<N; i++) {
		for (int j=i+1; j<N; j++) {
			ff256_t x,*y;
			x.val = i;
			y = poly_eval(h[pad][j], x);
			if (c[pad][i][j].val != y->val) {
				toReturn |= 1<<((i*N)+j);
			}
		}
	}
	return toReturn;
}

int pack2cont(trans_packet given, trans_contents result) {
	ff256_t ref = ff256_init();
	for (int i=0; i<N*T+1; i++) {
		result.h[i] = poly_init(T+1, &ref);
		for (int j=0; j<T+1; j++) 
			result.h[i]->coeffs[j] = given.h_vals[i][j];
		for (int j=0; j<N; j++) {
			ff256_t c_ff256 = ff256_init();
			c_ff256.val = given.c_vals[i][j];
			result.c[i][j] = c_ff256;
		}
	}
	return 0;
}

int cont2pack(trans_contents given, trans_packet result) {
	for (int i=0; i<N*T+1; i++) {
		for (int j=0; j<T+1; j++)
			result.h[i][j] = given.h_vals[

	b.
