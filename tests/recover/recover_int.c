#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "nvrecovery.h"

pthread_mutex_t gm;
#define touch_size 4096 * 3

void *t(void *args){
    int sec = 3;
    printf("locking\n");
    pthread_mutex_lock(&gm);
//  printf("child thread sleeping for %d seconds\n", sec);
//  sleep(sec);
    pthread_mutex_unlock(&gm);

    printf("locking again\n");
    pthread_mutex_lock(&gm);
//  printf("child thread sleeping for %d seconds\n", sec);
//  sleep(sec);
    pthread_mutex_unlock(&gm);

//  sleep(10);
    pthread_exit(NULL);
}
int main(){
    pthread_mutex_init(&gm, NULL);
    pthread_t tid1;
    
    
    printf("Checking crash status\n");
    if ( isCrashed() ) {
        printf("I need to recover!\n");
        void *ptr = malloc(sizeof(int));
        nvrecover(ptr, sizeof(int), (char *)"c");
        printf("Recovered c = %d\n", *(int*)ptr);
        free(ptr);
        ptr = malloc(sizeof(int));
        nvrecover(ptr, sizeof(int), (char*)"d");
        printf("Recovered d = %d\n", *(int*)ptr);
        free(ptr);
    }
    else{    
        printf("Program did not crash before, continue normal execution.\n");
        pthread_create(&tid1, NULL, t, NULL); 

        int *c = (int *)nvmalloc(sizeof(int), (char *)"c");
//      int *c = (int*)malloc(sizeof(int));
        *c = 12345;
        printf("c: %d\n", *c);
        int *d = (int *)nvmalloc(sizeof(int), (char *)"d");
        *d = 56789;
        printf("d: %d\n", *d);
        int *e = (int *)nvmalloc(sizeof(int), (char *)"e");
        *e = 99999;
        printf("e: %d\n", *e);         

        pthread_mutex_lock(&gm);
        pthread_mutex_unlock(&gm);
        pthread_mutex_lock(&gm);
        pthread_mutex_unlock(&gm);
        pthread_mutex_lock(&gm);
        pthread_mutex_unlock(&gm);

        pthread_mutex_lock(&gm);
        printf("finish writing to values\n");
        pthread_mutex_unlock(&gm);

        pthread_join(tid1, NULL);
        printf("internally abort!\n");
        abort(); 
    }

    printf("-------------main exits-------------------\n");
    return 0;
}
