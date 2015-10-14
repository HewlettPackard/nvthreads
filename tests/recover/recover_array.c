#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "nvrecovery.h"

pthread_mutex_t gm;
#define touch_pages 10
#define touch_size 4096 * touch_pages

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
        char *c = (char*)malloc(touch_size);
        nvrecover(c, touch_size, (char *)"c");
        for (int i = 0; i < touch_size; i++) {
            printf("c[%d] = %c\t", i, c[i]);
            if ( i % 7 == 0 && i != 0) {
                printf("\n");
            }
        }
        free(c);
    }
    else{    
        printf("Program did not crash before, continue normal execution.\n");
        pthread_create(&tid1, NULL, t, NULL); 

        char *c = (char *)nvmalloc(touch_size, (char *)"c");
        int ascii = 97;
        for (int i = 0; i < touch_size; i++, ascii++) {
            if ( ascii > 122 ) {
                ascii = 97;
            }
            c[i] = ascii;

            printf("c[%d] = %c\t", i, c[i]);
            if ( i % 7 == 0 && i != 0) {
                printf("\n");
            }
        }
        printf("wrote c for %d bytes\n", touch_size);
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
