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

    int i;
    for (i = 0; i < NumBlanks; ++i) tmp_s[i] = ' ';

    NVM_MEMCPY(node->Data + num_digits, tmp_s, NumBlanks);
    NVM_STR2(node->Data[len], '\0', sizeof(char)*8);
    return node;
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

long printSLL()
{
    long sum = 0;
    Node *tnode = SLL;
    while (tnode)
    {
//        fprintf(stderr, "%s ", tnode->Data);
        sum += atoi(tnode->Data);
        tnode = tnode->Next;
    }
//    fprintf(stderr, "\n");
    return sum;
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
        
        for (int i=0; i < total_nodes_inserted; i++)
        {
            
            NVM_STR2(insert_after->Next, new_nodes[i], sizeof(insert_after->Next)*8);
            insert_after = new_nodes[i]->Next;            
        }

        NVM_END_DURABLE();
    }
}

int main(int argc, char *argv[])
{
    struct timeval tv_start;
    struct timeval tv_end;
    gettimeofday(&tv_start, NULL);
    
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
    InsertAfter = SLL;
    InsertValue = NumItems/2+1;
    
    long j; 
    
    pthread_t insert_threads[T];
    int rc;

    for (j=0; j < T; j++)
    {
        rc = pthread_create(&insert_threads[j], NULL, insertPass2, (void *) j);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (j=0; j < T; j++)
    {
        pthread_join(insert_threads[j], NULL);
    }

    fprintf(stderr, "Sum of elements is %ld\n", printSLL());

    NVM_CloseRegion(sll_rgn_id);
    NVM_Finalize();

    gettimeofday(&tv_end, NULL);
     

    fprintf(stderr, "time elapsed %ld us\n",
            tv_end.tv_usec - tv_start.tv_usec +
            (tv_end.tv_sec - tv_start.tv_sec) * 1000000);

#ifdef NVM_STATS
    NVM_PrintNumFlushes();
    NVM_PrintStats();
#endif
    
    return 0;
}
