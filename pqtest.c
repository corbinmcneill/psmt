#include "debug.h"
#include "pq.h"
#include "psmt.h"

int main() {
    tppq queue;
    initpq(&queue,50);
    mc_packet testpackets[10];
    testpackets[0].tp.seq_num = 1;
    testpackets[0].tp.round_num = 1;
    testpackets[1].tp.seq_num = 1;
    testpackets[1].tp.round_num = 2;
    testpackets[4].tp.seq_num = 0;
    testpackets[4].tp.round_num = 3;
    insert(&queue,&(testpackets[4]));
    insert(&queue,&(testpackets[0]));
    insert(&queue,&(testpackets[1]));

    debug("%d\n",pop(&queue).tp.round_num);
    debug("%d\n",pop(&queue).tp.round_num);
    debug("%d\n",pop(&queue).tp.round_num);


}
