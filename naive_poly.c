#include "debug.h"
#include "libgfshare.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "poly.h"

#define SECRET_SIZE 1

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
// returns f(0) using libgfshare
unsigned char interpolate_yintercept(unsigned char *x, unsigned char *y, unsigned char num_points) {
   // add dummy point TODO: fix this 
    /*unsigned char *gfsharex = calloc(num_points+1,sizeof(unsigned char));
    unsigned char *gfsharey = calloc(num_points+1,sizeof(unsigned char));
    for (int i = 0; i < num_points; i++) {
        gfsharex[i] = x[i];
        gfsharey[i] = y[i];
    }
    gfsharex[num_points] = 0;
    gfsharey[num_points] = 0;*/
    unsigned char secret[1];
    gfshare_ctx* context = gfshare_ctx_init_dec( x, num_points , num_points, SECRET_SIZE);
    if (context == NULL) {
        fprintf(stderr, "error creating context\n");
        return 0;
    }
    for (unsigned char i = 0; i < num_points; i++) {
       gfshare_ctx_dec_giveshare(context, x[i], y); 
       y = y +1;
    }
    gfshare_ctx_dec_extract(context, secret);
    return secret[0];
}
