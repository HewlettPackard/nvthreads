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


/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file  xthread.cpp
 * @brief  Functions to manage thread related spawn, join.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <pthread.h>
#include <syscall.h>
#include "xthread.h"
#include "xatomic.h"
#include "xrun.h"
#include "debug.h"

unsigned int xthread::_nestingLevel = 0;
int xthread::_tid;

MemoryLog xthread::_localMemoryLog;
nvrecovery xthread::_localNvRecovery;

void* xthread::spawn(threadFunction *fn, void *arg, int parent_index) {
    // Allocate an object to hold the thread's return value.
    void *buf = allocateSharedObject(4096);
    HL::sassert<(4096 > sizeof(ThreadStatus))> checkSize;
    ThreadStatus *t = new(buf) ThreadStatus;

    return forkSpawn(fn, t, arg, parent_index);
}

/// @brief Get thread index for this thread.
int xthread::getThreadIndex(void *v) {
    assert(v != NULL);

    ThreadStatus *t = (ThreadStatus *)v;

    return t->threadIndex;
}

/// @brief Get thread tid for this thread.
int xthread::getThreadPid(void *v) {
    assert(v != NULL);

    ThreadStatus *t = (ThreadStatus *)v;

    return t->tid;
}

/// @brief Do pthread_join.
void xthread::join(void *v, void **result) {
    ThreadStatus *t = (ThreadStatus *)v;
    DEBUG("%d: Joining thread %d", getpid(), t->tid);

    // Grab the thread result from the status structure (set by the thread),
    // reclaim the memory, and return that result.
    if ( result != NULL ) {
        *result = t->retval;
    }

    // Free the shared object held by this thread.
    freeSharedObject(t, 4096);

    return;
}

/// @brief Cancel one thread. Send out a SIGKILL signal to that thread
int xthread::cancel(void *v) {
    ThreadStatus *t = (ThreadStatus *)v;

    int threadindex = t->threadIndex;

    kill(t->tid, SIGKILL);

    // Free the shared object held by this thread.
    freeSharedObject(t, 4096);
    return threadindex;
}

int xthread::thread_kill(void *v, int sig) {
    int threadindex;
    ThreadStatus *t = (ThreadStatus *)v;

    threadindex = t->threadIndex;

    kill(t->tid, sig);

    freeSharedObject(t, 4096);
    return threadindex;
}

void* xthread::forkSpawn(threadFunction *fn, ThreadStatus *t, void *arg, int parent_index) {
    // Use fork to create the effect of a thread spawn.
    // FIXME:: For current process, we should close share.
    // children to use MAP_PRIVATE mapping. Or just let child to do that in the beginning.
    int child = syscall(SYS_clone, CLONE_FS | CLONE_FILES | SIGCHLD, (void *)0);
    
    if ( child ) {

        // I need to wait until the child has waited on creation barrier sucessfully.       
        xrun::waitChildRegistered();

        return (void *)t;

    } else {
        pid_t mypid = syscall(SYS_getpid);
        setId(mypid);

//      if ( MemoryLog::_logging_enabled ) {
            // Initialize varmap log
            xthread::_localNvRecovery.initialize(false);

            // Initialize memory log
            xthread::_localMemoryLog.initialize(xthread::_localNvRecovery.nvid);
//      }

        int threadindex = xrun::childRegister(mypid, parent_index, &xthread::_localMemoryLog, &xthread::_localNvRecovery);

        t->threadIndex = threadindex;
        t->tid = mypid;

        xrun::waitParentNotify();

//      lprintf("%d: I'm thread %d\n", mypid, t->threadIndex);

        _nestingLevel++;
        run_thread(fn, t, arg);
        _nestingLevel--;

//      if ( MemoryLog::_logging_enabled ) {
            
            // Finalize memory log
            xthread::_localMemoryLog.finalize();

            // Finalize varmap log
            xthread::_localNvRecovery.finalize(); 

//      }

        _exit(0);
        return NULL;        
    }
}

// @brief Execute the thread.
void xthread::run_thread(threadFunction *fn, ThreadStatus *t, void *arg) {
    xrun::atomicBegin(true);
    void *result = fn(arg);
    xrun::threadDeregister();
    t->retval = result;
}
