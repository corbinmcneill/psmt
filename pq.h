#ifndef PQ_H
#define PQ_H
#include "mcio.h"
#include "psmt.h"
#include "stddef.h"

typedef struct {
    mc_packet* arr;
    size_t size;
    size_t maxsize;
} tppq;

void initpq(tppq* pq, int maxsize);
void insert( tppq* pq, mc_packet* item);
mc_packet pop(tppq* pq);

#endif
