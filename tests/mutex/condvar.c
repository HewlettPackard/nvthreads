#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "nvrecovery.h"
pthread_mutex_t glock1;
pthread_mutex_t glock2;
pthread_cond_t cv;
int A;
int *B;
#define LIMIT_A 100
void *thread_a(void *args){
        pthread_mutex_lock(&glock1);
	sleep(1);
        pthread_mutex_lock(&glock2);
	*B = 99;
	sleep(1);
        while(A != 1){
		sleep(1);
                pthread_cond_wait(&cv, &glock2);
        }
        pthread_mutex_unlock(&glock2);
	sleep(1);
        printf("abort!\n");
        abort();
        pthread_mutex_unlock(&glock1);
        return 0;
}
void *thread_b(void *args){
        sleep(1);
        pthread_mutex_lock(&glock2);
	sleep(1);
        A = 1;
        pthread_cond_signal(&cv);
        *B = *B + A;
	printf("B = %d\n", *B);
        pthread_mutex_unlock(&glock2);
        return 0;
}
int main(void){
        pthread_t tid1, tid2;
        A = 0;
        B = (int *)nvmalloc(sizeof(int), (char*)"B");
        if(isCrashed()) {
                nvrecover(B, sizeof(int), (char*)"B");
                printf("recovered B = %d\n", *B);
                return 0;
        } else{
                *B = 0;
        }
        pthread_mutex_init(&glock1, NULL);
        pthread_mutex_init(&glock2, NULL);
        pthread_cond_init(&cv, NULL);
        pthread_create(&tid1, NULL, thread_a, NULL);
        pthread_create(&tid2, NULL, thread_b, NULL);
        pthread_join(tid1, NULL);
        pthread_join(tid2, NULL);
        printf("final A = %d, B= %d\n", A, *B);
        return 0;
}

