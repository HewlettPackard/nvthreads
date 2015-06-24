#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
int a, b;
pthread_mutex_t gm;
void *t1(void *args){
    pthread_mutex_lock(&gm);
    a = 1;
    printf("t1: a=%d\n", a);
    pthread_mutex_unlock(&gm);
    return NULL;
}
void *t2(void *args){
    pthread_mutex_lock(&gm);
    b = 2;
    printf("t2: b=%d\n", b);
    pthread_mutex_unlock(&gm);
    return NULL;
}
int main(){
    a = b = 0;
    pthread_mutex_init(&gm, NULL);
    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, t1, NULL);
    pthread_create(&tid2, NULL, t2, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    return 0;
}
