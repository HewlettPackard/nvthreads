#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

#include "atlas_api.h"
#include "atlas_alloc.h"

#define LOOP_COUNT 1000000

void bar();

uint32_t stores_rgn_id;

inline uint64_t rdtsc() {
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return lo | ((uint64_t)hi << 32);
}

typedef int ARR_TYPE;

int main(int argc, char *argv[])
{
    ARR_TYPE *arr;
    int count = 0;
    struct timeval tv_start;
    struct timeval tv_end;

    NVM_Initialize();
    stores_rgn_id = NVM_FindOrCreateRegion("stores", O_RDWR, NULL);
    arr = (ARR_TYPE*)nvm_alloc(LOOP_COUNT*sizeof(ARR_TYPE), stores_rgn_id);
    
    assert(!gettimeofday(&tv_start, NULL));

    uint64_t start = rdtsc();
    
    int i;

//    NVM_BEGIN_DURABLE();
    for (i=0; i<LOOP_COUNT; i++)
    {
        NVM_BEGIN_DURABLE();
        
        NVM_STR2(arr[i], i, sizeof(arr[i])*8);
        
        NVM_END_DURABLE();
    }
//    NVM_END_DURABLE();

    assert(!gettimeofday(&tv_end, NULL));

    uint64_t end = rdtsc();

    for (i=0; i<LOOP_COUNT; ++i)
        count += arr[i];

    NVM_CloseRegion(stores_rgn_id);
    NVM_Finalize();
    
    fprintf(stderr, "Sume of elements is %d\n", count);
    fprintf(stderr, "time elapsed %ld us\n",
            tv_end.tv_usec - tv_start.tv_usec +
            (tv_end.tv_sec - tv_start.tv_sec) * 1000000);
    fprintf(stderr, "cycles: %ld\n", end - start);
    
#ifdef NVM_STATS
    NVM_PrintNumFlushes();
    NVM_PrintStats();
#endif    
    
    return 0;
}

    
