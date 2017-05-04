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
// Author: Terry Hsu
// Verify the memory allocator's ability to allocate large chunk of memory
// for multiple threads

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef NVTHREAD
#include "nvrecovery.h"
#endif

pthread_mutex_t gm;
unsigned long long chunk_size;

void* t(void *args) {
    int sec = 3;
    char *ptr = (char*)malloc(chunk_size);
    pthread_mutex_lock(&gm);
    printf("allocated\n");
    pthread_mutex_unlock(&gm);

    free(ptr);
    pthread_exit(NULL);
}
int main() {
    pthread_mutex_init(&gm, NULL);
    pthread_t tid1;
    chunk_size = 1024;  //KB
    chunk_size = chunk_size * chunk_size; //MB
    chunk_size = chunk_size * chunk_size; //GB
    chunk_size = chunk_size * 4; // 4GB
    printf("Program did not crash before, continue normal execution.\n");
    pthread_create(&tid1, NULL, t, NULL);
    char *ptr = (char*)malloc(chunk_size);

    pthread_mutex_lock(&gm);
    printf("allocated\n");
    pthread_mutex_unlock(&gm);
    pthread_join(tid1, NULL);

    printf("-------------main exits-------------------\n");
    free(ptr);
    return 0;
}
