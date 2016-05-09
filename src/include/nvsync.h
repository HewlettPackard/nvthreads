/*
  Copyright 2015-2016 Hewlett Packard Enterprise Development LP
  
  This program is free software; you can redistribute it and/or modify 
  it under the terms of the GNU General Public License, version 2 as 
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License 
  along with this program; if not, write to the Free Software 
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#ifndef __NVSYNC_H__
#define __NVSYNC_H__


/*
 * @file   nvsync.h
 * @brief  Main engine for synchronization management.
 * @author Terry Hsu terryhsu@purdue.edu
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "xdefines.h"
#include "list.h"
#include "xbitmap.h"
#include "xdefines.h"
#include "internalheap.h"
#include "real.h"
#include "prof.h"

#define MAX_THREADS 2048
#define MAX_MUTEX 1024
#define MAX_LOCKSET 1024

enum{
    COND_READY, // cond is ready for cond_wait request
    COND_WAIT,  // cond is not ready
    COND_BROADCAST  // cond is ready for all broadcast requests
};

class nvsync {
private:
    class lockSet;
    class mutexEntry;

    class lockSet {
    public:
        int lockSetID;          // ID of this lockSet
        unsigned long xact;     // transaction this lockset belongs to
        int requests;          // record the current number of locking requests in this lockSet
        mutexEntry *locks[MAX_MUTEX];   // mutex in this lockSet. MAX_MUTEX=1024
    };

    // Each pthread_mutex_t has one mutexEntry
    class mutexEntry { // Size of mutexEntry cannot be larger than pthread_mutex_t
    public:
        pthread_mutex_t *mutex;                // pointer to the actual mutex lock
        pthread_cond_t *cond;           // cond variable this mutex is associated with (could be 0)
        lockSet *lockset;    // record the lockSet this mutex is in
        int lockID;                 // the index this mutex is in mutexes[] and mutexEntries
        int waitingCount;            // Record the number of waiting thread
        int lockingThreadID;        // Record the threadID of the locking thread, if any.
    };

    // Each pthread_cond_t has one condEntry
    class condEntry {
    public:
        pthread_cond_t *cond;       // pointer to the actual cond variable
        int lockingThreadID;        // record the thread ID waiting mutex for this cond var
        volatile int status;                 // status of this cond variable
        int broadcastCount;         // number of thread already received the cond broadcast signal
    };

    class ThreadStatus {
	public:
		ThreadStatus(void * r, bool f) : retval(r), forked(f) {}
		ThreadStatus(void) {}
		volatile int tid;
		int threadIndex;
		void * retval;
		bool forked;
	};

    nvsync() :
        _initialized(false),
        _xact(0)
    { }

    bool _initialized;
    unsigned long _xact;   // global transaction counter

    // Record waitlist for mutexEntries
    bool waitList[MAX_MUTEX][MAX_THREADS];  

    // Record list of mutex pointers
    mutexEntry *mutexEntries[MAX_MUTEX];    

    // Record list of threads 
//  threadEntry threadEntries[MAX_THREADS]; // array index is the threadID recorded in xmemory
        
    // Record list of lockSet current in use, maximum number of lockSet supported is 1024
    lockSet locksets[MAX_LOCKSET];
    
    // Record list of in-use lockset;
    bool locksetInUse[MAX_LOCKSET];

    // Record which lockset a thread is currently using
    lockSet *ThreadLocksets[MAX_THREADS];

    // Record number of locks a thread is using
    int ThreadLockingCount[MAX_THREADS];

    // List of actual pthread mutexes
    pthread_mutex_t mutexes[MAX_MUTEX];


    // Shared mutex and condition variable for all threads.
    // They are useful to synchronize among all threads.
    pthread_mutex_t _mutex;
    pthread_cond_t cond;
    pthread_condattr_t _condattr;
    pthread_mutexattr_t _mutexattr;

    inline void lock(void) {
        determ::getInstance().lock();
    }

    inline void unlock(void) {
        determ::getInstance().unlock();
    }

public:

    /** nvsync engine functions 
    **/
    void initialize(void) {
        lprintf("initializing nvsync engine\n");

        // Set up with a shared attribute
        WRAP(pthread_mutexattr_init)(&_mutexattr);
        pthread_mutexattr_setpshared(&_mutexattr, PTHREAD_PROCESS_SHARED);
        WRAP(pthread_condattr_init)(&_condattr);
        pthread_condattr_setpshared(&_condattr, PTHREAD_PROCESS_SHARED);

        // Initialize the mutex
        WRAP(pthread_mutex_init)(&_mutex, &_mutexattr);
        WRAP(pthread_cond_init)(&cond, &_condattr);

        memset(waitList, 0, sizeof(bool) * MAX_MUTEX * MAX_THREADS);
        memset(mutexEntries, 0, sizeof(mutexEntry*) * MAX_MUTEX);
//      memset(threadEntries, 0, sizeof(threadEntry) * MAX_THREADS);
        memset(ThreadLocksets, 0, sizeof(lockSet) * MAX_THREADS); 
        memset(ThreadLockingCount, 0, sizeof(int) * MAX_THREADS);
        memset(mutexes, 0, sizeof(pthread_mutex_t) * MAX_MUTEX);
        memset(locksets, 0, sizeof(lockSet) * MAX_LOCKSET);
        memset(locksetInUse, 0, sizeof(bool) * MAX_LOCKSET);       
        _initialized = true;
    }
    void finalize(void) {
        WRAP(pthread_mutex_destroy)(&_mutex);
        WRAP(pthread_cond_destroy)(&cond);
    }

    static nvsync& getInstance(void) {
        static nvsync *nvsyncObject = NULL;
        static char nvsyncFname[512];
        static int nvsyncFd;
        if ( !nvsyncObject ) {           
            sprintf(nvsyncFname, "%snvsync", xmemory::getLogPath());
            nvsyncFd = createSharedMapFile(nvsyncFname, sizeof(nvsync));
            void *buf = mmap(NULL, sizeof(nvsync), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, nvsyncFd, 0);
            nvsyncObject = new(buf) nvsync();
        }
        return *nvsyncObject;
    }

    // Create a shared file for mmap and return the fd
    static int createSharedMapFile(char *fn, size_t sz){
        int fd = 0;

        // Check if file exists.  If so, rename file to "recover" first
        if ( access(fn, F_OK) != -1 ) {
            char _recoverFname[1024];
            sprintf(_recoverFname, "%s_recover", fn);
            if ( rename(fn, _recoverFname) != 0 ){
                lprintf("error: unable to rename file\n");
            }
        }

        // Create file for mmap
        fd = open(fn, O_RDWR | O_SYNC | O_CREAT, 0644);
        if ( fd == -1 ) {
            fprintf(stderr, "Failed to make persistent file.\n");
            ::abort();
        }

        // Set the files to the sizes of the desired object.
        if ( ftruncate(fd, sz) ) {
            fprintf(stderr, "Mysterious error with ftruncate.\n");
            ::abort();
        }
        return fd;
    }

    /** Pthread mutex functions 
    **/
    mutexEntry* mutex_init(void *mutex) {
    
        mutexEntry *entry = (mutexEntry *)getSyncEntry(mutex);
        if ( entry != NULL) {
            printf("thread %d found existing mutex %p at entry %p in mutexEntries[%d]\n", threadID(), mutex, entry, entry->lockID);
            return entry;
        }
        int i;
        for (i = 0; i < MAX_MUTEX; i++) {
            if ( mutex == (void*)mutexEntries[i] ) {
                printf("mutex %p already exists in mutexEntries[%d]!!\n", mutex, i);
            }
        }

        entry = allocateMutexEntry();
        entry->lockset = NULL;
        entry->cond = NULL;
        entry->lockID = getLockID();
        entry->lockingThreadID = 0;
        entry->waitingCount = 0;

        // allocate the actual mutex lock and set it to be process shared
        pthread_mutex_t *lock = (pthread_mutex_t *)InternalHeap::getInstance().malloc(sizeof(pthread_mutex_t));
        entry->mutex = lock;

        // _mutexattr is set to be process shared in initialize()
        WRAP(pthread_mutex_init)(entry->mutex, &_mutexattr);

        // Set the entry to be the mutex to speedup the mutex lookup
        setSyncEntry(mutex, (void *)entry);

        // Record this lock in mutexEntries[]
        mutexEntries[entry->lockID] = entry;
        printf("thread %d set mutex %p at entry %p in mutexEntries[%d]\n", threadID(), mutex, entry, entry->lockID);

        xmemory::commit(false); 
        xmemory::begin(true);

        return entry;        
    }

    void destroy_entry(mutexEntry *entry) {
        printf("thread %d destroy mutex at entry %p in mutexEntries[%d]\n", threadID(), entry, entry->lockID);
        clearSyncEntry(entry);
        mutexEntries[entry->lockID] = NULL;
        InternalHeap::getInstance().free(entry->mutex);
        freeEntry(entry);
    }

    void mutex_lock(pthread_mutex_t *mutex) {
        mutexEntry *entry;

        // Protect process shared data
        lock();

        entry = (mutexEntry *)getSyncEntry(mutex);
        if ( !entry ) {
            printf("cannot find entry for mutex %p, creating one\n", mutex);
            entry = mutex_init(mutex);
        }

        lockSet *lockset = entry->lockset;

        if ( !lockset ) {
            // This mutex is not currently used by other threads.
            // Check if current thread is holding any locks, if so, add this mutex to the existing lockset.
            // Otherwise creating a new one.
            if ( ThreadLocksets[threadID()] == NULL ) {            
//              _xact++; // operation on _xact needs to be atomic (here protected by lock()/unlock())
                INC_METACOUNTER(globalTransactionCount);
                // This thread is not using any lockset, create a new one
                lockset = createLockSet(GET_METACOUNTER(globalTransactionCount));
                entry->lockset = lockset;
                ThreadLocksets[threadID()] = entry->lockset;
                lprintf("This thread is not using any lockset, created a new lockset %p with ID %d\n", lockset, lockset->lockSetID);
            } else {
                // This thread is using a lockset, retrieve it
                entry->lockset = ThreadLocksets[threadID()];
                lockset = entry->lockset;
                lprintf("This thread is using a lockset %p with ID %d\n", entry->lockset, entry->lockset->lockSetID);
            }

            // Add mapping between lockSet and mutex (if needed) and set mutex to be process shared
            addMutexToLockSet(lockset, entry);
        }
        else {
            lprintf("Found lockset %p in lock entry %p\n", lockset, entry);
        }

        // Increase lock count by 1 for this lockSet
        increaseLockCount(lockset);

        // Increase lock count by 1 for this thread
        ThreadLockingCount[threadID()]++;

        // Always add a thread to the mutex's waiting list
        addToWaitList(entry, threadID()); 

        // Done with updating process shared data
        unlock();

        // Lock the mutex once other threads unlocks it
        bool waited = false;
        lprintf("thread %d perform actual locking for mutex %p\n", threadID(), mutex);
        while ( WRAP(pthread_mutex_trylock)(entry->mutex) != 0 ) {
            if ( !waited ) {
                lprintf("%d: thread %d waiting for mutex %p locked by %d\n", getpid(), threadID(), entry->mutex, entry->lockingThreadID);
            }
            xmemory::commit(false); 
            xmemory::begin(true);
            waited = true;
            // waiting...
        }
//      printf("thread %d locked mutex %p (mEntry: %p)\n", threadID(), entry->mutex, entry);

        // Record this thread locking this mutex
        mutexRecordLockingThread(entry, threadID());

        // Start of critical section ...
        lprintf("thread %d In critical section\n", threadID());
    }

    void mutex_unlock(pthread_mutex_t *mutex) {    
        // Protect process shared data
        lock();

        mutexEntry *entry = (mutexEntry *)getSyncEntry(mutex);


        lockSet *lockSet = entry->lockset;
        lprintf("lockset %p, entry %p, mutex %p\n", lockSet, entry, mutex);

        // Decrease lock count by 1 for this lockSet
        decreaseLockCount(lockSet);

        // Remove this thread from the mutex's waiting list, if needed
        removeFromWaitList(entry, threadID());

        // No one is waiting for the mutex, we can now safely remove this mutex from the lockSet
        if ( waitListIsEmpty(entry) ) {
            // I must be the one who locked it, if not, something is messed up
            lprintf("mutexLockingThread(entry): %d, threadID() %d\n", entry->lockingThreadID, threadID());
            lprintf("lockset %p, entry %p\n", lockSet, entry);
            removeMutexFromLockSet(lockSet, entry);
        }

        // Decrease lock count by 1 for this thread
        ThreadLockingCount[threadID()]--;
        if ( ThreadLockingCount[threadID()] == 0 ) {
            ThreadLocksets[threadID()] = NULL;
            lprintf("This thread released all locks.\n");
        }

        // No more threads using locks in this lockSet, commit dirty pages and do some cleanup
        if ( lockingCount(lockSet) == 0 ) {
            // FIXME: here we should not use lockset count to decide whether to commit page lookup info for dependency tracking
            // FIXME2" commitPageInfo now erases pageInfo global: design a new algorithm to handle the thread local pageInfo commit
//          commitPageInfo();
            deleteLockSet(lockSet);
        }

        mutexClearLockingThread(entry);

        // Commit dirty pages every time a thread unlocks a mutex
        xmemory::commit(false); 
        xmemory::begin(true);

//      printf("thread %d unlocking mutex %p\n", threadID(), entry->mutex);
        // Perform the actual mutex unlock
        WRAP(pthread_mutex_unlock)(entry->mutex);

        // This mutex was locked for cond var 
        if( entry->cond != NULL){            
            condEntry *cEntry = (condEntry*)getSyncEntry(entry->cond);
            // I am the thread who locked the mutex, reset cond to NULL
            if ( cEntry->lockingThreadID == threadID() ) {            
                cEntry->lockingThreadID = 0;
                cEntry->status = COND_WAIT;
                entry->cond = NULL;
                lprintf("thread %d unlocked mutex %p for cond var %p\n", threadID(), mutex, entry->cond);
            } else{
                lprintf("mutex %p was locked by thread %d for cond var %p, but I am thread %d\n", 
                        mutex, cEntry->lockingThreadID, entry->cond, threadID());
            }
        }

        // Done with updating process shared data
        unlock();

        // ...End of critical section
    }

