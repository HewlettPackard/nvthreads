#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/time.h>
#include<assert.h>
using namespace std;


extern "C" {
   # include "store_sync.h"
}

#define PAGESIZE 4096


int main() {
    const int NO_PAGES = 200;
     
    int num_of_items = (NO_PAGES * PAGESIZE) / sizeof(long);

    long* test_array = (long*) calloc(num_of_items, sizeof(long));

    timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);
    for (int i=0; i < num_of_items; ++i)
        STSY_store_long(&test_array[i], (long)(i*i+20));
    STSY_sync_all();
    assert(!gettimeofday(&tv_end, NULL));

    fprintf(stderr, "time elapsed %ld us\n",
            tv_end.tv_usec - tv_start.tv_usec +
            (tv_end.tv_sec - tv_start.tv_sec) * 1000000);
}

