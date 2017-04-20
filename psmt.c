#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include "psmt.h"
#include "params.h"
#include "fieldpoly/fieldpoly.h"
#include "fieldpoly/ff256.h"
#include "mcio.h"
#include "debug.h"

/*
int cont_free(trans_contents *given);
int pack2cont(trans_packet *given, trans_contents *result);
int cont2pack(trans_contents *given, trans_packet *result);
int find_best_pad(trans_contents conts[]);
int find_pad_conflicts(int pad, trans_contents conts[]);
uint8_t retrieve_pad(trans_contents* info, int info_n, int pad_num);
int receiver_phase1(int wire, trans_packet phase1pack) {
int receiver_phase3(trans_packet phase3pack) {
*/

/* this share_lock is used to lock all variables shared between spinning
 * instances and of spins and send_char */
pthread_mutex_t share_lock;

typedef struct
{
	trans_packet *packets;
	ff256_t *pads;
	poly_t **f;
	char secret;
	uint8_t best_pad;
	uint8_t best_pad_failed;
	uint8_t valid; /*used as count by receiver */
} history_unit;

/* history array (for both sender and receiver) holds
 * the transmitted information from phase 1 */
history_unit *history;

/* The next sequence number to use for a new transmission */
unsigned long current_seq = 0; 

void psmt_init() {
	history = calloc(HISTORY_SIZE, sizeof(history_unit));
	pthread_mutex_init(&share_lock, NULL);
}

void send_char(char secret) {

	/* spin until the buffer is clear enough for us to advance.
	 * This is a TERRBILE approach and will be fixed during
	 * multithreaded refactoring */
	pthread_mutex_lock(&share_lock);
	while(history[current_seq%HISTORY_SIZE].valid) {
		pthread_mutex_unlock(&share_lock);
		usleep(1000);
		pthread_mutex_lock(&share_lock);
	}
	pthread_mutex_unlock(&share_lock);

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
	ff256_t *pads = calloc(N*T+1, sizeof(ff256_t));
	/*the polynomials with y-intercepts corresponding to the pads */
	poly_t **f = calloc(N*T+1, sizeof(poly_t *));

	/* for each wire, the N*T+1 h polynomials and the associated checking
	 * pieces to be sent for each pad */
	trans_contents data[N];
	/* the uint8_t representation of data. This is precicely the contents
	 * of the packet send on each wire */
	trans_packet *data_pack = calloc(N, sizeof(trans_packet));

	/* This iterates over each pad */
	for (int i=0; i<N*T+1; i++) {
        /* initialize the polynomial with random values */
		f[i] = rand_poly(T, (element_t*)reference_element);
        
		/* initialize and populate the pads array */
		assign((element_t*)(pads+i), f[i]->coeffs[0]);

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
		// ssize_t n; why is this here?
		/* Iterate over each channel. For each channel, generate the checking
		 * pieces to be sent with the h's.*/
		for (int j=0; j<N; j++) {
			uint8_t h_element;
			data_pack[j].round_num = 1;
			/* iterate over the coefficients of each h poly */
			for (int k=0; k<T+1; k++) { 
				data_pack[j].h_vals[i][k] = 
					((ff256_t*)data[j].h[i]->coeffs[k])->val;
			}
			/* evaluate h at N+1 places */
			for (int k=0; k<N; k++) {
                iter_element->val = k+1;
                ff256_t *result = (ff256_t*)eval_poly(data[j].h[i], 
                		(element_t*)iter_element);
                data_pack[j].c_vals[i][k] = result->val;
			}
		}
	}

	/* set the proper round number on the packet */
	for (int i=0; i<N; i++) {
		data_pack[i].round_num = 1;
		data_pack[i].seq_num = current_seq;
	}

	/* send the data_pack's we created */
	for (int i=0; i<N; i++) {
		debug("writing packet %d\n",i);
		assert((data_pack+i)->round_num == 1);
		mc_write(data_pack+i, i);
	}
	
	/* save information for error detection in phase 2 */
	pthread_mutex_lock(&share_lock);
	history_unit *history_node = history+(current_seq%HISTORY_SIZE);
	history_node->packets = data_pack;
	history_node->pads = pads;
	history_node->f = f;
	history_node->secret = secret;
	history_node->valid = 1;
	pthread_mutex_unlock(&share_lock);

	/* advance the sequence number for the next send */
	current_seq += 1;

    /* free stuff */
    free(iter_element);
    free(reference_element);
	for (int i=0; i<N; i++) {
		cont_free(data+i, 0);
    }
}