/** Pthread Barrier helper functions 
**/
    void barrier_init(void *barrier, int count){
        pthread_barrierattr_t attr;
        pthread_barrier_t *entry = allocBarrierEntry();

        // Set up with a shared attribute.
        pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        WRAP(pthread_barrier_init)((pthread_barrier_t*)entry, &attr, count);

        setSyncEntry(barrier, entry);
        printf("thread %d initialized barrier %p to count %d\n", threadID(), barrier, count);
    }

    void barrier_wait(void *barrier){
        pthread_barrier_t *entry = (pthread_barrier_t *)getSyncEntry(barrier);
        printf("thread %d waiting for barrier %p\n", threadID(), entry);
        WRAP(pthread_barrier_wait)((pthread_barrier_t*)entry);
    }

/** Pthread condition variable functions
 **/
    condEntry *cond_init(void *cond){
        pthread_condattr_t attr;
        condEntry *entry = (condEntry*)getSyncEntry(cond);
        if ( entry != NULL ) {
            return entry;
        }
        entry = allocCondEntry();

        // allocate the actual cond and set it to be process shared
        entry->cond = (pthread_cond_t *)InternalHeap::getInstance().malloc(sizeof(pthread_cond_t));
        // Set up with a shared attribute
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        WRAP(pthread_cond_init)(entry->cond, &attr);
        entry->status = COND_WAIT;
        entry->lockingThreadID = 0;
        entry->broadcastCount = 0;

        printf("thread %d cond init cond %p\n", threadID(), cond);

        setSyncEntry(cond, entry);
        return entry;
    }

    void cond_wait(void *cond, void *mutex){
        lock(); 
        condEntry *cEntry = (condEntry*)getSyncEntry(cond);
        if ( cEntry == NULL ) {
            cEntry = cond_init(cond);
        }
        mutexEntry *mEntry = (mutexEntry *)getSyncEntry(mutex);
//      xmemory::commit(false);
        xmemory::commit(false);
        xmemory::begin(false);

        unlock();

        // 1. cond variable is signaled! return with mutex locked
        if ( cEntry->status == COND_READY ) {
cond_ready:
            // 1.1. set threadID in cEntry->lockingThreadID
            // 1.2. set cond in mEntry->cond (so we could clear cEntry->lockingThreadID in mutex_unlock if cond is not NULL)
            // 1.3. update dirtied pages as needed to make all thread-local changes visible
            lock();
            cEntry->lockingThreadID = threadID();
            mEntry->cond = (pthread_cond_t*)cond;
            xmemory::begin(true);
            lprintf("cond ready! cond %p, mutex %p (mEntry: %p).\n", cond, mutex, mEntry);
            unlock();
        }
        // 2. cond variable is not signaled yet, unlock mutex and keep waiting (in loop with sched_yield() and mfence)
        else {   
            // 2.1. unlock mutex
            mutex_unlock((pthread_mutex_t*)mutex);

            // 2.2. wait until status becomes ready
            while ( cEntry->status == COND_WAIT ) {
                sched_yield();
                __asm__ __volatile__("mfence");
            }
            lprintf("thread %d waited on cond %p, mutex %p (mEntry: %p). Ready!\n", threadID(), cond, mutex, mEntry);

            // 2.3. lock mutex before going to 1
            mutex_lock((pthread_mutex_t*)mutex);
            goto cond_ready;
        }
    }
    void cond_signal(void *cond){
        lock();
        condEntry *cEntry = (condEntry*)getSyncEntry(cond);

        xmemory::commit(false);
        xmemory::begin(false);

        // Set cond to be ready
        cEntry->status = COND_READY;
        printf("thread %d signal cond %p status %d\n", threadID(), cond, cEntry->status);
        unlock();
    }
    void cond_broadcast(void *cond){
        lock();
        condEntry *cEntry = (condEntry*)getSyncEntry(cond);

        xmemory::commit(false);
        xmemory::begin(false);

        // Set cond to be ready
        cEntry->status = COND_BROADCAST;
        printf("thread %d broadcast cond %p status %d\n", threadID(), cond, cEntry->status);
        unlock();
    }

