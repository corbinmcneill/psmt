#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "psmt.h"

/* define these macros just for testing */
#undef N
#undef T
#define N 3
#define T 2

int main(int argc, char *argv[]) {
	printf("Testing pack2cont...\n");
	void test_pack2cont();
	printf("passed...\n");

	printf("\nTesting cont2pack...\n");
	void test_cont2pack();
	printf("passed...\n");

	//printf("\n\nTesting find_pad_conflicts...\n");
	//void test_find_pad_conflicts();
	//printf("passed...\n");
}

void test_pack2cont() {
	uint8_t h_vals[N*T+1][T+1];
	uint8_t c_vals[N*T+1][N];

	for(int i=0; i<N*T+1; i++) {
		for (int j=0; j<T+1; j++) {
			h_vals[i][j] = rand();
		}
		for (int j=0; j<N; j++) {
			c_vals[i][j] = rand();
		}
	}

	trans_packet packet = {
		.seq_num = 3, 
		.round_num = 1,
		.aux = 0 };
	memcpy(packet.h_vals, h_vals, sizeof(uint8_t) * (N*T+1)*(T+1));
	memcpy(packet.c_vals, c_vals, sizeof(uint8_t) * (N*T+1)*(N));

	trans_contents contents;

	pack2cont(&packet, &contents);

	for(int i=0; i<N*T+1; i++) {
		for (int j=0; j<T+1; j++) {
			assert(h_vals[i][j] == ((ff256_t*)contents.h[i]->coeffs[j])->val);
		}
		for (int j=0; j<N; j++) {
			assert(c_vals[i][j] == (contents.c[i][j])->val);
		}
	}

	cont_free(&contents);
}

void test_cont2pack() {
	trans_contents contents;
	trans_packet packet;

	for(int i=0; i<N*T+1; i++) {
		poly_t *p = malloc(sizeof(poly_t));
		ff256_t** coeffs = calloc(sizeof(ff256_t*), T+1);
		for (int j=0; j<T+1; j++) {
			ff256_t *e = malloc(sizeof(ff256_t));
			ff256_init(e);
			e->val = rand();

			coeffs[j] = e;
		}
		p->degree = T+1;
		p->coeffs = (element_t**) coeffs;
		contents.h[i] = p;

		for (int j=0; j<N; j++) {
			ff256_t *e = malloc(sizeof(ff256_t));
			ff256_init(e);
			e->val = rand();

			contents.c[i][j] = e;
		}
	}

	cont2pack(&contents, &packet);

	for(int i=0; i<N*T+1; i++) {
		for (int j=0; j<T+1; j++) {
			assert(packet.h_vals[i][j] == ((ff256_t*)contents.h[i]->coeffs[j])->val);
		}
		for (int j=0; j<N; j++) {
			assert(packet.c_vals[i][j] == (contents.c[i][j])->val);
		}
	}
	cont_free(&contents);
}

void test_find_pad_conflicts() {
    /* ff256 evaluation of a polynomial */
    ff256_t* eval_element;

    /* an blank example of an initialized ff256 element for the fieldpoly 
     * library */
    ff256_t* reference_element = malloc(sizeof(ff256_t));
    ff256_init(reference_element);

    /* element used for evaluation of h's to generate checking pieces */
    ff256_t* iter_element = malloc(sizeof(ff256_t));
    ff256_init(iter_element);

	poly_t *f;

	/* for each wire, the N*T+1 h polynomials and the associated checking
	 * pieces to be sent for each pad */
	trans_contents data[N];
	/* the uint8_t representation of data. This is precicely the contents
	 * of the packet send on each wire */
	trans_packet *data_pack = calloc(N, sizeof(trans_packet));
    
	/* Iterate over each channel. For each channel, use the evaluation 
	 * of f at a unique point to designate the y-intercept of that h poly.*/
	for (int j=0; j<N; j++) {
        f = rand_poly(T, (element_t*) reference_element);
        /* pick x */
        iter_element->val = j+1;
        /* calculate y */
        eval_element = (ff256_t*) eval_poly(f,(element_t*) iter_element);
		data[j].h[0] = rand_poly_intercept(T, (element_t*) eval_element);

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
			data_pack[j].h_vals[0][k] = 
				((ff256_t*)data[j].h[0]->coeffs[k])->val;
		}
		/* evaluate h at N+1 places */
		for (int k=1; k<N+1; k++) {
            iter_element->val = k;
            ff256_t *result = (ff256_t*)eval_poly(data[j].h[0], 
                	(element_t*)iter_element);
            data_pack[j].c_vals[0][k] = result->val;
		}
	}

}
