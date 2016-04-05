#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#define PAGE_SIZE 4096
#define PAGES 5
#define touch_size PAGE_SIZE * PAGES
#define ALLOC_SIZE 1 * 1024 
int a[touch_size];
pthread_mutex_t gm;
void* t(void *args) {
    int i = 0;
    printf("%d: allocating\n", getpid());
    for (i = 0; i < touch_size; i++) {
        if ( i % PAGE_SIZE == 0 ) {
            printf("%d: locking page %d to write\n", getpid(), i / PAGE_SIZE);
            pthread_mutex_lock(&gm);
        }
        a[i] = 0;
        if ( i % PAGE_SIZE == 0 ) {
            printf("%d: unlocking\n", getpid());
            pthread_mutex_unlock(&gm);
        }
    }
    printf("%d: t exits\n", getpid());
    pthread_exit(NULL);
}
int main() {
    int i = 0;
    pthread_mutex_init(&gm, NULL);
    pthread_t tid1, tid2, tid3, tid4;
    pthread_create(&tid1, NULL, t, NULL);
    pthread_create(&tid2, NULL, t, NULL);
    pthread_create(&tid3, NULL, t, NULL);
    pthread_create(&tid4, NULL, t, NULL);

    for (i = 0; i < touch_size; i++) {
        if ( i % PAGE_SIZE == 0 ) {
            printf("%d: locking page %d to write\n", getpid(), i / PAGE_SIZE);
            pthread_mutex_lock(&gm);
        }
        a[i] = 0;
        if ( i % PAGE_SIZE == 0 ) {
            printf("%d: unlocking\n", getpid());
            pthread_mutex_unlock(&gm);
        }
    }

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);
    return 0;
}
