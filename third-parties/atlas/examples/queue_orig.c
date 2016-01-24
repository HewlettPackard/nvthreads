// Adapted from Michael & Scott, "Simple, Fast, and Practical Non-Blocking
// and Blocking Concurrent Queue Algorithms", PODC 1996.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>

#define NUM_ITEMS 100000

typedef struct node_t
{
    int val;
    struct node_t * next;
}node_t;

typedef struct queue_t 
{
    node_t * head;
    node_t * tail;
    pthread_mutex_t *head_lock;
    pthread_mutex_t *tail_lock;
}queue_t;

queue_t * Q;

int ready = 0;
int done = 0;

pthread_mutex_t ready_lock;
pthread_mutex_t done_lock;

void initialize()
{
    Q = (queue_t *) malloc(sizeof(queue_t));
    Q->head = NULL;
    Q->tail = NULL;
    Q->head_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(Q->head_lock, NULL);
    Q->tail_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(Q->tail_lock, NULL);
    
    node_t * node = (node_t *) malloc(sizeof(node_t));
    node->val = -1; // dummy value
    node->next = NULL;

    Q->head = Q->tail = node;
}

void enqueue(int val)
{
    node_t * node = (node_t *) malloc(sizeof(node_t));
    node->val = val;
    node->next = NULL;

    pthread_mutex_lock(Q->tail_lock);
    Q->tail->next = node;
    Q->tail = node;
    pthread_mutex_unlock(Q->tail_lock);
}

int dequeue(int * valp)
{
    pthread_mutex_lock(Q->head_lock);
    node_t * node = Q->head;
    node_t * new_head = node->next;
    if (new_head == NULL)
    {
        pthread_mutex_unlock(Q->head_lock);
        return 0;
    }
    *valp = new_head->val;
    Q->head = new_head;
    pthread_mutex_unlock(Q->head_lock);

    free(node);
    return 1;
}

void * do_work()
{
    pthread_mutex_lock(&ready_lock);
    ready = 1;
    pthread_mutex_unlock(&ready_lock);

    int global_count = 0;
    int t = 0;
    while (1)
    {
        int val;
        int status = dequeue(&val);
        if (status) ++ global_count;
        else if (t) break;
        
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
    

    gettimeofday(&tv_start, NULL);
    
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

    fprintf(stderr, "Total # items enqueued is %d\n", NUM_ITEMS);
    
    gettimeofday(&tv_end, NULL);
    fprintf(stderr, "time elapsed %ld us\n",
            tv_end.tv_usec - tv_start.tv_usec +
            (tv_end.tv_sec - tv_start.tv_sec) * 1000000);
    
    return 0;
}
