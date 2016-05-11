#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "nvrecovery.h"
#define NTHREADS 10
pthread_barrier_t bar;
int A;
void* thread_a(void *args) {
    A++;
    pthread_barrier_wait(&bar);
    printf("%d: A=%d\n", getpid(), A);
}
int main(void) {
    int i;    
    pthread_t tid[NTHREADS];
    printf("M: bar %p\n", &bar);
    pthread_barrier_init(&bar, NULL, NTHREADS);
    A = 0;
    printf("main: A=%d\n", A);
    for(i = 0; i < NTHREADS; i++) {
        pthread_create(&tid[i], NULL, thread_a, NULL);
    }

    for(i = 0; i < NTHREADS; i++) {
        pthread_join(tid[i], NULL);
    }
    printf("main: A=%d\n", A);
    printf("M: exits\n");
    return 0;
}



