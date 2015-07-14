#ifndef _LOG_STRUCTURE_H
#define _LOG_STRUCTURE_H

#include <stdint.h>
#include <pthread.h>
#include <vector>
#include <set>
#include <map>

#include "util.h"
#include "atomic_ops.h"

using namespace std;

enum LE_TYPE {LE_dummy, LE_acquire, LE_rwlock_rdlock, LE_rwlock_wrlock,
              LE_begin_durable, LE_release, LE_rwlock_unlock,
              LE_end_durable, LE_str, LE_memset, LE_memcpy, LE_memmove};

typedef map<void*,uint64_t> MapOfLockInfo;

// Structure of an undo log entry
typedef struct LogEntry
{
    void *Addr_;
    uintptr_t ValueOrPtr_; /* either value or ptr (for an acquire-node) */
    LogEntry * volatile Next_; /* ptr to next log entry in program order */
    size_t Size_:60; /* size of the update or for synchronization operations,
                        the CB generation number */
    LE_TYPE Type_:4;
}LogEntry;

typedef map<LogEntry *, bool> Log2Bool;

#define LAST_LOG_ELEM(p) ((char*)(p)+24)

// Here LogAddr_ is the address of the log entry of the last release
// operation of the corresponding synchronization object.
typedef struct OwnerInfo
{
    // Make it volatile since as we find a new release operation on
    // the same lock, we find the existing entry and change it atomically
    LogEntry * volatile LogAddr_;

    // The following map contains the set of locks (and their counts)
    // that *this* lock *depends* on. This *dependence* relation is
    // established at the point *this* lock is released. At any point
    // of time, there can be at most one writer since only one thread
    // can release *this* lock. But there can be multiple readers who
    // may be examining this map (e.g. rw-locks). Hence LockInfoPtr_
    // is volatile and is atomically updated.
    MapOfLockInfo * volatile LockInfoPtr_;
    
    // The Next_ field typically will be volatile but in this case, it
    // turns out that this field is updated before it is published. It is
    // also write-once.
    OwnerInfo * Next_;
}OwnerInfo;

typedef struct LockCount
{
    void *LockAddr_;
    uint64_t Count_;
    LockCount *Next_;
}LockCount;

// Log structure header: A shared statically allocated header points at
// the start of this structure, so it is available to all threads. Every
// entry has a pointer to a particular thread's log structure and a next
// pointer
typedef struct LogStructure
{
    LogEntry *Le_; // points to first non-deleted thread-specific log entry 
    // Insertion happens at the head, so the following is write-once before
    // getting published.
    LogStructure * Next_;
}LogStructure;

#define TAB_SIZE (1 << 10)
#define TAB_MASK (TAB_SIZE-1)
#define COLOR (128)
#define O_SHIFT (3)
#define UNS(a) ((uintptr_t)(a))

#define FLUSH_TAB_SIZE 8
#define FLUSH_TAB_MASK (FLUSH_TAB_SIZE-1)
// TODO the following should really be derived from cache line size
#define FLUSH_SHIFT 6

// Globals accessed only by the main thread
extern void * non_mutated_addr;
extern uint32_t nvm_logs_id;

// Shared globals

// Here's the static ownership table OwnerTab.
// needs to be volatile.
// semantically, a map from a lock-address to a 64-bit word.
// This word is a pointer to a singly-linked list. Each element of this
// list is a struct OwnerInfo which contains the <log_address, next_ptr> where
// log_address is the address of the log entry of the last release
// operation of the synchronization object corresponding to the lock-address.
extern intptr_t * volatile OwnerTab[TAB_SIZE];

extern intptr_t * volatile LockCountTab[TAB_SIZE];

extern pthread_mutex_t LockTab[TAB_SIZE];

// acquire this lock when printing something to stderr
extern pthread_mutex_t print_lock;

// This is the topmost header to the entire global log structure
extern LogStructure * volatile *log_structure_header_p;

// This flag indicates that the main threads are done
extern volatile AO_t all_done;

// Condition variable thru which user threads signal the helper thread
extern pthread_cond_t helper_cond;

// Mutex for the above condition variable
extern pthread_mutex_t helper_lock;

// The following maps an address into a pointer to an entry of OwnerTab table
#define O_ENTRY(a) (OwnerTab + (((UNS(a)+COLOR) >> O_SHIFT) & TAB_MASK))

#define LOCK_COUNT_TABLE_ENTRY(a) \
    (LockCountTab + (((UNS(a)+COLOR) >> O_SHIFT) & TAB_MASK))

// We need a lock for every element of the table OwnerTab. For now, as
// shown below, we keep a table of locks exactly the same size as
// OwnerTab, so that we can use the same index. 
#define O_LOCK(a) (LockTab + (((UNS(a)+COLOR) >> O_SHIFT) & TAB_MASK))

typedef vector<LogEntry*> LogEntryVec;

