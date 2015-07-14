#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef NVTHREAD
#include "nvrecovery.h"
#elif DTHREAD
#else
#include <pthread.h>
#endif


#define nthreads 3

int ready;      // shared and transient
int x, y, m, p, q; // shared and persistent
pthread_mutex_t l1, l2, l3;

void *t0(void *args){
    printf("t0\n");
    fflush(stdout);
    int t = 0;  // local and transient
    pthread_mutex_lock(&l1);
    printf("t0 locked l1\n");
    t = ready;
    printf("t0 unlocked l1\n");
    pthread_mutex_unlock(&l1);
    if ( t ) {
        y = x;
        printf("ready! y = x = %d\n", y);
    }
    else{
        printf("Not ready.. t = %d, y = %d, x = %d\n", t, y, x);    
    }
    printf("--------t0 exits-----------\n");
//  sleep(3);
    pthread_exit(NULL);
}

void *t1(void *args){
    printf("t1\n");
    fflush(stdout);
    pthread_mutex_lock(&l2);
    printf("t1 locked l2\n");
    p = q;
    printf("t1 unlocked l2\n"); 
    pthread_mutex_unlock(&l2);
    x = 1;
    pthread_mutex_lock(&l1); 
    printf("t1 locked l1\n");
    ready = 1;
    printf("t1 unlocked l1\n"); 
    pthread_mutex_unlock(&l1); 

    printf("--------t1 exits-----------\n");
    pthread_exit(NULL);
}

void *t2(void *args){
    printf("t2 sleeping...\n");
    fflush(stdout);
    
sleep(3);
/*
    pthread_mutex_lock(&l3);
    printf("t2 locked l3\n"); 
    pthread_mutex_lock(&l2); 
    printf("t2 locked l2\n"); 
    q = 1;
    printf("t2 locked l2\n");
    pthread_mutex_unlock(&l2);
    m = 1;
    printf("t2 locked l3\n");
    pthread_mutex_unlock(&l3);
*/
    printf("--------t2 exits-----------\n");
    pthread_exit(NULL);
}

int main(void){
    
    pthread_t th[3];
    int i;
    
    pthread_mutex_init(&l1, NULL);
    pthread_mutex_init(&l2, NULL);
    pthread_mutex_init(&l3, NULL);
    x = y = m = p = 0;
    pthread_create(&th[0], NULL, t0, NULL);
    pthread_create(&th[1], NULL, t1, NULL);
    pthread_create(&th[2], NULL, t2, NULL);
    

    for (i = 0; i < nthreads; i++) {
        pthread_join(th[i], NULL);
    }


	return 0;
}
