#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#define N 10
int shared;
int *hshared;
pthread_mutex_t plock;
pthread_mutex_t hlock;

void *thread_print(void *a){
	int id = *(int*)a;
	pthread_mutex_lock(&plock);
	pthread_mutex_lock(&hlock);
	shared = id * 10;
	*hshared = shared * 10;
	printf("%d: thread id %d, shared=%d, hshared=%d\n", getpid(), id, shared, *hshared);
	pthread_mutex_unlock(&hlock);
	pthread_mutex_unlock(&plock);
	return NULL;
}

int main(void){
	int rv;
	pthread_t tid[N];
	int index[N];
	int i;
	hshared = (int*) malloc(sizeof(int));
	shared = 0;
	pthread_mutex_init(&plock, NULL);
	pthread_mutex_init(&hlock, NULL);
	for (i = 0; i < N; i++){
		index[i] = i;
		rv = pthread_create(&tid[i], NULL, &thread_print, &index[i]);
		if( rv != 0)
			exit(-1);
	}
	for (i = 0; i < N; i++){
		pthread_join(tid[i], NULL);
	}
	return 0;
}
