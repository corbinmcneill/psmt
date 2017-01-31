#include "poly.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "debug.h"

int main() {
    /* test polynomial functions */
    debug("testing eval_poly..."); 
    uint8_t test_poly[3] = {(uint8_t) 7, (uint8_t) 8, (uint8_t) 9};
    assert(eval_poly(test_poly, 2, 2) == 59);
    assert(eval_poly(test_poly, 2, 20) == 183);
    debug("passed\n"); 
    debug("testing make_poly_intercept...");
    uint8_t* generated_poly = make_poly_intercept(10, 42);
    assert(eval_poly(generated_poly, 10, 0) == 42);
    debug("passed\n"); 
    debug("testing poly_degree...");
    uint8_t line[5] = { 1, 1, 0, 3, 0};
    assert( poly_degree(line, 4) == 3);
    debug("passed\n"); 
    debug("\n\n\ninterpolation stuff\n");
    unsigned char x[2] = { 1, 2};
    unsigned char y[2] = { 20, 30};
    unsigned char secret; 
    secret = interpolate_yintercept(x,y,2);
    debug("\n\n%hhu \n", secret);
    return 0;
} 
