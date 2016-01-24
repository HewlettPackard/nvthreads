#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <assert.h>
#include <string.h>
#include <complex>
#include <sys/time.h>
#include <pthread.h>

#include "atlas_api.h"
#include "atlas_alloc.h"

/// N O T E ////////////////////////////////////////////////////////////////////////////////////////
// This program will only work with NumItems such that NumItems is perfectly divisible by NumRegions.
// NumRegions is calculated at 10% of NumItems
///////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct Node
{
    char *Data;
    struct Node *Next;
}Node;

typedef struct SearchResult
{
    Node* Match;
    Node* Prev;
}SearchResult;

uint32_t sll_rgn_id;


Node *SLL = NULL;
int NumItems = 0;
int NumFAI = 0;
int NumBlanks = 0;
pthread_mutex_t InsertPosLock = PTHREAD_MUTEX_INITIALIZER;
Node *InsertAfter = NULL;
int InsertValue;

pthread_mutex_t DeletePosLock = PTHREAD_MUTEX_INITIALIZER;
int DeleteValue;

pthread_mutex_t DeleteStartLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t DeleteCondVar = PTHREAD_COND_INITIALIZER;
bool StartDelete = false;

int NumRegions = 0;
pthread_mutex_t** RegionLocks;


Node *createSingleNode(int Num)
{
    Node *node = (Node *) nvm_alloc(sizeof(Node), sll_rgn_id);
    int num_digits = log10(Num)+1;
    int len = num_digits+NumBlanks;
    char* d = (char*) nvm_alloc((len+1)*sizeof(char), sll_rgn_id);

    NVM_STR2(node->Data, d, sizeof(char*)*8);

    char *tmp_s = (char *) alloca(
        (num_digits > NumBlanks ? num_digits+1 : NumBlanks+1)*sizeof(char));
    sprintf(tmp_s, "%d", Num);

    NVM_MEMCPY(node->Data, tmp_s, num_digits);
    NVM_STR2(node->Data[num_digits], '\0', sizeof(char)*8);

    tmp_s[0] = '\0';
    int i;
    for (i = 0; i < NumBlanks; ++i) strcat(tmp_s, " ");

    NVM_MEMCPY(node->Data + num_digits, tmp_s, NumBlanks);
    NVM_STR2(node->Data[num_digits+NumBlanks], '\0', sizeof(char)*8);
    return node;
}

int getRegion(int d)
{
    int data;
    data = d > NumItems/2 ? d - NumItems/2 : d;
    if (data == 1)
        return 0;
    int perRegion = NumItems / (2*NumRegions);
    return (data - 1) / perRegion;
}

int getRegion(Node* node)
{
    return getRegion(atoi(node->Data));
}

SearchResult* searchNode(int Num)
{
    Node* prev = NULL;
    for (Node* iter = SLL; iter != NULL;)
    {
        
        if (Num == atoi(iter->Data))
        {
            SearchResult* result = (SearchResult*) malloc(sizeof(SearchResult));
            result->Match = iter;
            result->Prev = prev;
            return result;
        }
        prev = iter;
        iter = iter->Next;
    }
    return NULL;
}

void flushNode(Node* node)
{
    NVM_FLUSH(&node->Data);
    if (isOnDifferentCacheLine(&node->Data, &node->Next))
        NVM_FLUSH(&node->Next);
}

Node *insertPass1(int Num)
{
    Node *node = createSingleNode(Num);

    // In pass 1, the new node is the last node in the list
    NVM_STR2(node->Next, NULL, sizeof(node->Next)*8);

    NVM_BEGIN_DURABLE();

    if (!SLL)
    {
        NVM_STR2(SLL, node, sizeof(SLL)*8); // write-once
        InsertAfter = node;
    }
    else
    {
        NVM_STR2(InsertAfter->Next, node, sizeof(InsertAfter->Next)*8);
        InsertAfter = node;
    }
    NVM_END_DURABLE();

    return node;
}


void printSLL()
{
    Node *tnode = SLL;
    while (tnode)
    {
        fprintf(stderr, "%s ", tnode->Data);
        tnode = tnode->Next;
    }
    fprintf(stderr, "\n");
}

void* insertPass2(void *threadid)
{
    Node* insert_after;
    int start_value;
    while (true)
    {
        pthread_mutex_lock(&InsertPosLock);
        
        insert_after = InsertAfter;
        start_value = InsertValue;
        if (start_value > NumItems)
        {
            pthread_mutex_unlock (&InsertPosLock);
            pthread_exit(NULL);
        }    
        InsertValue += NumFAI;
        for (int i=0; i < NumFAI && InsertAfter != NULL; ++i)
            InsertAfter = InsertAfter->Next;

        pthread_mutex_unlock (&InsertPosLock);
      
        if (start_value >= (3 * NumItems) / 4)
        {
           pthread_mutex_lock(&DeleteStartLock);
           StartDelete = true;
           pthread_mutex_unlock(&DeleteStartLock);
           pthread_cond_broadcast(&DeleteCondVar);
        }
 
        int e = start_value + NumFAI -1;
        int end_value = e > NumItems ? NumItems : e;
        int total_nodes_inserted = end_value-start_value+1; 

        Node** new_nodes = (Node**) malloc ((total_nodes_inserted)*sizeof(Node*));
        Node* list_iter = insert_after->Next;
        
        for (int i = 0; i < total_nodes_inserted; i++)
        {
             new_nodes[i] = createSingleNode(start_value + i);
             if (list_iter != NULL)
             {
                 NVM_STR2(new_nodes[i]->Next, list_iter, sizeof(list_iter)*8);
                 list_iter = list_iter->Next;
             }
        }

        NVM_BEGIN_DURABLE();
        
        int prev_region = getRegion(insert_after);
        int current_region;
        pthread_mutex_lock(RegionLocks[prev_region]);
        for (int i=0; i < total_nodes_inserted; i++)
        {
            current_region = getRegion(insert_after);
            if (current_region != prev_region)
            {
                pthread_mutex_lock(RegionLocks[current_region]);
                pthread_mutex_unlock(RegionLocks[prev_region]);
                prev_region = current_region;
            }
            
            NVM_STR2(insert_after->Next, new_nodes[i], sizeof(insert_after->Next)*8);
            insert_after = new_nodes[i]->Next;            
        }
        pthread_mutex_unlock(RegionLocks[current_region]);

        NVM_END_DURABLE();
    }
}