void *send_spin(void *params) {
	for (;;) {
		/* perform public read and determine whether a pad was 
	 	 * successfully read.*/
		trans_packet phase2pack;
		int wire;
		int amount = mc_read(&phase2pack, &wire);
		if (amount == 0) {
			continue;
		}
		if (wire != -1) {
			printf("send_spin error: expected public read %d\n", wire);
			exit(1);
		}

		/* these values will be used repeatedly */
		unsigned long local_seq = phase2pack.seq_num;
		unsigned long local_seq_mod = local_seq % HISTORY_SIZE;
		
		pthread_mutex_lock(&share_lock);
		if (!history[local_seq_mod].valid) {
			pthread_mutex_unlock(&share_lock);
			/* Just drop it. This shouldn't happen often. */
            debug("dropping packet in send_spin, this shouldn't happen often\n");
		}
		else {
			pthread_mutex_unlock(&share_lock);
			trans_packet cipher_packet;
			memset(&cipher_packet, 0, sizeof(trans_packet));
			if (phase2pack.h_vals[phase2pack.aux] == 0) {
				/* finish reading other channel's records */
			 	trans_packet phase2packs[N]; 
			 	//memset(phase2packs, 0, N*(sizeof(trans2cont)));
			 	phase2packs[0] = phase2pack;
			 	for (int i=1; i<N; i++) {
			 		int wire;
			 		mc_read(phase2packs+i, &wire);
			 		/* Make sure these reads are public. If they are
			 		 * not there is a mistake in mcio. */
			 		if (wire != -1) {
						printf("send_spin error: received non-public "
						       "read when expecting public read. %d\n", wire);
						exit(1);
			 		}
			 	}
			 	for (int i=0; i<N; i++) {
			 		/* check which channels' transmissions were corrupted */
					pthread_mutex_lock(&share_lock);
			 		if (memcmp(phase2packs+i,
			 		    &(history[local_seq_mod].packets[i]), 
			 		    sizeof(trans_packet))) {

						cipher_packet.c_vals[0][i] = 1; 
			 		}
					pthread_mutex_unlock(&share_lock);
			 	}
			} 
			cipher_packet.seq_num = local_seq;
			cipher_packet.round_num = 3;
			/* NOTE: make sure history[i].secret is being interpreted as a
			 * char not an int */
			pthread_mutex_lock(&share_lock);

            int best_pad = phase2pack.aux;
			cipher_packet.aux = history[local_seq_mod].secret ^
				(history[local_seq_mod].pads[best_pad]).val;
			pthread_mutex_unlock(&share_lock);
			mc_write(&cipher_packet, -1);
            debug("phase3: secret=0x%hhx, pad=0x%hhx, aux=secret^pad=0x%hhx\n",history[local_seq_mod].secret,(history[local_seq_mod].pads[local_seq]).val,cipher_packet.aux);
		}
		pthread_mutex_lock(&share_lock);
		history[local_seq_mod].valid = 0;
		pthread_mutex_unlock(&share_lock);
		/* TODO free history block and all it's contents. Note that some of
		 * the contents are likely currently on the stack so they should be
		 * moved on to the heap. */
	}
}

