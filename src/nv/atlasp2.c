#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t redlock;
pthread_mutex_t bluelock;
int x;
int y;
int z;
void *t1(void *args){
    sleep(1);
    pthread_mutex_lock(&bluelock);
    sleep(1);
    x = 1;
    printf("%d x = %d\n", getpid(), x);
    pthread_mutex_unlock(&bluelock); 

    sleep(3);
    pthread_mutex_lock(&bluelock);
    y = x;
    printf("%d y = x = %d\n", getpid(), y);
    pthread_mutex_unlock(&bluelock); 
    return NULL;
}
void *t2(void *args){
//  sleep(1);
    pthread_mutex_lock(&redlock);
    sleep(2);
    pthread_mutex_lock(&bluelock); 
    x = 2;
    printf("%d x = %d\n", getpid(), x);
    pthread_mutex_unlock(&bluelock); 
    
    sleep(3);
    z = 5;
    printf("%d z = %d\n", getpid(), z);
    pthread_mutex_unlock(&redlock); 
    return NULL; 
}
int main(void){
    pthread_t tid1;
    pthread_t tid2;
    x = 0;
    y = 0;
    z = 0;
    pthread_mutex_init(&redlock, NULL);
    pthread_mutex_init(&bluelock, NULL);
    pthread_create(&tid1, NULL, t1, NULL);
    pthread_create(&tid2, NULL, t2, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    return 0;
}
