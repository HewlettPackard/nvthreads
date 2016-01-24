#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#include "util.h"
#include "log_structure.h"
#include "atlas_alloc.h"
#include "atlas_alloc_priv.h"
#include "region_internal.h"
#include "recover.h"

Tid2Log first_log_tracker;
Tid2Log last_log_tracker;
MapR2A map_r2a;
MapLog2Bool replayed_entries;
MapInt2Bool done_threads;
MapLog2Log prev_log_mapper;

extern uint32_t nvm_logs_id;
extern void *log_base_addr;

// All open persistent regions must have an entry in the following data
// structure that maps a region address range to its region id.
MapInterval mapped_prs;

// TODO
// The recovery process should also be able to call into the logic in the
// helper thread to evolve the consistent state if possible before applying
// the _real_ recovery steps.
extern LogStructure *recovery_time_lsp;

int main(int argc, char **argv)
{
    assert(argc == 2);
    
    R_Initialize(argv[1]);
    
    LogStructure *lsp = GetLogStructureHeader();

    // This can happen only if logs were never created by the user threads.
    // Note that if logs are ever created, there should be some remnants
    // after a crash since the helper thread never removes everything.
    if (!lsp)
    {
        fprintf(stderr, "[Atlas] Warning: No logs present\n");
        R_Finalize(argv[1]);
        exit(0);
    }

    helper(lsp);

    if (recovery_time_lsp) lsp = recovery_time_lsp;
    
    CreateRelToAcqMappings(lsp);

    Recover();
    
    R_Finalize(argv[1]);
}

void R_Initialize(const char *s)
{
    NVM_SetupRegionTable(s);

    // The exact mechanism to find the regions that need to be reverted
    // remains to be done. One possibility is to look at all logs on
    // persistent memory, conceivably from a certain pre-specified region.
    // For now, the recovery process requires the name of the process
    // whose crash we are trying to recover from. Using this information,
    // the recovery phase constructs the name of the corresponding log and
    // finds the regions needing recovery from this log.
    const char *log_name = NVM_GetLogRegionName(s);
    if (!NVM_doesLogExist(NVM_GetFullyQualifiedRegionName(log_name)))
    {
        fprintf(stderr, "[Atlas] No log file exists, nothing to do ...\n");
        exit(0);
    }

    nvm_logs_id = NVM_FindRegion_priv(log_name, O_RDWR, true);
    InsertToMapInterval(&mapped_prs,
                        (uint64_t)log_base_addr,
                        (uint64_t)((char*)log_base_addr+FILESIZE),
                        nvm_logs_id);
}

void R_Finalize(const char *s)
{
    MapInterval::const_iterator ci_end = mapped_prs.end();
    for (MapInterval::const_iterator ci = mapped_prs.begin();
         ci != ci_end; ++ ci)
        NVM_CloseRegion_priv(ci->second, true);
    NVM_DeleteRegion(NVM_GetLogRegionName(s));
    fprintf(stderr, "[Atlas] Done bookkeeping\n");
}

LogStructure *GetLogStructureHeader()
{
    LogStructure **lsh_p =
        (LogStructure**)NVM_GetRegionRoot(nvm_logs_id);
    assert(lsh_p && "[Atlas] Region root is null: did you forget to set it?");
    
    return (LogStructure*)*lsh_p;
}

void CreateRelToAcqMappings(LogStructure *lsp)
{
    // just use a logical thread id
    int tid = 0;
    // The very first log for a given thread must be found. This will be
    // the last node replayed for this thread. Note that most of the time
    // this first log will point to null but that cannot be guaranteed. The
    // helper thread commits changes atomically by switching the log structure
    // header pointer. The logs are destroyed after this atomic switch.
    while (lsp)
    {
        LogEntry *last_log = 0; // last log written in program order
        LogEntry *le = lsp->Le_;
//        uint64_t num_strs = 0;
        if (le)
        {
            assert(first_log_tracker.find(tid) == first_log_tracker.end());
            first_log_tracker[tid] = le;
        }
        while (le)
        {
            if (le->Type_ == LE_acquire) AddToMap(le, tid);
//            if (le->Type_ == LE_str) ++num_strs;
            prev_log_mapper[le] = last_log;
            last_log = le;
            le = le->Next_;
        }
//        UtilVerboseTrace3(stderr, "[RECOVERY] num_strs = %ld\n", num_strs);
        if (last_log)
        {
            assert(last_log_tracker.find(tid) == last_log_tracker.end());
            last_log_tracker.insert(make_pair(tid, last_log));
        }
        ++ tid;
        lsp = lsp->Next_;
    }
}

void AddToMap(LogEntry *acq_le, int tid)
{
    LogEntry *rel_le = (LogEntry *)(acq_le->ValueOrPtr_);

    if (rel_le)
    {
#if 0 // We can't assert this since a log entry may be reused.
#ifdef DEBUG        
        if (map_r2a.find(rel_le) != map_r2a.end())
        {
            assert(isRWLockUnlock(rel_le));
            assert(isRWLockRdLock(acq_le));
        }
#endif
#endif        
        map_r2a.insert(make_pair(rel_le, make_pair(acq_le, tid)));
    }
}

