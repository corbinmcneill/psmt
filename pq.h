#include "psmt.h"
typedef struct {
    transpacket* arr;
    size_t size;
    size_t maxsize;
} tppq;
void initpq(tppq* pq, int maxsize);
void insert(transpacket* item, tppq* pq);
transpacket* pop(tppq* pq);