/** Other pthread functions
 **/
    void nv_join(void *v){
        int w;
        int status;
        ThreadStatus *t = (ThreadStatus *)v;
        int tid = t->tid;
        w = waitpid(tid, &status, 0);
        if ( w == -1 ) {
            fprintf(stderr, "failed to wait thread pid %d\n", tid);
        } else{
//          printf("waited thread pid %d\n", tid);
            xmemory::commit(true);
            xmemory::begin(false);
        }
    }

/** LockSet helper functions 
**/
    lockSet* createLockSet(unsigned long xact) {
        int index = -1;
        int i;
        lockSet *ls;
        for (i = 0; i < MAX_LOCKSET; i++) {
            if ( !locksetInUse[i] ) {
                index = i;
                break;
            }
        }
        ls = &locksets[index];                
        locksetInUse[i] = true;
        ls->lockSetID = index;
        ls->xact = xact;
        ls->requests = 0;
        memset(ls->locks, 0, sizeof(void *) * MAX_MUTEX);
        lprintf("created lockset %p with ID %d for xact %lu\n", ls, index, xact);
        return ls;
    }

    void deleteLockSet(lockSet *ls) {
        int i;
        lprintf("deleting lockset ID %d\n", ls->lockSetID);
        for (i = 0; i < MAX_LOCKSET; i++) {
            if ( ls->locks[i] != NULL ){
                mutexEntry *entry = ls->locks[i];
                entry->lockset = NULL;
                lprintf("remove lockset %d recorded in entry %d\n", ls->lockSetID, entry->lockID);
            }
        }
        locksetInUse[ls->lockSetID] = false;
        memset(&locksets[ls->lockSetID], 0, sizeof(lockSet));
    }

    void addMutexToLockSet(lockSet *ls, mutexEntry *entry) {
        int i;
        for ( i = 0; i < MAX_LOCKSET; i++ ) {
            if ( ls->locks[i] == NULL ){
                lprintf("Record entry at %d in this lockset\n", i);
                break;
            }
        }
        assert(i != MAX_LOCKSET);
        ls->locks[i] = entry;
        lprintf("added entry->lockID %d to lockSetID %d: lockset->locks[%d]\n",
                entry->lockID, ls->lockSetID, i);
    }

    void removeMutexFromLockSet(lockSet *ls, mutexEntry *entry) {
        int i;        
        for (i = 0; i < MAX_LOCKSET; i++) {
            if ( ls->locks[i] == entry ){
                lprintf("entry is at %d in this lockset, remove it\n", i);
                break;
            } else {
                lprintf("ls->locks[%d]: %p\n", i, ls->locks[i]);
            }
        }
        assert(i != MAX_LOCKSET);
        ls->locks[i] = NULL;
        entry->lockset = NULL;
        lprintf("removed entry->lockID %d from lockSetID %d: lockset->locks[%d]\n",
                entry->lockID, ls->lockSetID, i);
    }

    void increaseLockCount(lockSet *ls) {
        ls->requests++;
        lprintf("lockset %p #requests: %d\n", ls, ls->requests);
    }

    void decreaseLockCount(lockSet *ls) {
        ls->requests--;
        lprintf("lockset %p #requests: %d\n", ls, ls->requests);
    }

    int lockingCount(lockSet *ls) {
        return ls->requests;
    }

