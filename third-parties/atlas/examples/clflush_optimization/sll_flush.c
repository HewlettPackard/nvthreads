// Compile example: g++ sll.c
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <assert.h>
#include <string.h>
#include <complex>
#include <sys/time.h>

extern "C" {
    #include "store_sync.h"
}

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


Node *SLL = NULL;
Node *InsertAfter = NULL;

int cache_line_size = 64;
uint64_t cache_line_mask = 0xffffffffffffffff - cache_line_size + 1;

static __inline void psync(void *start_addr, size_t sz)
{
    if (sz <= 0) return;

    char *last_addr = (char*)start_addr + sz - 1;

    char *cacheline_addr =
        (char*)(((uint64_t)start_addr) & cache_line_mask);
    char *last_cacheline_addr =
        (char*)(((uint64_t)last_addr) & cache_line_mask);

    STSY_full_fence();
    do {
        NUM_SYNC_CLFLUSH++;
        STSY_clflush((intptr_t) cacheline_addr);
        cacheline_addr += cache_line_size;
    }while (cacheline_addr < last_cacheline_addr+1);
    STSY_full_fence();
}


Node *createSingleNode(int Num, int NumBlanks)
{
    Node *node = (Node *) malloc(sizeof(Node));
    int num_digits = log10(Num)+1;
    int len = num_digits+NumBlanks;
    char* d = (char*) malloc((len+1)*sizeof(char));
                                  
    node->Data = d;
    STSY_full_fence();
    STSY_clflush((intptr_t) &node->Data);
    STSY_full_fence();
    
    char *tmp_s = (char *) alloca(
        (num_digits > NumBlanks ? num_digits+1 : NumBlanks+1)*sizeof(char));
    sprintf(tmp_s, "%d", Num);

    memcpy(node->Data, tmp_s, num_digits);
    psync(node->Data, num_digits);
    
    int i;
    for (i = 0; i < NumBlanks; ++i) tmp_s[i]= ' ';
    
    memcpy(node->Data + num_digits, tmp_s, NumBlanks);
    psync(node->Data + num_digits, NumBlanks);
    
    node->Data[len] = '\0';
    STSY_full_fence();
    STSY_clflush((intptr_t) &node->Data[num_digits+NumBlanks]);
    STSY_full_fence();
 
    return node;
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

Node *insertPass1(int Num, int NumBlanks, int NumFAI) 
{
    Node *node = createSingleNode(Num, NumBlanks);
    
    // In pass 1, the new node is the last node in the list
    node->Next=NULL;
    STSY_full_fence();
    STSY_clflush((intptr_t) &node->Next);
    STSY_full_fence();

    if (!SLL)
    {
        SLL =  node;
        STSY_full_fence();
        STSY_clflush((intptr_t) &SLL);
        STSY_full_fence();
        
        InsertAfter = node;
    }
    else
    {
        InsertAfter->Next = node;
        STSY_full_fence();
        STSY_clflush((intptr_t) &InsertAfter->Next);
        STSY_full_fence();
        
        InsertAfter = node;
    }
    
    return node;
}

Node *insertPass2(int Num, int NumBlanks, int NumFAI)
{
    Node** new_nodes = (Node**) malloc (NumFAI*sizeof(Node*));
    Node* list_iter = InsertAfter->Next;
    for (int i=0; i < NumFAI; i++)
    {
        new_nodes[i] = createSingleNode(Num + i, NumBlanks);
        if (list_iter != NULL)
        {
            new_nodes[i]->Next = list_iter;
            STSY_full_fence();
            STSY_clflush((intptr_t) &new_nodes[i]->Next);
            STSY_full_fence();
            
        }
        else
            break;
        list_iter = list_iter->Next;
    }

    for (int i=0; i < NumFAI; i++)
    {
        InsertAfter->Next = new_nodes[i];
        STSY_full_fence();
        STSY_clflush((intptr_t) &InsertAfter->Next);
        STSY_full_fence();
        
        if (new_nodes[i]->Next != NULL)
             InsertAfter = new_nodes[i]->Next;
        else 
        {
             InsertAfter = new_nodes[i];
             break;
        }
    }
    
    return InsertAfter;
}

Node* deletePass1(int Num, int NumItems, int NumFAI)
{
    int n = Num - NumFAI + 1;
    int start_value = n < NumItems/2 + 1 ?  NumItems/2 + 1 : n;
    
    SearchResult* result =  searchNode(start_value);
    Node* prev = result->Prev;
    Node* current;
 
    for (int i=start_value; i <= Num; ++i)
    {
        current = prev->Next;
        if (atoi(current->Data) == i)
        {
            prev->Next = current->Next;
            STSY_full_fence();
            STSY_clflush((intptr_t) &prev->Next);
            STSY_full_fence();
            
            prev = prev->Next;
        }
        else
           assert(false);
    }

    return result != NULL ? result->Match : NULL;
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

int main(int argc, char *argv[])
{
    struct timeval tv_start;
    struct timeval tv_end;
    assert(!gettimeofday(&tv_start, NULL));
    
    if (argc != 4)
    {
        fprintf(stderr, "usage: a.out numInts numBlanks numFAItems\n");
        exit(0);
    }
    int N = atoi(argv[1]);
    int K = atoi(argv[2]);
    int X = atoi(argv[3]);
    fprintf(stderr, "N = %d K = %d X = %d\n", N, K, X);
    assert(!(N%2) && "N is not even");

    int i;
    for (i = 1; i < N/2+1; ++i) insertPass1(i, K, X);
    InsertAfter = SLL;
    //for (i = N/2+1; i < N+1; ++i) insertPass2(i, K, X);

    for (i=N/2+1; i < N; i += X) insertPass2(i, K, X);
    int total_inserted = i - 1;
    insertPass2(i, K,  N - total_inserted);

    
    //printSLL();
 
    //for (i=N; i > N/2; i -= X) deletePass1(i, N, X);

    //printSLL();

    

    assert(!gettimeofday(&tv_end, NULL));
    fprintf(stderr, "time elapsed %ld us\n",
            tv_end.tv_usec - tv_start.tv_usec +
            (tv_end.tv_sec - tv_start.tv_sec) * 1000000);
    fprintf(stderr, "Num of clflush: %d\n", NUM_CLFLUSH); 
    fprintf(stderr, "Num of sync clflush: %d\n", NUM_SYNC_CLFLUSH); 
    fprintf(stderr, "Num of syncs: %d\n", NUM_SYNCS); 
    return 0;
}
