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
        nvcheckpoint();
        printf("finish writing to values\n");

        pthread_join(tid1, NULL);
        printf("internally abort!\n");
        abort(); 
    }

    printf("-------------main exits-------------------\n");
    return 0;
}
