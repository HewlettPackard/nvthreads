// Auther: Terry Hsu
// Verify that the pages dirtied by child threads without critical sections
// will still be merged back to the main process at the join() call 
// Result: Yes, dirtied pages will be merged back to main process after join (a synchronization point)
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "nvrecovery.h"

struct thread_data {
    int tid;
    int *local_data;
};

void *t(void *args){
    struct thread_data *data = (struct thread_data*)args;
    *(data->local_data) = data->tid;
    free(args);
}

int main(){
    pthread_t tid[3];
    int i;
    int *data = (int*)malloc(sizeof(int) * 3);
    for (i = 0; i < 3; i++){
        struct thread_data *tmp = (struct thread_data*)malloc(sizeof(struct thread_data));
        tmp->tid = i;
        tmp->local_data = &(data[i]);
        pthread_create(&tid[i], NULL, t, (void*)tmp);
    }
    fprintf(stderr, "joining threads\n");
    for (i = 0; i < 3; i++)
        pthread_join(tid[i], NULL);
    fprintf(stderr, "joined threads\n");

    for (i = 0; i < 3; i++)
        printf("data[%d] = %d\n", i, data[i]);

    printf("-------------main exits-------------------\n");
    return 0;
}