int receiver_phase1(int wire, trans_packet phase1pack) {
	/* check the wire number */
	if (wire < 0 || wire >= N ) {
		printf("receiver_phase1 error: invalid wire number\n");
		exit(1);
	} 
    debug("phase1: starting receiver phase 1, sequence %d, wire %d\n", phase1pack.seq_num, wire);

	/* these values will be used repeatedly */
	unsigned long local_seq = phase1pack.seq_num;
	unsigned long local_seq_mod = local_seq % HISTORY_SIZE;
    debug("phase1: local_seq=%d, local_seq_mod=%d\n", local_seq, local_seq_mod);

    debug("phase1: locking history lock\n");
	pthread_mutex_lock(&share_lock);
	if (history[local_seq_mod].valid <= 0) {
        debug("phase1: allocating space for history[local_seq_mod].packets\n");
		history[local_seq_mod].packets = calloc(N, sizeof(trans_packet));
	}

    
	history[local_seq_mod].packets[wire] = phase1pack;
	history[local_seq_mod].valid++;
    debug("phase1: saved packet to history and incremented valid to %d\n", history[local_seq_mod].valid);

	/* wait until we have all the packets for a sequence and then
	 * process it. Recall that, at timeout, dummy packets will be
	 * returned from mcio, so we will ALWAYS receive the full N
	 * trans_packets for this sequence. */
	if (history[local_seq_mod].valid < N) {
        debug("phase1: unlocking history and returning since history[local_seq_mod].valid=%d < N=%d\n", history[local_seq_mod].valid, N); 
		pthread_mutex_unlock(&share_lock);
		return 0;
	}
    debug("phase1: unlocking history and processing the phase\n");
	pthread_mutex_unlock(&share_lock);

    debug("phase1: calling pack2cont on all packets\n");
	trans_contents contents[N];
	for (int i=0; i<N; i++) {
		pack2cont((history[local_seq_mod].packets)+i, contents+i);
	}

	int best_pad = find_best_pad(contents);
	int best_pad_failed = !!find_pad_conflicts(best_pad, contents);
    debug("phase1: best_pad=%d, best_pad_failed=%d\n", best_pad,best_pad_failed);

  
	/* store best_pad and best_pad_failed for the next stage */
	pthread_mutex_lock(&share_lock);
	history[local_seq_mod].best_pad = best_pad;
	history[local_seq_mod].best_pad_failed = best_pad_failed;
	pthread_mutex_unlock(&share_lock);

	if (best_pad_failed) {
        debug("phase1: best pad failed\n");
		/* send back censored information */
		/* build the censored packet */
		trans_packet phase1trans_censored;
		phase1trans_censored = phase1pack;
		phase1trans_censored.round_num = 2;
		phase1trans_censored.aux = best_pad;
		memset(phase1trans_censored.h_vals[best_pad], 0,
			   sizeof(uint8_t) * (T+1)); 
		memset(phase1trans_censored.c_vals[best_pad], 0,
			   sizeof(uint8_t) * (N)); 
		/* send the censored packet */
		mc_write(&phase1trans_censored, -1);
	} else {
        debug("phase1: best pad didn't fail\n");
		/* send back OK, best_pad */
		trans_packet phase2ok;
		phase2ok.seq_num = local_seq;
		phase2ok.round_num = 2;
		phase2ok.aux = best_pad;
		phase2ok.h_vals[best_pad][0] = 1;
		mc_write(&phase2ok, -1);
	} 
	return 0;
}

int receiver_phase3(trans_packet phase3pack) {

	uint8_t ciphertext = phase3pack.aux;

	/* these values will be used repeatedly */
	unsigned long local_seq = phase3pack.seq_num;
	unsigned long local_seq_mod = local_seq % HISTORY_SIZE;

	uint8_t onetimepad;
	trans_contents conts[N];

	int counter;
	pthread_mutex_lock(&share_lock);
	if (history[local_seq_mod].best_pad_failed) {
        debug("phase3: best pad failed\n");
		/* convert all of our good packets to trans_content */
		counter = 0;
		for (int i=0; i<N; i++) {
			if (! phase3pack.c_vals[0][i]) {
				pack2cont((history[local_seq_mod].packets)+i, 
						conts+counter);
				counter++;
			}
		}
	} else {
        debug("phase3: best pad didn't fail\n");
		/* convert our N packets to trans_contents */
		for (int i=0; i<N; i++) {
			pack2cont((history[local_seq_mod].packets)+i, conts+i);
		}
		counter = N;
	}
	/* do polynomial interpolation here to get the y-int of the 
	 * f polynomial */
	onetimepad = retrieve_pad(conts, counter, history[local_seq_mod].best_pad);
	pthread_mutex_unlock(&share_lock);
		
	/* At this point we have a padded message and a pad. Just recreate 
	 * the message */
	char plaintext = ciphertext ^ onetimepad;
    debug("phase3: ciphertext=0x%hhx, onetimepad=0x%hhx\n",ciphertext,onetimepad);
	printf("RECEIVED: %c\n", plaintext);

	return 0;
}

