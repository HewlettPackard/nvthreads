#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "nvrecovery.h"
pthread_mutex_t m1;
pthread_mutex_t m2;
int *x;
int *y;
#define aborting 1
void* thread_a(void *args) {
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    *x = 2;
    pthread_mutex_unlock(&m2); // log 1
    if (aborting) {
        sleep(2);
        printf("aborting, x = %d, y = %d\n", *x, *y);
        abort();
    }
    pthread_mutex_unlock(&m1); // log 3
    return 0;
}
void* thread_b(void *args) {
    sleep(1);
    pthread_mutex_lock(&m2);
    *y = *x;
    pthread_mutex_unlock(&m2);  // log 2
    return 0;
}
int main(void) {
    pthread_t tid1, tid2;
    x = (int*)nvmalloc(sizeof(int), (char*)"x");
    y = (int*)nvmalloc(sizeof(int), (char*)"y");

    if (isCrashed()) {
        nvrecover(x, sizeof(int), (char*)"x");
        nvrecover(y, sizeof(int), (char*)"y");
        printf("recovered, x = %d, y = %d\n", *x, *y);     
    } else{
        *x = 0;
        *y = 0;   
        pthread_mutex_init(&m1, NULL);
        pthread_mutex_init(&m2, NULL);
        pthread_create(&tid1, NULL, thread_a, NULL);
        pthread_create(&tid2, NULL, thread_b, NULL);
        pthread_join(tid1, NULL);
        pthread_join(tid2, NULL);
    }
    return 0;
}


