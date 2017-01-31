#ifndef POLY_H
#define POLY_h

#include <stdint.h>

uint8_t* make_poly(uint8_t degree);
uint8_t eval_poly(uint8_t *poly, uint8_t degree, uint8_t x);
uint8_t* make_poly_intercept(uint8_t degree, uint8_t intercept);
uint8_t poly_degree(uint8_t *poly, uint8_t max_degree);
unsigned char interpolate_yintercept(unsigned char *x, unsigned char *y, unsigned char num_points);

#endif
