/*
(c) Copyright [2017] Hewlett Packard Enterprise Development LP

This program is free software; you can redistribute it and/or modify it under 
the terms of the GNU General Public License, version 2 as published by the 
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
Place, Suite 330, Boston, MA 02111-1307 USA

*/
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
    // Directly modify private data section without entering critical sections
    *(data->local_data) = data->tid;
    free(args);
}

int main(){
    pthread_t tid[3];
    int i;

    // data[i] will be modified by thread i
    int *data = (int*)malloc(sizeof(int) * 3);
    memset(data, -1, sizeof(int)*3);
    // verify we initial the value in data[i] to -1
    for (i = 0; i < 3; i++)
        printf("data[%d] = %d\n", i, data[i]);

    // Create thread i to modify data[i]
    for (i = 0; i < 3; i++){
        struct thread_data *tmp = (struct thread_data*)malloc(sizeof(struct thread_data));
        tmp->tid = i;
        tmp->local_data = &(data[i]);
        pthread_create(&tid[i], NULL, t, (void*)tmp);
    }

    // A synchronization call (but the entire program does not contain any critical sections)
    fprintf(stderr, "joining threads\n");
    for (i = 0; i < 3; i++)
        pthread_join(tid[i], NULL);
    fprintf(stderr, "joined threads\n");

    // data array now should be data[i] = i
    for (i = 0; i < 3; i++)
        printf("data[%d] = %d\n", i, data[i]);

    printf("-------------main exits-------------------\n");
    return 0;
}