/* retrieve the pad designated by pad_num. */
uint8_t retrieve_pad(trans_contents* info, int info_n, int pad_num) {
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
			Y[i] = (ff256_t*)info[i].h[pad_num]->coeffs[0];
		}
		poly_t *f = interpolate((element_t**)X, (element_t**)Y, N);
		
		// NOTE: one time pad is always set to 0 -- is this still
		// true?
		uint8_t onetimepad = ((ff256_t*)f->coeffs[0])->val;

		free(f);
        debug("retrieve_pad: pad_num=%d, onetimepad=0x%hhx\n",pad_num, onetimepad);

		return onetimepad;
}

void *receive_spin(void *params) {
	for (;;) {
		/* read a trans_pack */
		trans_packet packet;
		int wire;
		if (!mc_read(&packet, &wire)) {
			continue;
		}

		/* handle the trans_pack differently depending on 
		 * the round_num */
		if (packet.round_num == 1) {
			receiver_phase1(wire, packet);
		} else if (packet.round_num == 3) {
			receiver_phase3(packet);
		} else {
			printf("receive_spin error: received packet with invalid "
		       	   "round number %d\n", packet.round_num);
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
		conflicts[i] = find_pad_conflicts(i, conts);	
	}
	for (int i=0; i<N*T+1; i++) {
		int otherUnion = 0;
		for (int j=0; j<N*T+1; j++) {
			if (i!=j) {
				otherUnion |= conflicts[j];
			}
		}
		if ((otherUnion & conflicts[i]) == conflicts[i]) {
			return i;
		}
	}
	return 0;
}

/* Find the wire-by-wire conflicts known by a single pad, labeled
 * pad. 
 * A set of conflicts are returned in the following manner:
 * A conflict exists on wires n and m where n < m iff
 * result & (n*N + m) > 0 */
int find_pad_conflicts(int pad, trans_contents conts[]) {
	assert(N*N <= sizeof(int)*8);
	int toReturn = 0;
	/*first index though wires*/
	for (int i=0; i<N; i++) {
		/*second index though wires*/
		for (int j=i+1; j<N; j++) {
			ff256_t x,*y;
			ff256_init(&x);
			x.val = i;
			y = (ff256_t*) eval_poly(conts[j].h[pad], (element_t*) &x);
			if (conts[i].c[pad][j]->val != y->val) {
				toReturn |= 1<<((i*N)+j);
			}
		}
	}
	return toReturn;
}

/* take the contents of given and puts them into result
 * by converting uint8_t's to poly's for h polynomials */
int pack2cont(trans_packet *given, trans_contents *result) {
	ff256_t ref;
	ff256_init(&ref);

	for (int i=0; i<N*T+1; i++) {
		result->h[i] = poly_init(T+1, (element_t*) &ref);
		for (int j=0; j<T+1; j++) 
			((ff256_t*)(result->h[i]->coeffs[j]))->val = given->h_vals[i][j];
		for (int j=0; j<N; j++) {
			ff256_t *c_ff256 = (ff256_t*) malloc(sizeof(ff256_t));
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
			result->h_vals[i][j] = ((ff256_t*)given->h[i]->coeffs[j])->val;
		for (int j=0; j<N; j++)
			result->c_vals[i][j] = given->c[i][j]->val;
	}
	return 0;
}

/* free the ff256_t's and polynomials within a trans_cont and the
 * trans_cont */
int cont_free(trans_contents *given, int freec) {
	for (int i=0; i<N*T+1; i++) {
		poly_free(given->h[i]);
		if (freec) {
			for (int j=0; j<N; j++) {
				free(given->c[i][j]);
			}
		}
	}
	return 0;
}

void validateF(poly_t **f, size_t n) {
	for (size_t i=0; i<n; i++) {
		for (int j=0; j<(f[i]->degree+1); j++) {
			assert(f[i]->coeffs[j]->size == sizeof(ff256_t));
		}
	}
}
