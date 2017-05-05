#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "pq.h"
#include "debug.h"

int parent(int node){
    return node/2;
}
int left(int node){
    return 2*node;
}
int right(int node){
    return 2*node +1;
}

void initpq(tppq* pq,int maxsize) {
    pq->arr = malloc(maxsize*sizeof(mc_packet));
    pq->size = 0;
    pq->maxsize = maxsize;
} 

void destroypq(tppq* pq) {
    free(pq->arr);
}

int mc_packet_comp(mc_packet* a, mc_packet* b) {
    int seqdiff = a->tp.seq_num - b->tp.seq_num;
    if (seqdiff == 0)
        return a->tp.round_num - b->tp.round_num;
    return seqdiff;
}

void swap(tppq* pq, int a, int b) {
    mc_packet temp = pq->arr[a];
    pq->arr[a] = pq->arr[b]; 
    pq->arr[b] = temp;
}


void min_heapify(tppq* pq, int node) {
    size_t l = left(node);
    size_t r = right(node);
    int smallest;
    if (l < pq->size && mc_packet_comp(&pq->arr[l],&pq->arr[node]) <= 0) {
        smallest = l;
    }else {
        smallest = node;
    }
    if (r < pq->size && mc_packet_comp(&pq->arr[r],&pq->arr[smallest]) <= 0) {
        smallest = r;   
    }
    if (smallest != node) {
        swap(pq, smallest,node);
        min_heapify(pq, smallest);
    }
}

mc_packet pop(tppq* pq){
    assert(pq->size > 0);
    mc_packet to_return = pq->arr[0];
    pq->arr[0] = pq->arr[pq->size-1];
    pq->size--;
    min_heapify(pq,0);
    debug("Popped packet: wire=%d, seq=%d,round=%d, pqsize=%d\n",     to_return.wire, to_return.tp.seq_num, to_return.tp.round_num,pq->size);
    return to_return;
}

void insert(tppq* pq, mc_packet* item) {
    assert(pq->size != pq->maxsize);
    
    pq->size++;
    int i = pq->size-1;
    pq->arr[i] = *item;
    debug("inserted packet: wire=%d, seq=%d,round=%d, pqsize=%d\n",item->wire, item->tp.seq_num, item->tp.round_num,pq->size);
    while (i > 0 && mc_packet_comp(&pq->arr[parent(i)],&pq->arr[i]) > 0 ){
        swap(pq, parent(i), i);
        i = parent(i);
    }
}



