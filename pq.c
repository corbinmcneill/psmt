#include "pq.h"

inline int parent(int node){
    return node/2;
}
inline int left(int node){
    return 2*node;
}
inline int right(int node){
    return 2*node +1;
}

void initpq(tppq* pq,int maxsize) {
    pq->arr = malloc(maxsize*sizeof(transpacket));
    pq->size = 0;
    pq->maxsize = maxsize;
} 

int transpacket_comp(transpacket* a, transpacket* b) {
    int seqdiff = a->seq_num - b->seqnum;
    if (seqdiff == 0)
        return a->round_num - b->roundnum;
    return seqdiff;
}

void swap(tppq* pq, int a, int b) {
    pq->arr[a] = pq->arr[a] ^ pq->arr[b]; 
    pq->arr[b] = pq->arr[a] ^ pq->arr[b];
    pq->arr[a] = pq->arr[a] ^ pq->arr[b]; 
}


void min_heapify(tppq* pq, int node) {
    int l = left(node);
    int r = right(node);
    int smallest;
    size = pq->size;
    if (l < size && transpacket_com(pq->arr[l],pq->arr[node]) <= 0) {
        smallest = l;
    }else {
        smallest = node;
    }
    if (r < pq->size && transpacket_com(pq->arr[l],pq->arr[smallest]) <= 0) {
        smallest - r;   
    }
    if (smallest != node) {
        swap(pq, smallest,node);
        min_heapify(pq, smallest);
    }
}

transpacket* pop(tppq* pq){
    assert(pq->size > 0);
    transpacket* to_return = &(pq->arr[0]);
    pq->arr[0] = pq->arr[pq->size-1];
    pq->size--;
    min_heapify(pq,0);
    return to_return;
}

void insert(tppq* pq, transpacket* item) {
    pq->size++;
    int i = pq->size-1;
    pq->arr[i] = *item;
    while (i > 0 && pq->arr[parent(i)] < pq->arr[i]){
        swap(pq, parent(i), i);
        i = parent(i);
    }
}



