#ifndef _REGION_H
#define _REGION_H

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>

#include <map>
#include <utility>

#include "util.h"

using namespace std;

#ifdef PMM_OS
   #define FILESIZE 1048576   /* 10MB */
#else
//#define FILESIZE 1073741824 /* 1GB */
#define FILESIZE 8589934592 /* 8GB */
#endif
#define MAX_NUM_HEAPS 100 /* arbitrary at this point */
#define MAX_FREE_CATEGORY 128
#define ALLOC_TAB_MSK (MAX_NUM_HEAPS-1)
#define ALLOC_COLOR 128
#define ALLOC_SHIFT 3
#define UNS(a) ((uintptr_t)(a))

#define NVM_ALLOC_ALIGNMENT (2*(sizeof(size_t)))
#define NVM_ALIGN_MASK (NVM_ALLOC_ALIGNMENT-1)

typedef map<void *, bool> MemMap;
typedef map<size_t, MemMap> FreeList;

// For now, create a thread-safe interface for managing the table that
// tracks the persistent regions. In reality, it needs to be a service
// that uses distributed transactions. 

// A table of HeapEntry should reside in persistent memory
// The number of entries in the table is maintained at the beginning of
// this region.
typedef struct entry
{
    char name[MAXLEN];
    uint32_t id;
    // base address of mapping used the first time
    // For now, we attempt to map at the same address
    intptr_t * base_addr; 
    // curr_alloc_addr points to the
    // piece of memory within this persistent region from which the next
    // allocation should be serviced. 
    intptr_t * curr_alloc_addr;
    FreeList * free_list;
    // the file_desc should ideally be transient. Only used by close.
    // valid only if the file is mapped, otherwise it is meaningless
    int file_desc; 
    bool is_mapped;
    bool is_valid;
}HeapEntry;

// Every heap has its own lock for protection. However, there may be
// some false conflicts.
extern pthread_mutex_t AllocLockTab[MAX_NUM_HEAPS];

extern void * region_table_addr;

// This mapper maintains association between a region address range and the
// corresponding region id
extern MapInterval * volatile map_interval_p;

/* #define GetAllocLock(a) (AllocLockTab + (((UNS(a)+ALLOC_COLOR)        \
                                          >> ALLOC_SHIFT) & ALLOC_TAB_MSK))
*/
#define GetAllocLock(a) (AllocLockTab + (UNS(a) & ALLOC_TAB_MSK))

inline void AcquireRegionAllocLock(uint32_t rid)
{
    pthread_mutex_t *mtx = GetAllocLock(rid);
#if !defined(NDEBUG)
    int status =
#endif        
        pthread_mutex_lock(mtx);
    assert(!status);
}

inline void ReleaseRegionAllocLock(uint32_t rid)
{
    pthread_mutex_t *mtx = GetAllocLock(rid);
#if !defined(NDEBUG)
    int status =
#endif        
        pthread_mutex_unlock(mtx);
    assert(!status);
}

// Make these interfaces a part of the memory allocator. For now, just
// acquire a lock for any kind of reading and writing. Limit the max size
// of every region to something like 10 GB. Note that this table itself
// is a persistent region but it is not tracked since it can be mapped
// anywhere in the address space. Note that the recovery log structure goes
// into a special region, called "__nvm_system_recovery_logs__". One
// related question is during recovery, how does the recovery process know
// which regions to revert back? For this purpose, every undo log entry
// has the region id as well.

// mem points to actual memory allocated
// ptr stands for pointer returned to user code
#define mem2ptr(mem) ((void *)((char *)(mem) + 2*sizeof(size_t)))
#define ptr2mem(ptr) ((void *)((char *)(ptr) - 2*sizeof(size_t)))

void *nvm_alloc(size_t, intptr_t** /* ptr to start addr for allocation */);
void *nvm_alloc_from_free_list(size_t sz, FreeList *free_list, HeapEntry*);
void *nvm_alloc_cache_line_aligned(size_t sz, uint32_t rid);
void nvm_free(void*, bool);

// private interfaces

void NVM_SetupRegionTable(const char*);
uint32_t NVM_GetNumRegions();
void NVM_SetNumRegions(uint32_t);
HeapEntry * NVM_GetRegionMetaData(uint32_t);
pair<HeapEntry*,void*> NVM_FindRegionMetaData(
    const char*, bool ignore_attr=false);
void *NVM_ComputeNewBaseAddr();
int NVM_CreateNewMappedFile(
    const char * name, int flags, void * base_addr = 0);
pair<void *, int> NVM_MapExistingFile(
    const char * name, int flags, intptr_t * base_addr);
pair<void*,uint32_t> NVM_EnsureMapped(
    intptr_t *addr, bool ignore_mapped_attr=false);
pair<bool, uint32_t> NVM_GetRegionId(void * addr, size_t size);
uint32_t NVM_GetOpenRegionId(void *addr, size_t sz);
void NVM_CreateLoggingDirectory();
size_t NVM_get_actual_alloc_size(size_t);
void NVM_UnmapAndClose(HeapEntry *he, bool is_recover=false);
void NVM_UnmapUnlockAndClose(HeapEntry *he, bool is_recover=false);
HeapEntry * NVM_GetTablePtr();
void NVM_InsertToFreeList(HeapEntry * he, void * ptr);
void BuildFreeList(HeapEntry *rgn_entry);
pair<void*, bool> BuildPartialFreeList(size_t sz, HeapEntry *rgn_entry);
uint32_t NVM_CreateLogFile(const char *name);
void RegionDump(HeapEntry *);
void RegionTableDump();

#endif
