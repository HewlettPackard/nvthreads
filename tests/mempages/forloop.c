#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef NVTHREAD
#include "nvrecovery.h"
#endif

#define touch_size 4096 * 10

char b[touch_size];
void* t1(void *args) {
    int i;

    for (i = 0; i < touch_size; i++) {
        b[i] = 33;
        if ( i % 4096 == 0 )
            printf("writing to page: %d\n", i / 4096);
    }
    printf("--------------thread 1 exits------------------\n");
    return NULL;
}
int main() {

    int i;
    pthread_t tid1;
    pthread_create(&tid1, NULL, t1, NULL);
    pthread_join(tid1, NULL);

    for (i = 0; i < touch_size; i++) {
        b[i] = 33;
        if ( i % 4096 == 0 )
            printf("writing to page: %d\n", i / 4096);
    }

    printf("-------------main exits-------------------\n");
    return 0;
}
