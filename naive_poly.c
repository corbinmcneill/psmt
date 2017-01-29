#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "poly.h"

uint8_t* make_poly(uint8_t degree) {
	srand(time(NULL));
	uint8_t *poly = calloc(degree+1, sizeof(uint8_t));
	uint8_t i;
	for (i=0; i<degree+1; i++) {

		poly[i] = (uint8_t) rand();
	}
	return poly;
}

uint8_t* make_poly_intercept(uint8_t degree, uint8_t intercept) {
	uint8_t *poly = calloc(degree+1, sizeof(uint8_t));
	uint8_t i;
	poly[0] = intercept;
	for (i=1; i<degree+1; i++) {
		poly[i] = (uint8_t) rand();
	}
	return poly;
}

uint8_t eval_poly(uint8_t *poly, uint8_t degree, uint8_t x) {
	uint8_t toReturn = 0;
	uint8_t workingX = 1;
	uint8_t i;
	for (i=0; i<=degree; i++) {
		toReturn += workingX*poly[i];
		workingX *= x;
	}
	return toReturn;
}

uint8_t poly_degree(uint8_t *poly, uint8_t max_degree) {
	uint8_t i;
    uint8_t degree;
	for (i=0; i<=max_degree; i++) {
		if (poly[i] != 0) degree = i;
	}
	return degree;
}

uint8_t* interpolate(uint8_t *x, uint8_t *y, uint8_t n) {
	return 0;
}
