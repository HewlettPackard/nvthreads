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
    char c[5000];
};
struct foo{
    int id;
    struct bar b;
};

void *t(void *args){
    pthread_exit(NULL);
}

int main(){
    pthread_t tid1;    
    
    printf("Checking crash status\n");
    if ( isCrashed() ) {
        printf("I need to recover!\n");
        // Recover aggregate data structure
        struct foo *f = (struct foo*)nvmalloc(sizeof(struct foo), (char*)"f");
        nvrecover(f, sizeof(struct foo), (char *)"f");

        // Verify results
        printf("f->b.c[4096] = %c\n", f->b.c[4096]);        
        printf("f->id = %d\n", f->id);
        free(f);
    }
    else{    
        printf("Program did not crash before, continue normal execution.\n");
        pthread_create(&tid1, NULL, t, NULL); 
        
        // Assign magic numbers and character to the aggregate data structure
        struct foo *f = (struct foo*)nvmalloc(sizeof(struct foo), (char*)"f");
        f->id = 12345;
        f->b.c[4096] = '$';

        printf("finish writing to values\n");

        pthread_join(tid1, NULL);

        // Crash the program
        printf("internally abort!\n");
        abort(); 
    }

    printf("-------------main exits-------------------\n");
    return 0;
}
