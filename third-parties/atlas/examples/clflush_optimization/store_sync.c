#include "store_sync.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef THREADS
    __thread intptr_t STSY_not_yet_flushed[STSY_TABLE_SZ];
#else
    intptr_t STSY_not_yet_flushed[STSY_TABLE_SZ];
#endif
    
int NUM_SYNC_CLFLUSH = 0;
int NUM_CLFLUSH = 0;
int NUM_SYNCS = 0;
void STSY_sync_all(void)
{
  unsigned i;

  STSY_full_fence();
  NUM_SYNCS++;
  for (i = 0; i < STSY_TABLE_SZ; ++i) {
    intptr_t *entry_ptr = STSY_not_yet_flushed + i;
    intptr_t entry = *entry_ptr;
    if (entry) {
      NUM_SYNC_CLFLUSH++;
      STSY_clflush(entry << STSY_LOG_MIN_LINE_SZ);
      *entry_ptr = 0;
    }
  }
  STSY_full_fence();
}



void* STSY_memcpy(void * destination, const void * source, size_t num )
{
    char* end = ((char*) destination) + num;
    char* p = (char*) destination;
    int fenced = 0;
    memcpy(destination, source, num);
    for (; p < end; p+= STSY_MIN_LINE_SZ)
    {
        intptr_t ln = STSY_cache_line(p); /* shifted address */ 
        intptr_t *entry_ptr = STSY_not_yet_flushed + STSY_hash(ln); 
        intptr_t entry = *entry_ptr; 
        assert(STSY_cache_line((char *)(p+1) - 1) == ln); /* aligned */
        if (entry != ln) 
        { 
            if (entry == 0) 
                *entry_ptr = ln;
            else 
            {
                if (!fenced) 
                {
                    STSY_full_fence();
                    fenced = 1;
                }
                STSY_clflush(entry << STSY_LOG_MIN_LINE_SZ); 
                *entry_ptr = ln; 
            }
        }
        assert(*entry_ptr == ln); 
    } 
}
