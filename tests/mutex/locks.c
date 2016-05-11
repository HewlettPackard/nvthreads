// Test case for NVthreads sync engine
// Multiple threads incrementing global counter by 1
// Expected result: counter should equal to THREADS * quota
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
//#include "nvrecovery.h"

#define THREADS 10
#define quota 5
pthread_mutex_t glock1;
pthread_mutex_t glock2;
int A;
int B;
struct file_struct{
    char *name;
};

void* thread_a(void *args) {
    int i;
    pthread_mutex_init(&glock2, NULL);
    for(i = 0; i < quota; i++) {
        pthread_mutex_lock(&glock1);    
        A++;
        printf("(+A) %d %d: A=%d B=%d\n", getpid(), i, A, B);
        pthread_mutex_unlock(&glock1);
    }   
//  for(int i = 0; i < quota; i++) {
//      pthread_mutex_lock(&glock2);
//      B++;
//      printf("(+B) %d %d: A=%d B=%d\n", getpid(), i, A, B);
//      pthread_mutex_unlock(&glock2);
//  }
    printf("thread: Exits thread_a()\n");
    return 0;
}
int main(void) {
    pthread_t tid[THREADS];
    int i;
    int t;
    A = 10;
    printf("M: glock1: %p\n", &glock1);
    printf("A: %p, B: %p\n", &A, &B);
    pthread_mutex_init(&glock1, NULL);
    pthread_mutex_init(&glock2, NULL);
    pthread_mutex_init(&glock2, NULL);
    for(t = 0; t < THREADS; t++) {
        pthread_create(&tid[t], NULL, thread_a, NULL);
    }
    for(i = 0; i < quota; i++) {
        pthread_mutex_lock(&glock1);
        A++;
        printf("(+A) %d %d: A=%d B=%d\n", getpid(), i, A, B);
        pthread_mutex_unlock(&glock1);
    }
    
//  for(int i = 0; i < quota; i++) {
//      pthread_mutex_lock(&glock2);
//      B++;
//      printf("(+B) %d %d: A=%d B=%d\n", getpid(), i, A, B);
//      pthread_mutex_unlock(&glock2);
//  }
//
    for(t = 0; t < THREADS; t++) {
        pthread_join(tid[t], NULL);
    }
//  printf("A: %d, B: %d\n", A, B);
    printf("A: %d\n", A);
    printf("M: exits\n");
    return 0;
}