#ifdef _NVM_TRACE
template<class T> void RecoveryTrace(void *p)
{
    fprintf(stderr, "%p %ld\n", p, *(T*)p);
}
#endif

// The first time an address in a given persistent region (PR) is found,
// that PR is opened and all open PRs must have an entry in mapped_prs.
// Every time an address has to be replayed, it is looked up in the above
// mapper to determine whether it is already mapped. The same mapper data
// structure is used during logging as well to track all open PRs. We assume
// that no transient location is logged since it must have been filtered
// out using this mapper during logging.
void Replay(LogEntry *le)
{
    assert(le);
    assert(isStr(le) || isMemop(le));

    void *addr = le->Addr_;
    if (FindInMapInterval(mapped_prs,
                          (uint64_t)addr,
                          (uint64_t)((char*)addr+le->Size_-1)) ==
        mapped_prs.end())
    {
        pair<void*,uint32_t> mapper_result =
            NVM_EnsureMapped((intptr_t*)addr, true);
        InsertToMapInterval(&mapped_prs,
                            (uint64_t)(char*)mapper_result.first,
                            (uint64_t)((char*)mapper_result.first+FILESIZE),
                            mapper_result.second);
    }

    if (isStr(le))
    {
        // TODO bit access is not supported?
        assert(!(le->Size_ % 8));
        memcpy(addr, (void*)&(le->ValueOrPtr_), le->Size_/8);
    }
    else if (isMemop(le))
    {
        assert(le->ValueOrPtr_);
        memcpy(addr, (void*)(le->ValueOrPtr_), le->Size_);
    }
    
}

LogEntry *GetPrevLogEntry(LogEntry *le)
{
    assert(le);
    if (prev_log_mapper.find(le) != prev_log_mapper.end())
        return prev_log_mapper.find(le)->second;
    else return NULL;
}

// We start from an arbitrary thread (say, tid 0), grab its last log and
// start playing back. If we hit an acquire node, we do nothing. If we
// hit a release node, we look up the map to see if it has a corresponding
// acquire node. If yes, we switch to this other thread, grab its last
// log and start playing it. This method is followed until all logs have
// been played. We need to update the last log of a given thread whenever
// we switch threads. If all logs in a given thread are exhausted, we delete
// that thread entry from the last-log-tracker.

void Recover()
{
    Tid2Log::iterator ci_end = last_log_tracker.end();
    for (Tid2Log::iterator ci = last_log_tracker.begin(); ci != ci_end; ++ ci)
        Recover(ci->first);
    fprintf(stderr, "[Atlas] Done reverting all data in the log ...\n");
}

void Recover(int tid)
{
    if (done_threads.find(tid) != done_threads.end()) return;
    
    // All replayed logs must have been filtered already.
    Tid2Log::iterator ci = last_log_tracker.find(tid);
    if (ci == last_log_tracker.end()) return;
    
    LogEntry *le = ci->second;

    assert(first_log_tracker.find(tid) != first_log_tracker.end());
    LogEntry *stop_node = first_log_tracker.find(tid)->second;
    
    while (le)
    {
#ifdef _NVM_TRACE
        fprintf(stderr,
                "Replaying tid = %d le = %p, addr = %p, val = %ld Type = %s\n",
                tid, le, le->Addr_, le->ValueOrPtr_,
                le->Type_ == LE_acquire ? "acq" :
                le->Type_ == LE_release ? "rel" :
                le->Type_ == LE_str ? "str" :
                isMemset(le) ? "memset" :
                isMemcpy(le) ? "memcpy" :
                isMemmove(le) ? "memmove" : "don't-care");
#endif
        // TODO: handle other kinds of locks during recovery.
        if (isRelease(le))
        {
            pair<R2AIter, R2AIter> r2a_iter = map_r2a.equal_range(le);
            if (r2a_iter.first != r2a_iter.second)
            {
                // We are doing a switch, so adjust the last log
                // from where a subsequent visit should start
                ci->second = GetPrevLogEntry(le);
            }
            for (R2AIter ii = r2a_iter.first; ii != r2a_iter.second; ++ii)
            {
                LogEntry *new_tid_acq = ii->second.first;
                int new_tid = ii->second.second;

                if (!isAlreadyReplayed(new_tid_acq)) Recover(new_tid);
            }
        }
        else if (isAcquire(le))
        {
            if (done_threads.find(tid) != done_threads.end()) break;
            MarkReplayed(le);
        }
        else if (isStr(le) || isMemset(le) || isMemcpy(le) || isMemmove(le))
            Replay(le);

        if (le == stop_node)
        {
            done_threads.insert(make_pair(tid, true));
            break;
        }
        le = GetPrevLogEntry(le);
    }
}

void MarkReplayed(LogEntry *le)
{
    assert(le->Type_ == LE_acquire);
    assert(replayed_entries.find(le) == replayed_entries.end());
    replayed_entries[le] = true;
}

bool isAlreadyReplayed(LogEntry *le)
{
    assert(le->Type_ == LE_acquire);
    return replayed_entries.find(le) != replayed_entries.end();
}
