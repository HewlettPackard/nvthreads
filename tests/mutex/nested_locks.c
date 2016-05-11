#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "nvrecovery.h"

pthread_mutex_t glock1;
pthread_mutex_t glock2;
int A;
int B;
int B1;
int X;
#define LIMIT_A 100

void print_thread_id(pthread_t id)
{
//  size_t i;
//  for (i = sizeof(i); i; --i)
//      printf("%02x", *(((unsigned char*) &id) + i - 1));
    printf("%lx\n", (unsigned long)id);
}

void* thread_a(void *args) {
    pthread_mutex_lock(&glock1);
//  printf("a locked glock 1\n");
    X = 0;
//  sleep(1);
    pthread_mutex_lock(&glock2);
//  printf("a locked glock 2\n");
//  sleep(1);
    A = 2;
    printf("A: A = %d, X = %d\n", A, X);

//  printf("a unlocking glock 2\n");
    pthread_mutex_unlock(&glock2);
    X = 2;
//  sleep(3);
//  printf("a unlocking glock 1\n");
    pthread_mutex_unlock(&glock1);
    sleep(3);
    printf("A: Exits thread_a()\n");
    return 0;
}
void* thread_b(void *args) {
    pthread_mutex_lock(&glock2);
//  printf("b locked glock 2\n");

    B = A;
    B1 = X;
//  sleep(2);
    printf("B: A=%d, B1=%d\n", A, B1); 
//  printf("b unlocking glock 2\n");
    pthread_mutex_unlock(&glock2);
    sleep(3);
    printf("B: Exits thread_b()\n");
    return 0;
}
int main(void) {
    pthread_t tid1, tid2;
    A = 0;
    B = 0;
    printf("M: glock1: %p, glock2: %p\n", &glock1, &glock2);
    pthread_mutex_init(&glock1, NULL);
    pthread_mutex_init(&glock2, NULL);
    pthread_create(&tid1, NULL, thread_a, NULL);
    pthread_create(&tid2, NULL, thread_b, NULL);

    pthread_mutex_lock(&glock1);
    printf("M: main(): A = %d\n", A);
    pthread_mutex_unlock(&glock1);

    pthread_mutex_lock(&glock2);
    printf("M: main(): B = %d\n", B);
    pthread_mutex_unlock(&glock2);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    printf("M: exits\n");
    return 0;
}