void* deletePass(void *threadid)
{
    int end_value;

    while (true)
    {
        pthread_mutex_lock(&DeletePosLock);

        end_value = DeleteValue;
        if (end_value < NumItems/2 + 1)
        {
            pthread_mutex_unlock(&DeletePosLock);
            pthread_exit(NULL);
        }
        DeleteValue -= NumFAI;
        pthread_mutex_unlock(&DeletePosLock);

        pthread_mutex_lock(&DeleteStartLock);
        while(!StartDelete)
            pthread_cond_wait(&DeleteCondVar, &DeleteStartLock);
        pthread_mutex_unlock(&DeleteStartLock);

        int n = end_value - NumFAI + 1;
        int start_value = n < NumItems/2 + 1 ?  NumItems/2 + 1 : n;

        SearchResult* result =  searchNode(start_value);
        printf("Delete starting %d\n", start_value);
        if (result == NULL) 
        {
            printf("Delete starting at %d, not found\n", start_value);
            continue;
        }

        Node* prev = result->Prev;
        Node* current = result->Match;
        int prev_region = getRegion(prev);
        int current_region; 
        bool abort = false;
        NVM_BEGIN_DURABLE();
        pthread_mutex_lock(RegionLocks[prev_region]);
        for (int i=start_value;; ++i)
        {
            current = prev->Next;
            current_region = getRegion(current);
            if (prev_region != current_region)
                pthread_mutex_lock(RegionLocks[current_region]);                
            if (atoi(current->Data) == i)
            {
                NVM_STR2(prev->Next, current->Next, sizeof(prev->Next)*8);
                prev = prev->Next;
            }
            else
                assert(false);

            if (prev_region != current_region)
                pthread_mutex_unlock(RegionLocks[prev_region]);
            if (abort || i == end_value) break;
            prev_region = getRegion(prev);
            if (prev_region != current_region)
            {
                pthread_mutex_lock(RegionLocks[prev_region]);
                pthread_mutex_unlock(RegionLocks[current_region]);   
            }
        }
        pthread_mutex_unlock(RegionLocks[current_region]);
        NVM_END_DURABLE();
    }
}



int main(int argc, char *argv[])
{
    struct timeval tv_start;
    struct timeval tv_end;
    assert(!gettimeofday(&tv_start, NULL));
    
    if (argc != 5)
    {
        fprintf(stderr, "usage: a.out numInts numBlanks numFAItems numThreads\n");
        exit(0);
    }
    NumItems = atoi(argv[1]);
    NumBlanks = atoi(argv[2]);
    NumFAI = atoi(argv[3]);
    int T = atoi(argv[4]);
    fprintf(stderr, "N = %d K = %d X = %d T = %d\n", NumItems, NumBlanks, NumFAI, T);
    assert(!(NumItems%2) && "N is not even");

    NVM_Initialize();
    sll_rgn_id = NVM_FindOrCreateRegion("sll_ll", O_RDWR, NULL);

    int i;
    for (i = 1; i < NumItems/2+1; ++i) insertPass1(i);
    printSLL();
    InsertAfter = SLL;
    InsertValue = NumItems/2+1;
    DeleteValue = NumItems;
    
    // num locks = 10% of the NumItems. Calculating the ceiling of NumItems / 10
    NumRegions = (NumItems + 10 - 1) / 10;  
    RegionLocks = (pthread_mutex_t**) malloc (NumRegions * sizeof(pthread_mutex_t*));

    long j; 
    for (j=0; j < NumRegions; j++)
    {
        RegionLocks[j] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(RegionLocks[j], NULL);
    }
    
    pthread_t insert_threads[T];
    pthread_t delete_threads[T];
    int rc;

    for (j=0; j < T; j++)
    {
        rc = pthread_create(&insert_threads[j], NULL, insertPass2, (void *) j);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
        rc = pthread_create(&delete_threads[j], NULL, deletePass, (void *) j);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (j=0; j < T; j++)
    {
        pthread_join(insert_threads[j], NULL);
        pthread_join(delete_threads[j], NULL);
    }

    printSLL();

    NVM_CloseRegion(sll_rgn_id);
    NVM_Finalize();

    assert(!gettimeofday(&tv_end, NULL));
     
    fprintf(stderr, "time elapsed %ld us\n",
            tv_end.tv_usec - tv_start.tv_usec +
            (tv_end.tv_sec - tv_start.tv_sec) * 1000000);

#ifdef NVM_STATS
    NVM_PrintNumFlushes();
    NVM_PrintStats();
#endif
    
    return 0;
}
