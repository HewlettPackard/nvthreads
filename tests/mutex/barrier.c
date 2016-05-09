#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "nvrecovery.h"
#define NTHREADS 10
pthread_barrier_t bar;

void* thread_a(void *args) {
    int *tid = (int*)args;
    pthread_barrier_wait(&bar);
}
int main(void) {
    int i;
    pthread_t tid[NTHREADS];
    printf("M: bar %p\n", &bar);
    pthread_barrier_init(&bar, NULL, NTHREADS+1);
    for(i = 0; i < NTHREADS; i++) {
        pthread_create(&tid[i], NULL, thread_a, NULL);
    }
        sleep(2);
    pthread_barrier_wait(&bar);
    for(i = 0; i < NTHREADS; i++) {
        pthread_join(tid[i], NULL);
    }
    printf("M: exits\n");
    return 0;
}



