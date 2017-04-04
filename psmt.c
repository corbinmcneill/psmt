#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <time.h>

#include "psmt.h"
#include "params.h"
#include "fieldpoly/fieldpoly.h"
#include "fieldpoly/ff256.h"

typedef struct
{
	trans_packet *packets[N];
	ff256_t *pads[N*T+1];
	poly_t *f[N*T+1];
	char secret;
	uint8_t valid; /*used as count by receiver */
} history_unit;

history_unit *history;

/* The next sequence number to use for a new transmission */
unsigned long current_seq = 0; 

void psmt_init() {
	history = calloc(HISTORY_SIZE, sizeof history_unit);
}

int send_char(char secret) {

	/* spin until the buffer is clear enough for us to advance.
	 * This is a TERRBILE approach and will be fixed during
	 * multithreaded refactoring */
	while(history[current_seq%HISTORY_SIZE].valid) {
		usleep(1000);
	}

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
	ff256_t *pads = calloc(N*T+1, sizeof ff256_t);
	/*the polynomials with y-intercepts corresponding to the pads */
	poly_t *f = calloc(N*T+1, sizeof poly_t);

	/* for each wire, the N*T+1 h polynomials and the associated checking
	 * pieces to be sent for each pad */
	trans_contents data[N];
	/* the uint8_t representation of data. This is precicely the contents
	 * of the packet send on each wire */
	trans_packet *data_pack = calloc(N, sizeof trans_packet);

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
			data_pack[j]->round_num = 1;
			/* iterate over the coefficients of each h poly */
			for (int k=0; k<T+1; k++) { 
				data_pack[j]->h_vals[i][k] = 
					((ff256_t*)data[j].h[i]->coeffs[k])->val;
			}
			/* evaluate h at N+1 places */
			for (int k=1; k<N+1; k++) {
                iter_element->val = k;
                data_pack[j]->c_vals[i][k] = 
                	(ff256_t*)eval_poly(h[i][j], (element_t*)iter_element);
			}
		}
	}
	/* set the proper round number on the packet */
	for (int i=0; i<N; i++) {
		data_pack[i]->round_num = 1;
	}

	/* send the data_pack's we created */
	for (int i=0; i<N; i++) {
		if (mc_write(data_pack[i], i) < 0) {
			printf("error: mc_write\n");
		}
	}
	
	/* save information for error detection in phase 2 */
	history_unit *history_node = history+(current_seq%HISTORY_SIZE);
	history_node->packets = data_pack;
	history_node->pads = pads;
	history_node->f = f;
	history_node->secret = secret;
	history_node->valid = 1;

	/* advance the sequence number for the next send */
	current_seq += 1;

    /* free stuff */
    free(iter_element);
    free(reference_element);
	for (int i=0; i<N; i++) {
		cont_free(phase1cont[i]);
    }

    return 0;
}

