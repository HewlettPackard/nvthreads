// Adapted from Michael & Scott, "Simple, Fast, and Practical Non-Blocking
// and Blocking Concurrent Queue Algorithms", PODC 1996.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include <sys/time.h>

#define NUM_ITEMS 100000

extern "C" {
    #include "store_sync.h"
}

typedef struct node_t
{
    int val;
    struct node_t * next;
}node_t;

typedef struct queue_t 
{
    node_t * head;
    node_t * tail;
    pthread_mutex_t * head_lock;
    pthread_mutex_t * tail_lock;
}queue_t;

queue_t * Q;

int ready = 0;
int done = 0;

pthread_mutex_t ready_lock;
pthread_mutex_t done_lock;

uint32_t queue_rgn_id;

void traverse();

void initialize()
{
    node_t * node =
        (node_t *) malloc(sizeof(node_t));
    assert(node);
    node->val = -1; // dummy value
    STSY_full_fence();
    STSY_clflush((intptr_t) &node->val);
    STSY_full_fence();

    node->next = NULL;
    STSY_full_fence();
    STSY_clflush((intptr_t) &node->next);
    STSY_full_fence();

    
    Q = (queue_t *) malloc(sizeof(queue_t));
    assert(Q);

    Q->head_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    Q->tail_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));

    Q->head = node;
    STSY_full_fence();
    STSY_clflush((intptr_t) &Q->head);
    STSY_full_fence();
    
    Q->tail = node;
    STSY_full_fence();
    STSY_clflush((intptr_t) &Q->tail);
    STSY_full_fence();
}

// not thread safe
void traverse()
{
    assert(Q);
    assert(Q->head);
    assert(Q->tail);

    node_t * t = Q->head;
    fprintf(stderr, "Contents of existing queue: ");
    while (t)
    {
        fprintf(stderr, "%p %d ", t, t->val);
        t = t->next;
    }
    fprintf(stderr, "\n");
}

void enqueue(int val)
{
    node_t * node =
        (node_t *) malloc(sizeof(node_t));
    assert(node);

    node->val = val;
    STSY_full_fence();
    STSY_clflush((intptr_t) &node->val);
    STSY_full_fence();

    node->next = NULL;
    STSY_full_fence();
    STSY_clflush((intptr_t) &node->next);
    STSY_full_fence();

    pthread_mutex_lock(Q->tail_lock);
    Q->tail->next = node;
    STSY_full_fence();
    STSY_clflush((intptr_t) &Q->tail->next);
    STSY_full_fence();
    Q->tail = node;
    STSY_full_fence();
    STSY_clflush((intptr_t) &Q->tail);
    STSY_full_fence();
    pthread_mutex_unlock(Q->tail_lock);
}

int dequeue(int * valp)
{
    pthread_mutex_lock(Q->head_lock);
    // note that not everything of type node_t is persistent
    // In the following statement, we have a transient pointer to
    // persistent data which is just fine. If there is a crash, the
    // transient pointer goes away.
    node_t * node = Q->head; 
    node_t * new_head = node->next;
    if (new_head == NULL)
    {
        pthread_mutex_unlock(Q->head_lock);
        return 0;
    }
    *valp = new_head->val;
    Q->head = new_head;
    STSY_full_fence();
    STSY_clflush((intptr_t) &Q->head);
    STSY_full_fence();

    pthread_mutex_unlock(Q->head_lock);
    return 1;
}

void * do_work()
{
    // TODO All lock/unlock must be instrumented, regardless of whether the
    // critical sections have accesses to persistent locations
    pthread_mutex_lock(&ready_lock);
    ready = 1;
    pthread_mutex_unlock(&ready_lock);

    int global_count = 0;
    int count = 0;
    int t = 0;
    while (1)
    {
        int val;
        int status = dequeue(&val);
        if (status)
        {
            ++ count;
            ++ global_count;
        }
        else
        {
            count = 0;
            if (t) break;
        }

        pthread_mutex_lock(&done_lock);
        t = done;
        pthread_mutex_unlock(&done_lock);
    }
    fprintf(stderr, "Total # items dequeued is %d\n", global_count);
    return 0;
}

int main()
{
    pthread_t thread;
    struct timeval tv_start;
    struct timeval tv_end;
    
    assert(!gettimeofday(&tv_start, NULL));
    
    initialize();

    pthread_create(&thread, 0, (void *(*)(void *))do_work, 0);

    // wait for the child to be ready
    int t = 0;
    while (!t)
    {
        pthread_mutex_lock(&ready_lock);
        t = ready; 
        pthread_mutex_unlock(&ready_lock);
    }

    int i;
    for (i = 0; i < NUM_ITEMS; ++i)
        enqueue(i);

    pthread_mutex_lock(&done_lock);
    done = 1;
    pthread_mutex_unlock(&done_lock);

    pthread_join(thread, NULL);

    assert(!gettimeofday(&tv_end, NULL));
    fprintf(stderr, "time elapsed %ld us\n",
            tv_end.tv_usec - tv_start.tv_usec +
            (tv_end.tv_sec - tv_start.tv_sec) * 1000000);
    fprintf(stderr, "Num of clflush: %d\n", NUM_CLFLUSH);
    fprintf(stderr, "Num of sync clflush: %d\n", NUM_SYNC_CLFLUSH);
    fprintf(stderr, "Num of syncs: %d\n", NUM_SYNCS);
    return 0;
}
