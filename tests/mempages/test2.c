#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef NVTHREAD
#include "nvrecovery.h"
#endif

#define touch_size 4096 * 3

int a;
char b[touch_size];
pthread_mutex_t gm;
void* t1(void *args) {
    int i;
//  printf("%d: locking\n", getpid());
    pthread_mutex_lock(&gm);
//  printf("%d: locked\n", getpid());
//  a++;
//  printf("t1: a=%d\n", a);
//  printf("%d: unlocking\n", getpid());
    for (i = 0; i < touch_size; i++) {
        b[i] = 33;
        if ( i % 4096 == 0 )
            printf("writing to page: %d\n", i / 4096);
    }
    printf("--------------thread 1 exits------------------\n");
    pthread_mutex_unlock(&gm);
    return NULL;
}
void* t2(void *args) {
    int i;

//  printf("%d: locking\n", getpid());
    pthread_mutex_lock(&gm);
//  printf("%d: locked\n", getpid());
//  printf("t2: a=%d\n", a);
//  a++;
//  a = 2;
//  printf("t2: a=%d\n", a);
    for (i = 0; i < touch_size; i++) {
        b[i] = 33;
        if ( i % 4096 == 0 )
            printf("writing to page: %d\n", i / 4096);
    }
    printf("--------------thread 2 exits------------------\n");
    pthread_mutex_unlock(&gm);
    return NULL;
}
int main() {

//  a = 0;
//  printf("a: %p\n", (&a));
    pthread_mutex_init(&gm, NULL);
    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, t1, NULL);
    pthread_create(&tid2, NULL, t2, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("-------------main exits-------------------\n");
    return 0;
}
