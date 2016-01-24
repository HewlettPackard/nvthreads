#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>
#include <inttypes.h>

#define LOOP_COUNT 1000000

inline uint64_t rdtsc() {
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return lo | ((uint64_t)hi << 32);
}

void *foo()
{
    return 0;
}

int main(int argc, char *argv[])
{
    int *arr;
    int count = 0;
    struct timeval tv_start;
    struct timeval tv_end;
    pthread_t thread;

    pthread_create(&thread, 0, (void *(*)(void*))foo, 0);
    
    arr = (int*)malloc(LOOP_COUNT*sizeof(int));

    assert(!gettimeofday(&tv_start, NULL));

    uint64_t start = rdtsc();
    
    int i;
    for (i=0; i<LOOP_COUNT; ++i)
        arr[i] = i;

    uint64_t end = rdtsc();
    
    assert(!gettimeofday(&tv_end, NULL));

    for (i=0; i<LOOP_COUNT; ++i)
        count += arr[i];
    fprintf(stderr, "Sume of elements is %d\n", count);
    fprintf(stderr, "time elapsed %ld us\n",
            tv_end.tv_usec - tv_start.tv_usec +
            (tv_end.tv_sec - tv_start.tv_sec) * 1000000);
    fprintf(stderr, "cycles: %ld\n", end - start);

    return 0;
}

    
