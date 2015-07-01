#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef NVTHREAD
#include "nvrecovery.h"
#endif

#define touch_size 4096 * 3

int a;
char b[touch_size];
pthread_mutex_t gm;
void *t1(void *args){
    printf("%d: locking\n", getpid());
    pthread_mutex_lock(&gm);
    printf("%d: locked\n", getpid());
    a++;
    printf("t1: a=%d\n", a);
    printf("%d: unlocking\n", getpid());
    pthread_mutex_unlock(&gm);
    printf("--------------thread 1 exits------------------\n");
    return NULL;
}
void *t2(void *args){    
    int i;

#ifdef NVTHREAD
    int *c = (int *)nvmalloc(sizeof(int), (char *)"c");
#endif
    for (i = 0; i < touch_size; i++) {
        b[i] = 33;
//      b[0] = 33;
        if ( i % 4096 == 0 )
            printf("writing to page: %d\n", i / 4096);
    }
    printf("%d: locking\n", getpid());
    pthread_mutex_lock(&gm);
    printf("%d: locked\n", getpid());
    printf("t2: a=%d\n", a);
    a++;
//  a = 2;
    printf("t2: a=%d\n", a);
    printf("%d: unlocking\n", getpid());
    pthread_mutex_unlock(&gm);
    printf("--------------thread 2 exits------------------\n");
    return NULL;
}
int main(){
    int i;
#ifdef NVTHREAD
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
        int *c = (int *)nvmalloc(sizeof(int), (char *)"c");
        *c = 12345;
        printf("c: %d\n", *c);
        int *d = (int *)nvmalloc(sizeof(int), (char *)"d");
        *d = 56789;
        printf("d: %d\n", *d);
        int *e = (int *)nvmalloc(sizeof(int), (char *)"e");
        *e = 99999;
        printf("e: %d\n", *e); 
        
        printf("abort!\n");
        abort();
    }
#endif

//  a = 0;
//  printf("a: %p\n", (&a));
//  pthread_mutex_init(&gm, NULL);
//  pthread_t tid1, tid2;
//  pthread_create(&tid1, NULL, t1, NULL);
//  pthread_create(&tid2, NULL, t2, NULL);
//
//  pthread_join(tid1, NULL);
//  pthread_join(tid2, NULL);

    printf("-------------main exits-------------------\n");
    return 0;
}
