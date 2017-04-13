#ifndef PSMT_H
#define PSMT_H

#include <stddef.h>
#include "params.h"
#include "fieldpoly/ff256.h"
#include "fieldpoly/fieldpoly.h"

/* this holds all the polynomials and checking pieces for 
 * a round of communication on a single wire */
typedef struct 
{
	poly_t  *h[N*T+1];
	ff256_t *c[N*T+1][N];
} trans_contents;

/* this holds the uint8_t representation of a trans_contents.
 * Specifically it is exactly what is sent on a wire for a single
 * round that transmits h poly's and checking pieces */
typedef struct 
{
	unsigned long seq_num;
	uint8_t round_num;
	/* this aux uint8_t is used differently at different phases
	 * Phase 1: 1 if dummy packet (see documentation in mcio). 0 
	 * otherwise
	 * Phase 2: the best_pad that was selected
	 * Phase 3: the ciphertext
	 */
	uint8_t aux; 
	/* On Phase 2 whether or not a pad was successfully recovered
	 * will be indicated at h_vals[best_pad][0]. If the pad was 
	 * successfully recovered, 1. Otherwise, 0.
	 */
	uint8_t h_vals[N*T+1][T+1];
	/* On Phase 3 whether a fault was detected on wire will be 
	 * indicated by c_vals[0][i]. If there was a fault, 1. 
	 * Otherwise, 0. */
	uint8_t c_vals[N*T+1][N];
}__attribute__((packed)) trans_packet;

/* these are the publically accessible methods for interfacing
 * with PSMT */
void psmt_init();
void send_char(char secret);
void *send_spin(void *params);
void *receive_spin(void *params);

/* these don't actually need to be publically accessible but
 * we want to unit test them */
int cont_free(trans_contents *given);
int pack2cont(trans_packet *given, trans_contents *result);
int cont2pack(trans_contents *given, trans_packet *result);
int find_best_pad(trans_contents conts[]);
int find_pad_conflicts(int pad, trans_contents conts[]);
uint8_t retrieve_pad(trans_contents* info, int info_n, int pad_num);
int receiver_phase1(int wire, trans_packet phase1pack);
int receiver_phase3(trans_packet phase3pack);

#endif