int send_spin() {
	for (;;) {
		/* perform public read and determine whether a pad was 
	 	 * successfully read.*/
		trans_packet phase2pack;
		mc_read(*phase2pack, -1);

		/* these values will be used repeatedly */
		unsigned long local_seq = phase2pack.seq_num;
		unsigned long local_seq_mod = local_seq % HISTORY_SIZE;
		
		if (history[local_seq_mod]->valid) {
			/* Just drop it. This shouldn't happen often. */
		}
		else {
			trans_packet cipher_packet;
			memset(&cipher_packet, 0, sizeof trans_packet);
			if (phase2pack.h_vals[phase2pack.aux] == 0) {
				/* finish reading other channel's records */
			 	trans_packet phase2packs[N]; 
			 	memset(phase2packs, 0, N*(sizeof trans2cont));
			 	phase2packs[0] = phase2pack;
			 	for (int i=1; i<N i++) {
			 		int wire;
			 		mc_read(phase2packs+i, &wire, 2);
			 		/* Make sure these reads are public. If they are
			 		 * not there is a mistake in mcio. */
			 		if (wire != -1) {
						printf("send_spin error: received non-public "
						       "read when expecting public read.\n" 
						exit(1);
			 		}
			 	}
			 	for (int i=0; i<N i++) {
			 		/* check which channels' transmissions were corrupted */
			 		if (memcmp(phase2packs[i],
			 		    *history[local_seq_mod].packets[i], 
			 		    sizeof trans_packet)) {

						cipher_packet.c_vals[0][i] = 1; 
			 		}
			 	}
			} 
			cipher_packet.seq_num = local_seq;
			cipher_packet.round_num = 3;
			cipher_packet.aux = history[local_seq_mod]->secret ^
				history[local_seq_mod]->pads[local_seq];
			mc_write(cipher_packet, -1);
		}
		history[local_seq_mod]->valid = 0;
		/* TODO free this history block and all it's parts here */
	}
}

int receiver_phase1(int wire, trans_packet phase1pack) {
	/* check the wire number */
	if (wire < 0 || wire >= N ) {
		printf("receiver_phase1 error: invalid wire number\n");
		exit(1);
	} 

	/* these values will be used repeatedly */
	unsigned long local_seq = phase2pack.seq_num;
	unsigned long local_seq_mod = local_seq % HISTORY_SIZE;

	if (! history[local_seq_mod].valid) {
		memset(history[local_seq_mod],0,sizeof history_unit); 
	}

	history[local_seq_mod].packets[wire] = phase1pack;
	history[local_seq_mod].valid++;

	/* wait until we have all the packets for a sequence and then
	 * process it. Recall that, at timeout, dummy packets will be
	 * returned from mcio, so we will ALWAYS receive the full N
	 * trans_packets for this sequence. */
	if (history[local_seq_mod].valid == N) {
		trans_packet contents[N];
		for (int i=0; i<N; i++) {
			pack2cont(history[local_seq_mod].packets[i], contents+i);
		}
	}

	int best_pad = find_best_pad(contents);
	int best_pad_failed = !!find_pad_conflicts(best_pad, contents);

	if (best_pad_failed) {
		/* send back censored information */
	} 
	else {
		
	}
	
	if (best_pad_failed) {
		trans_contents phase1trans_censored[N];
		for (int i=0; i<N; i++0) {
			phase1trans_censored[i] = phase1trans[i];
			phase1trans_censored[i].round_num = 2;
			phase1trans_censored[i].aux = best_pad;
			memset(phase1trans_censored.h_vals[best_pad], 0,
			       sizeof(uint8_t) * (T+1)); 
			memset(phase1trans_censored.c_vals[best_pad], 0,
			       sizeof(uint8_t) * (N)); 
		}
		//TODO left off here
	} else {

	} 

}
int receiver_phase3(trans_packet phase3pack) {
	trans_packet phase3pack;
	mc_read(*phase3pack, -1); /* public read the trans_pack */

	uint8_t ciphertext = phase3pack.aux;

	uint8_t onetimepad;
	poly_t *f;
	if (best_pad_failed) {
		//collect and interperet fault info that was sent back
		//we'll skip this for now
	} else {
		/* do polynomial interpolation here to get the y-int of the 
		 * f polynomial */
		
		/* create the zero element of ff256 */
		ff256_t zero;
		ff256_init(&zero);
		ff256_add_id((element_t*)&zero);

		/* declare the X's and Y's for polynomial interpolation */
		ff256_t *X[N];
		ff256_t *Y[N];
		
		for (int i=0; i<N; i++) {
		X[i] = malloc(sizeof(ff256_t));
			ff256_init(X[i]);
			ff256_set(i+1, X[i]);
			Y[i] = (ff256_t*)h[best_pad][i]->coeffs[0];
		}
		f = interpolate((element_t**)X, (element_t**)Y, N);
		// NOTE: one time pad is always set to 0 -- is this still
		// true?
		onetimepad = ((ff256_t*)f->coeffs[0])->val;
	}
	/* At this point we have a padded message and a pad. Just recreate 
	 * the message */
	char plaintext = ciphertext ^ onetimepad;
	printf("%c", plaintext);
	poly_free(f);
}

int receive_spin() {
	for (;;) {
		/* read a trans_pack */
		trans_packet packet;
		int wire;
		mc_read(*packet, &wire); 

		/* handle the trans_pack differently depending on 
		 * the round_num */
		if (packet.round_num == 1) {
			receiver_phase1(wire, packet);
		} else if (packet.round_num == 3) {
			receiver_phase3(wire, packet);
		} else {
			printf("receive_spin error: received packet with invalid"
		       	   "round number\n");
			exit(1);
		}
	}
}

/* find the pad whose wire conflicts are subset of the wire conflicts
 * on all the other pads. Such a pad is guaranteed to exist by a 
 * pidgeon hole argument. See PSMT p.41 */
/* NOTE: this may require optimization*/
int find_best_pad(trans_contents conts[]) {
	int conflicts[N*T+1];
	for (int i=0; i<N*T+1; i++) {
		conflict[i] = find_pad_conflicts(i, conts);	
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
int find_pad_conflicts(int pad, trans_contents conts[]) {
	assert(N*N <= sizeof int)
	int toReturn = 0;
	/*first index though wires*/
	for (int i=0; i<N; i++) {
		/*second index though wires*/
		for (int j=i+1; j<N; j++) {
			ff256_t x,*y;
			x.val = i;
			y = poly_eval(conts[j].h[pad].val, x);
			if (conts[i].c[pad][j].val != y->val) {
				toReturn |= 1<<((i*N)+j);
			}
		}
	}
	return toReturn;
}

/* take the contents of given and puts them into result
 * by converting uint8_t's to poly's for h polynomials */
int pack2cont(trans_packet *given, trans_contents *result) {
	ff256_t ref = ff256_init();
	for (int i=0; i<N*T+1; i++) {
		result->h[i] = poly_init(T+1, &ref);
		for (int j=0; j<T+1; j++) 
			result->h[i]->coeffs[j] = given->h_vals[i][j];
		for (int j=0; j<N; j++) {
			ff256_t *c_ff256 = (ff256_t*) malloc(sizeof ff256_t);
			ff256_init(c_ff256);
			c_ff256->val = given->c_vals[i][j];
			result->c[i][j] = c_ff256;
		}
	}
	return 0;
}

/* take the contents of given and puts them into result
 * by converting everything to uint8_t's */
int cont2pack(trans_contents *given, trans_packet *result) {
	for (int i=0; i<N*T+1; i++) {
		for (int j=0; j<T+1; j++)
			result->h_vals[i][j] = given->h[i]->coeffs[j]->val;
		for (int j=0; j<N; j++)
			result->c_vals[i][j] = given->c[i][j]->val;
	}
}

/* free the ff256_t's and polynomials within a trans_cont and the
 * trans_cont */
int cont_free(trans_contents *given) {
	for (int i=0; i<N*T+1; i++) {
		poly_free(given->c[i]);
		for (int j=0; j<N; j++) {
			free(given->c[i][j]);
		}
	}
	free(given);
}

void validateF(poly_t **f, size_t n) {
	for (size_t i=0; i<n; i++) {
		for (int j=0; j<(f[i]->degree+1); j++) {
			assert(f[i]->coeffs[j]->size == sizeof(ff256_t));
		}
	}
}