/** Mutex and WaitList functions 
**/
    void addToWaitList(mutexEntry *entry, int threadIndex) {
        lprintf("adding thread %d to entry %p: waitList[%d][%d] = 1\n", threadIndex, entry, entry->lockID, threadIndex);
        waitList[entry->lockID][threadIndex] = 1;
        entry->waitingCount++;
    }

    void removeFromWaitList(mutexEntry *entry, int threadIndex) {
        lprintf("removing thread %d from entry %p: waitList[%d][%d] = 0\n", threadIndex, entry, entry->lockID, threadIndex);
        waitList[entry->lockID][threadIndex] = 0;
        entry->waitingCount--;
    }

    int mutexRecordLockingThread(mutexEntry *entry, int threadIndex) {
        entry->lockingThreadID = threadIndex;
        lprintf("thread %d set mutex %p locking thread to be %d\n", threadIndex, entry->mutex, threadID());
    }

    int mutexClearLockingThread(mutexEntry *entry) {
        entry->lockingThreadID = 0;
        lprintf("thread %d set mutex %p locking thread to be 0\n", threadID(), entry->mutex);
    }

    int mutexIsLocked(mutexEntry *entry){
        return entry->lockingThreadID;
    }

    int waitListIsEmpty(mutexEntry *entry) {
        if ( entry->waitingCount == 0 ){
            lprintf("entry->lockID %d waitlist is empty\n", entry->lockID);
            return 1;
        } else{
            return 0;
        }
    }