// These are reqd only by the implementation which is C++, so no C linkage
LogStructure *CreateLogStructure(LogEntry*);
LogEntry * CreateLogEntry(void *lock_address, LE_TYPE le_type);
LogEntry * CreateStrLogEntry(intptr_t *addr, size_t size);
LogEntry * CreateLogEntry(void * addr, size_t sz, LE_TYPE le_type);
void PublishLogEntry(LogEntry * le);
OwnerInfo * GetHeader(void * lock_address);
OwnerInfo * volatile * GetPointerToHeader(void * lock_address);
OwnerInfo * FindOwnerInfo(void * lock_address);
OwnerInfo * CreateNewOwnerInfo(LogEntry * le, const MapOfLockInfo & locks);
void AddLogToOwnerTable(LogEntry * le, const MapOfLockInfo & undo_locks);
LockCount *GetLockCountHeader(void *lock_address);
LockCount * volatile * GetPointertoLockCountHeader(void *lock_address);
LockCount *FindLockCount(void *lock_address);
LockCount *CreateNewLockCount(void *lock_address, uint64_t count);
void AddLockCount(void *lock_address, uint64_t count);
void HandleAcquireLockCount(void *lock_address);
void MarkEndTCS(LogEntry*);
void CollectCacheLines(SetOfInts*, void*, size_t);
void FlushCacheLines();
void FlushCacheLines(const LogEntryVec &);
void FlushCacheLines(const SetOfInts &);
void FlushCacheLinesUnconstrained(const SetOfInts &);
void SignalHelper();
bool canElideLogging();
void PrintLog();
void PrintLogEntry(LogEntry*);

inline bool isAcquire(LogEntry * le) 
{ return le->Type_ == LE_acquire; }

inline bool isAcquire(LE_TYPE le_type)
{ return le_type == LE_acquire; }

inline bool isRWLockRdLock(LogEntry * le)
{ return le->Type_ == LE_rwlock_rdlock; }

inline bool isRWLockRdLock(LE_TYPE le_type)
{ return le_type == LE_rwlock_rdlock; }

inline bool isRWLockWrLock(LogEntry * le)
{ return le->Type_ == LE_rwlock_wrlock; }

inline bool isRWLockWrLock(LE_TYPE le_type)
{ return le_type == LE_rwlock_wrlock; }

inline bool isBeginDurable(LogEntry *le)
{ return le->Type_ == LE_begin_durable; }

inline bool isBeginDurable(LE_TYPE le_type)
{ return le_type == LE_begin_durable; }

inline bool isRelease(LogEntry * le)
{ return le->Type_ == LE_release; }

inline bool isRelease(LE_TYPE le_type)
{ return le_type == LE_release; }

inline bool isRWLockUnlock(LogEntry * le)
{ return le->Type_ == LE_rwlock_unlock; }

inline bool isRWLockUnlock(LE_TYPE le_type)
{ return le_type == LE_rwlock_unlock; }

inline bool isEndDurable(LogEntry *le)
{ return le->Type_ == LE_end_durable; }

inline bool isEndDurable(LE_TYPE le_type)
{ return le_type == LE_end_durable; }

inline bool isStr(LogEntry * le)
{ return le->Type_ == LE_str; }

inline bool isStr(LE_TYPE le_type)
{ return le_type == LE_str; }

inline bool isMemset(LogEntry * le)
{ return le->Type_ == LE_memset; }

inline bool isMemset(LE_TYPE le_type)
{ return le_type == LE_memset; }

inline bool isMemcpy(LogEntry *le)
{ return le->Type_ == LE_memcpy; }

inline bool isMemcpy(LE_TYPE le_type)
{ return le_type == LE_memcpy; }

inline bool isMemmove(LogEntry *le)
{ return le->Type_ == LE_memmove; }

inline bool isMemmove(LE_TYPE le_type)
{ return le_type == LE_memmove; }

inline bool isMemop(LogEntry *le)
{ return le->Type_ == LE_memset || le->Type_ == LE_memcpy ||
        le->Type_ == LE_memmove; }

inline bool isMemop(LE_TYPE le_type)
{ return le_type == LE_memset || le_type == LE_memcpy ||
        le_type == LE_memmove; }

inline bool isStartSection(LogEntry *le)
{
    return isAcquire(le) || isRWLockRdLock(le) || isRWLockWrLock(le) ||
        isBeginDurable(le);
}

inline bool isStartSection(LE_TYPE le_type)
{
    return isAcquire(le_type) || isRWLockRdLock(le_type) ||
        isRWLockWrLock(le_type) || isBeginDurable(le_type);
}

inline bool isEndSection(LogEntry *le)
{
    return isRelease(le) || isRWLockUnlock(le) || isEndDurable(le);
}

inline bool isEndSection(LE_TYPE le_type)
{
    return isRelease(le_type) || isRWLockUnlock(le_type) ||
        isEndDurable(le_type);
}

void * helper();

inline uint64_t atlas_rdtsc() {
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return lo | ((uint64_t)hi << 32);
}

#endif
