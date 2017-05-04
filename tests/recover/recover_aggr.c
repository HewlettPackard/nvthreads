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
#define touch_pages 10
#define touch_size 4096 * touch_pages

// aggregate data structures
struct bar{
    int a;
    char c[15000];
};
struct foo{
    int id;
    struct bar b;
};


struct thread_data {
    int tid;
    struct foo *local_data;
};

void *t(void *args){
    struct thread_data *data = (struct thread_data*)args;
    int tid = data->tid;
    struct foo *tmp = data->local_data;

    pthread_mutex_lock(&gm);
    tmp->b.c[97+tid] = 97+tid;    // 97 is 'a'
    printf("tid %d wrote %c at f->b.c[%d]\n", tid, tmp->b.c[97+tid], 97+tid);   
    tmp->b.c[10000] = '!'+tid;

    if (tid == 0) {
        tmp->b.c[13000] = '@';
        printf("tid %d wrote %c at f->b.c[13000]\n", tid, tmp->b.c[13000]);   
    }
  
    printf("tid %d wrote %c at f->b.c[10000]\n", tid, tmp->b.c[10000]);   
    pthread_mutex_unlock(&gm);
 
    free(args);
    pthread_exit(NULL);
}

int main(){
    pthread_mutex_init(&gm, NULL);
    pthread_t tid[3];    
    
    printf("Checking crash status\n");
    if ( isCrashed() ) {
        printf("I need to recover!\n");
        // Recover aggregate data structure
        struct foo *f = (struct foo*)nvmalloc(sizeof(struct foo), (char*)"f");
        nvrecover(f, sizeof(struct foo), (char *)"f");

        // Verify results
        printf("f->b.c[97] = %c\n", f->b.c[97]);        
        printf("f->b.c[98] = %c\n", f->b.c[98]);        
        printf("f->b.c[99] = %c\n", f->b.c[99]);        
        printf("f->b.c[10000] = %c\n", f->b.c[10000]);        
        printf("f->b.c[13000] = %c\n", f->b.c[13000]);        
        printf("f->id = %d\n", f->id);
        free(f);
    }
    else{    
        printf("Program did not crash before, continue normal execution.\n");

        // Assign magic numbers and character to the aggregate data structure
        struct foo *f = (struct foo*)nvmalloc(sizeof(struct foo), (char*)"f");
        memset(f->b.c, 0, sizeof(struct foo));
        printf("finish writing to values\n");                
       
        int i;
        for (i = 0; i < 3; i++) {
            struct thread_data *tmp = (struct thread_data*)malloc(sizeof(struct thread_data));
            tmp->tid = i;
            tmp->local_data = f;
            pthread_create(&tid[i], NULL, t, (void*)tmp);
        }

        pthread_mutex_lock(&gm);
        f->id = 12345;
        f->b.c[10000] = '$';
        printf("main wrote $ to f->b.c[10000]\n");
        pthread_mutex_unlock(&gm);

        pthread_mutex_lock(&gm);
        f->b.c[10000] = '$';
        printf("main wrote $ to f->b.c[10000] again\n");
        pthread_mutex_unlock(&gm);

        for (i = 0; i < 3; i++) {
            pthread_join(tid[i], NULL);
        }

        printf("f->b.c[97] = %c\n", f->b.c[97]);        
        printf("f->b.c[98] = %c\n", f->b.c[98]);        
        printf("f->b.c[99] = %c\n", f->b.c[99]);        
        printf("f->b.c[10000] = %c\n", f->b.c[10000]);        
        printf("f->b.c[13000] = %c\n", f->b.c[13000]);        
        printf("f->id = %d\n", f->id);

        // Crash the program
        printf("internally abort!\n");
        abort(); 
    }

    printf("-------------main exits-------------------\n");
    return 0;
}
