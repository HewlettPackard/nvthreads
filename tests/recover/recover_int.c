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
// Verify that aggregate data structures spanning multiple pages can be recovered by NVthreads
// Result: recovery works correctly

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "nvrecovery.h"

pthread_mutex_t gm;
#define touch_size 4096 * 3

void *t(void *args){        
    nvcheckpoint();
    pthread_exit(NULL);
}

int main(){
    pthread_mutex_init(&gm, NULL);
    pthread_t tid1;
    
    
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
        ptr = malloc(sizeof(int));
        nvrecover(ptr, sizeof(int), (char*)"e");
        printf("Recovered e = %d\n", *(int*)ptr);
        free(ptr);
    }
    else{    
        printf("Program did not crash before, continue normal execution.\n");
        pthread_create(&tid1, NULL, t, NULL);

        int *c = (int *)nvmalloc(sizeof(int), (char *)"c");
        *c = 12345;
        printf("c: %d\n", *c);
        int *d = (int *)nvmalloc(sizeof(int), (char *)"d");
        *d = 56789;
        printf("d: %d\n", *d);
        int *e = (int *)nvmalloc(sizeof(int), (char *)"e");
        *e = 99999;
        printf("e: %d\n", *e);         
        printf("finish writing to values\n");
        nvcheckpoint();

        pthread_join(tid1, NULL);
        printf("internally abort!\n");
        abort(); 
    }

    printf("-------------main exits-------------------\n");
    return 0;
}
