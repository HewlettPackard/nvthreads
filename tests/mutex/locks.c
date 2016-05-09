// Test case for NVthreads sync engine
// Multiple threads incrementing global counter by 1
// Expected result: counter should equal to THREADS * quota
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "nvrecovery.h"

#define THREADS 2
#define quota 10
pthread_mutex_t glock1;
pthread_mutex_t glock2;
int A;
int B;


void* thread_a(void *args) {
    int i;
    pthread_mutex_init(&glock2, NULL);
    for(int i = 0; i < quota; i++) {
        pthread_mutex_lock(&glock1);    
        A++;
        printf("%d %d: A=%d\n", getpid(), i, A);
//      B = A;
//      printf("thread: B=%d\n", B);
        pthread_mutex_unlock(&glock1);
    }   
    for(int i = 0; i < quota; i++) {
        pthread_mutex_lock(&glock2);    
        B++;
        printf("%d %d: B=%d\n", getpid(), i, B);
        pthread_mutex_unlock(&glock2);
    }   
    printf("thread: Exits thread_a()\n");
    return 0;
}
int main(void) {
    pthread_t tid[THREADS];
    int i;
    int t;
    A = 0;
    printf("M: glock1: %p\n", &glock1);
    pthread_mutex_init(&glock1, NULL);
    pthread_mutex_init(&glock2, NULL);
    pthread_mutex_init(&glock2, NULL);
    for(t = 0; t < THREADS; t++) {
        pthread_create(&tid[t], NULL, thread_a, NULL);
    }
    
    for(int i = 0; i < quota; i++) {
        pthread_mutex_lock(&glock1);
        A++;
        printf("main %d: A=%d\n", i, A);
        pthread_mutex_unlock(&glock1);
    }
    
    for(int i = 0; i < quota; i++) {
        pthread_mutex_lock(&glock2);    
        B++;
        printf("main %d: B=%d\n", i, B);
        pthread_mutex_unlock(&glock2);
    }   

    for(t = 0; t < THREADS; t++) {
        pthread_join(tid[t], NULL);
    }
    printf("A: %d, B: %d\n", A, B);
    printf("M: exits\n");
    return 0;
}


