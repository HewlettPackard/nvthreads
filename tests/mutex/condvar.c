#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
pthread_mutex_t glock1;
pthread_mutex_t glock2;
pthread_cond_t cv;
int A;
int B;
#define LIMIT_A 100
void *thread_a(void *args){
        pthread_mutex_lock(&glock1);
        printf("a locked glock 1\n");
	sleep(1);
        pthread_mutex_lock(&glock2);
        printf("a locked glock 2\n");
	B = 99;
	sleep(1);
        while(A != 1){
		sleep(1);
                pthread_cond_wait(&cv, &glock2);
        }
        printf("a read A=%d!\n", A);
        printf("a unlocked glock 2\n");
        pthread_mutex_unlock(&glock2);
	sleep(1);
        printf("a unlocked glock 1\n");
        pthread_mutex_unlock(&glock1);
        return 0;
}
void *thread_b(void *args){
        sleep(1);
        pthread_mutex_lock(&glock2);
        printf("b locked glock 2\n");
	sleep(1);
        pthread_cond_signal(&cv);
	printf("B = %d\n", B);
        A = 1;
        printf("b unlocked glock 2\n");
        pthread_mutex_unlock(&glock2);
        return 0;
}
int main(void){
        pthread_t tid1, tid2;
        A = 0;
	B = 0;
        pthread_mutex_init(&glock1, NULL);
        pthread_mutex_init(&glock2, NULL);
        pthread_cond_init(&cv, NULL);
        pthread_create(&tid1, NULL, thread_a, NULL);
        pthread_create(&tid2, NULL, thread_b, NULL);
        pthread_join(tid1, NULL);
        pthread_join(tid2, NULL);
        printf("final A = %d\n", A);
        return 0;
}