/** Entry helper functions 
**/
    void* getSyncEntry(void *entry) {
        void **ptr = (void **)entry;
        return (*ptr);
    }

    void setSyncEntry(void *origentry, void *newentry) {
        void **dest = (void **)origentry;
        *dest = newentry;
        // Update the shared copy
        xmemory::mem_write(dest, newentry);
    }

    void clearSyncEntry(void *origentry) {
        void **dest = (void **)origentry;
        *dest = NULL;
        // Update the shared copy
        xmemory::mem_write(*dest, NULL);
    }

    inline mutexEntry *allocateMutexEntry(void){
        return ((mutexEntry*)InternalHeap::getInstance().malloc(sizeof(mutexEntry)));
    }

    inline pthread_barrier_t* allocBarrierEntry(void) {
        return ((pthread_barrier_t*)InternalHeap::getInstance().malloc(sizeof(pthread_barrier_t)));
    }

    inline condEntry* allocCondEntry(void) {
        return ((condEntry*)InternalHeap::getInstance().malloc(sizeof(condEntry)));
    }

    inline void freeEntry(void *ptr){
        if ( ptr != NULL ) {
            InternalHeap::getInstance().free(ptr);
        }
    }

/** Commit   */
    void commitPageInfo(void){
        lprintf("Commit tmp pageInfo stored in cache to the actual pageInfo\n");
        xmemory::commitCacheBuffer();
    }
    
    void commitDirtyPages(void){
        xmemory::commit(false);
    }

/** Other helper functions
*/
    int threadID(void){
        return xmemory::_localMemoryLog->threadID;
    }


    int getLockID(void){
        int i;
        for (i = 0; i < MAX_MUTEX; i++) {
            if(mutexEntries[i] == NULL) 
                return i;
        }
        assert(i != MAX_MUTEX);
    }
    
};
#endif
