#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
pthread_mutex_t glock1;
pthread_mutex_t glock2;
int A;
int B;
int B1;
int X;
#define LIMIT_A 100
void* thread_a(void *args) {
    pthread_mutex_lock(&glock1);
    printf("a locked glock 1\n");
    X = 0;
    sleep(1);
    pthread_mutex_lock(&glock2);
    printf("a locked glock 2\n");
    sleep(1);
    A = 2;

    printf("a unlocked glock 2\n");
    pthread_mutex_unlock(&glock2);
    X = 2;
    sleep(1);
    printf("a unlocked glock 1\n");
    pthread_mutex_unlock(&glock1);
    return 0;
}
void* thread_b(void *args) {
    pthread_mutex_lock(&glock2);
    printf("b locked glock 2\n");

    B = A;
    B1 = X;
       
    printf("A=%d, B1=%d\n", A, B1); 
    printf("b unlocked glock 2\n");
    pthread_mutex_unlock(&glock2);
    return 0;
}
int main(void) {
    pthread_t tid1, tid2;
    A = 0;
    B = 0;
    pthread_mutex_init(&glock1, NULL);
    pthread_mutex_init(&glock2, NULL);
    pthread_create(&tid1, NULL, thread_a, NULL);
    pthread_create(&tid2, NULL, thread_b, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    return 0;
}


