#include <stdio.h>
#include <stdlib.h>

#include "atlas_api.h"
#include "helper.h"
#include "util.h"
#include "log_alloc.h"

#define HELPER_LE_ANALYSIS_LIMIT 256

extern CbListNode<LogEntry> * volatile cb_log_list;
extern uint64_t removed_log_count;
MultiMapLog2Int deleted_rel_logs;

extern bool is_recovery;
extern LogStructure *recovery_time_lsp;

#if defined(_NVM_TRACE) || defined(_NVM_VERBOSE_TRACE)
__thread uint64_t tl_log_dealloc_count = 0;
#endif

// TODO mark with inline keyword
void TrackLog(LogEntry *le, Log2Bool *log_map)
{
    (*log_map).insert(make_pair(le, true));
}

void RemoveLog(LogEntry *le, Log2Bool *log_map)
{
    Log2Bool::iterator ci = log_map->find(le);
    if (ci == log_map->end()) return;
    log_map->erase(ci);
}

void CollectRelLogEntries(LogStructure *lsp, MapLog2Int *rle)
{
    while (lsp)
    {
        LogEntry *le = lsp->Le_;
        while (le)
        {
            if (isRelease(le)) rle->insert(make_pair(le, le->Size_));
            le = le->Next_;
        }
        lsp = lsp->Next_;
    }
}

bool isFoundInExistingLog(LogEntry *le, uint64_t gen_num,
                          const MapLog2Int & elmap)
{
    MapLog2Int::const_iterator ci = elmap.find(le);
    if (ci != elmap.end() && ci->second == gen_num) return true;
    return false;
}
    
template<class T>
void DeleteSlot(CbLog<T> *cb, T *addr)
{
    assert(!isCbEmpty<T>(cb));
    assert(addr == &(cb->LogArray_[ALAR(cb->Start_)]));
    ASRW(cb->Start_, (ALAR(cb->Start_)+1) % cb->Size_);

#if defined(_NVM_VERBOSE_TRACE)
    tl_log_dealloc_count ++;
    if (!((tl_log_dealloc_count+1) % TRACE_GRANULE))
        UtilVerboseTrace3(helper_trace_file, "Log entry dealloc count = %ld\n",
                          tl_log_dealloc_count);
#endif    
}

template<class T>
void DeleteEntry(CbListNode<T> * volatile cb_list, T *addr)
{
    // Instead of searching every time, we keep track of the last Cb used.
    static CbListNode<T> *last_cb_used = 0;
    if (last_cb_used &&
        ((uintptr_t)addr >= (uintptr_t)last_cb_used->StartAddr_ &&
         (uintptr_t)addr <= (uintptr_t)last_cb_used->EndAddr_))
    {
        DeleteSlot<T>(last_cb_used->Cb_, addr);
        if (isCbEmpty<T>(last_cb_used->Cb_) &&
            (uint32_t)ALAR_int(last_cb_used->Cb_->isFilled_))
            ASRW_int(last_cb_used->isAvailable_, true);
        return;
    }
    CbListNode<T> *curr = (CbListNode<T>*)ALAR(cb_list);
    assert(curr);
    while (curr)
    {
        if ((uintptr_t)addr >= (uintptr_t)curr->StartAddr_ &&
            (uintptr_t)addr <= (uintptr_t)curr->EndAddr_)
        {
            last_cb_used = curr;
            DeleteSlot<T>(curr->Cb_, addr);
            // A buffer that got filled and then emptied implies that
            // the user thread moved on to a new buffer. Hence, mark it
            // available so that it can be reused.
            if (isCbEmpty<T>(curr->Cb_) &&
                (uint32_t)ALAR_int(curr->Cb_->isFilled_))
                ASRW_int(curr->isAvailable_, true);
            break;
        }
        curr = curr->Next_;
    }
}

// The Outermost Critical Section (OCS) is a data structure used by
// the helper thread alone. It starts with a provided log entry and
// traverses the thread-specific logs until it either runs out of them
// or comes across the end of an outermost critical section. If the
// former, an OCS is not built. If the latter, an OCS is built and
// returned.   
OcsMarker *CreateOcsMarker(LogEntry *le)
{
    uint32_t lock_count = 0;
    uint32_t log_count = 0;
    LogEntry *first_le = le;
    while (le)
    {
        ++log_count;

        /* unnecessary
        if (log_count > HELPER_LE_ANALYSIS_LIMIT) return 0;
        */
        
        LogEntry *next_le = (LogEntry*) ALAR(le->Next_);;
        if (!next_le) return 0; // always keep one non-null log entry
        
        if (isAcquire(le) || isRWLockRdLock(le) || isRWLockWrLock(le)
            || isBeginDurable(le)) ++lock_count;

        if (isRelease(le) || isRWLockUnlock(le) || isEndDurable(le))
        {
            if (lock_count > 0) --lock_count;
            if (!lock_count) 
            {
                OcsMarker *ocs = (OcsMarker*)malloc(sizeof(OcsMarker));
                ocs->First_ = first_le;
                ocs->Last_ = le;
                ocs->Next_ = 0;
                ocs->isDeleted_ = false;
                return ocs;
            }
        }
        le = next_le;
    }
    assert(0);
    return 0;
}

void AddOcsMarkerToMap(OcsMap *ocs_map, LogStructure *ls, OcsMarker *ocs)
{
    ocs_map->insert(make_pair(ls, ocs));
}

void DeleteOcsMarkerFromMap(OcsMap *ocs_map, LogStructure *ls)
{
    OcsMap::iterator ci = ocs_map->find(ls);
    if (ci != ocs_map->end()) ocs_map->erase(ci);
}

OcsMarker *GetOcsMarkerFromMap(const OcsMap & ocs_map, LogStructure *ls)
{
    OcsMap::const_iterator ci = ocs_map.find(ls);
    if (ci != ocs_map.end()) return ci->second;
    else return NULL;
}

void AddOcsMarkerToVec(OcsVec *ocs_vec, OcsMarker *ocs)
{
    ocs_vec->push_back(ocs);
}

void DestroyOcses(OcsVec *ocs_vec)
{
    assert(ocs_vec);
    OcsVec::iterator ci_end = ocs_vec->end();
    for (OcsVec::iterator ci = ocs_vec->begin(); ci != ci_end; ++ci)
        free(*ci);
    ocs_vec->clear();
}

void RemoveAcquiresAndReleases(const DirectedGraph & dg, VDesc nid,
                               Log2Bool *acq_map, Log2Bool *rel_map)
{
    OcsMarker *ocs = dg[nid].OCS_;
    assert(ocs);

    LogEntry *current_le = ocs->First_;
    assert(current_le);
    LogEntry *last_le = ocs->Last_;
    assert(last_le);

    do 
    {
        if (isAcquire(current_le)) RemoveLog(current_le, acq_map);
        else if (isRelease(current_le)) RemoveLog(current_le, rel_map);

        if (current_le == last_le) break;
        current_le = current_le->Next_;
    }while (true);
}

// Since a release log can have a max of 1 incoming edge from an
// acquire node, if there is such an edge within the graph, then no
// incoming edge from a node outside the graph can exist. In such a case,
// the release log does not have to be tracked for the purpose of
// nullifying later acquires.
// TODO: this may not be true for rwlocks
void PruneReleaseLog(Log2Bool *rel_map, const Log2Bool & acq_map)
{
    Log2Bool::const_iterator ci_end = acq_map.end();
    for (Log2Bool::const_iterator ci = acq_map.begin();
         ci != ci_end; ++ ci)
    {
        LogEntry *acq_le = ci->first;
        assert(acq_le);
        if (acq_le->ValueOrPtr_)
        {
            LogEntry *rel_le = (LogEntry *)(acq_le->ValueOrPtr_);
            Log2Bool::iterator ci_rel = rel_map->find(rel_le);
            if (ci_rel != rel_map->end())
                rel_map->erase(ci_rel);
        }
    }
}

VDesc CreateNode(DirectedGraph *dg, OcsMarker *ocs, intptr_t sid)
{
    VDesc nodeid = add_vertex(*dg);
    (*dg)[nodeid].Sid_ = sid;
    (*dg)[nodeid].OCS_ = ocs;
    
    // default attribute is stable. If any contained log entry is
    // unresolved, the stable bit is flipped and the corresponding node
    // removed from the graph
    (*dg)[nodeid].isStable_ = true;
    return nodeid;
}

EDesc CreateEdge(DirectedGraph *dg, VDesc src, VDesc tgt)
{
    pair<EDesc, bool> edge_pair = add_edge(src, tgt, *dg);
    return edge_pair.first;
}

void AddToPendingList(PendingList *pl, LogEntry *le, VDesc nid)
{
    PendingPair pp = make_pair(le, nid);
    pl->push_back(pp);
}

void AddToNodeInfoMap(
    NodeInfoMap *nim, LogEntry *le, VDesc nid, intptr_t sid, NODE_TYPE nt)
{
    NodeInfo ni;
    ni.Nid_ = nid;
    ni.Sid_ = sid;
    ni.Nt_ = nt;
    
    (*nim)[le] = ni;
}

pair<VDesc,NODE_TYPE> GetTargetNodeInfo(
    const DirectedGraph & dg, const NodeInfoMap & nim, LogEntry *tgt_le)
{
    NodeInfoMap::const_iterator ci = nim.find(tgt_le);
    if (ci == nim.end()) return make_pair((VDesc)0 /* dummy */, NT_absent);
    else if (ci->second.Nt_ == NT_deleted)
        return make_pair((VDesc)0, NT_deleted);
    else return make_pair(ci->second.Nid_, NT_avail);
}

void ResolvePendingList(PendingList *pl,
                        const NodeInfoMap & nim,
                        DirectedGraph *dg,
                        const LogVersions & log_v)
{
    // This vector is for storing the entries in the pendinglist that
    // get resolved. These can be deleted so that the pending list is
    // left only with elements that cannot be resolved.
    typedef vector<PendingList::iterator> DelVec;
    DelVec del_vec;
    
    PendingList::iterator ci_end = pl->end();
    for (PendingList::iterator ci = pl->begin(); ci != ci_end; ++ ci)
    {
        LogEntry * le = ci->first;
        
        assert(le);
        assert(isAcquire(le));
        assert(le->ValueOrPtr_);

        pair<VDesc,NODE_TYPE> ni_pair = GetTargetNodeInfo(
            *dg, nim, (LogEntry *)le->ValueOrPtr_);
        // NT_deleted already filtered out

        if (ni_pair.second == NT_avail)
        {
            CreateEdge(dg, ci->second, ni_pair.first);
            // Now that this entry is resolved, tag it for deletion
            del_vec.push_back(ci);
        }
        // Mark the corresponding node unstable only if target is absent.
        // If deleted, leave the node stable
        else if (ni_pair.second == NT_absent)
            if (le->ValueOrPtr_) (*dg)[ci->second].isStable_ = false;
    }
    // Actual removal of tagged resolved entries
    DelVec::const_iterator del_ci_end = del_vec.end();
    for (DelVec::const_iterator del_ci = del_vec.begin();
         del_ci != del_ci_end; ++ del_ci)
        pl->erase(*del_ci);
}

// This routine removes nodes that cannot be resolved
void RemoveUnresolvedNodes(DirectedGraph *dg, const PendingList & pl,
                           LogEntryVec *le_vec, Log2Bool *acq_map,
                           Log2Bool *rel_map)
{
    // quick check since we keep the pending list up-to-date
    if (pl.empty()) return;

    MapNodes removed_nodes;
    pair<VIter,VIter> vp;
    int counter = 0;
    for (vp = vertices(*dg); vp.first != vp.second; ++ vp.first)
    {
        VDesc nid = *vp.first;

        if (removed_nodes.find(nid) != removed_nodes.end())
        {
            assert(!(*dg)[nid].isStable_);
            continue;
        }
        
        if ((*dg)[nid].isStable_) continue;

        HandleUnresolved(dg, nid, &removed_nodes, &counter);
    }
    
    // In addition to marking the nodes in the graph "deleted", we
    // collect the acquire nodes that point to these deleted entries.
    // After a version is removed, we nullify the corresponding pointers
    // from the acquire nodes. If there is a crash and all the nullification
    // is not complete, this task has to be completed during recovery.
    // During recovery, a pre-pass nullifies the pointer for every acquire
    // node in a complete TCS if the target log entry is not found. This
    // nullification phase during recovery is sound. Here's why: let's
    // consider the log entries left around at the time of a crash. The
    // corresponding TCSes may be complete or incomplete. If an acquire
    // entry has a non-null pointer to a release entry, that release entry
    // must either be in the left-around log or the release entry must
    // correspond to a log entry that was deleted before the crash. Note
    // that the TCS corresponding to such a release log in the left-around
    // log structure does not have to be complete.
#if 0    
    for (vp = vertices(*dg); vp.first != vp.second; ++ vp.first)
    {
        // The target cannot be in the list of removed nodes
        // The source has to be in the list of removed nodes
        VDesc nid = *vp.first;
        if (removed_nodes.find(nid) != removed_nodes.end()) continue;

        pair<IEIter, IEIter> iep;
        for (iep = in_edges(nid, *dg); iep.first != iep.second; ++ iep.first)
        {
            EDesc eid = *iep.first;
            VDesc src = source(eid, *dg);
            if (removed_nodes.find(src) == removed_nodes.end()) continue;
        }
    }
#endif
    // Actually remove the nodes
    // VertexList cannot be of type vecS, since remove_vertex renumbers the
    // vertex indices. 
    MapNodes::const_iterator rm_ci_end = removed_nodes.end();
    MapNodes::const_iterator rm_ci = removed_nodes.begin();
    for (; rm_ci != rm_ci_end; ++ rm_ci)
    {
        // Don't track these acquires and releases any more for the purpose
        // of nullifying acquires
        RemoveAcquiresAndReleases(*dg, rm_ci->first, acq_map, rel_map);
        clear_vertex(rm_ci->first, *dg);
        remove_vertex(rm_ci->first, *dg);
    }
}

void HandleUnresolved(DirectedGraph *dg, VDesc nid, MapNodes *rm,
                      int *counter)
{
    ++ (*counter);
    
    // already handled
    if (rm->find(nid) != rm->end()) return;

    (*rm)[nid] = true;
    
    // if nid is removed, examine all in-edges and for a given in-edge,
    // remove the source node as well
    pair<IEIter,IEIter> iep;
    for (iep = in_edges(nid, *dg); iep.first != iep.second; ++iep.first)
    {
        EDesc eid = *iep.first;
        VDesc src = source(eid, *dg);

        if (rm->find(src) != rm->end())
        {
            assert(!(*dg)[src].isStable_);
            continue;
        }
        
        (*dg)[src].isStable_ = false;

        HandleUnresolved(dg, src, rm, counter);
    }
}

void AddLogStructure(LogEntry *le, LogStructure **header,
                     LogStructure **last_header)
{
    assert(header);
    assert(last_header);
    LogStructure *new_ls = CreateLogStructure(le);
    if (!*header) *header = new_ls;
    else
    {
        assert(*last_header);
        (*last_header)->Next_ = new_ls;
#if !defined(_DISABLE_LOG_FLUSH) && !defined(DISABLE_FLUSHES)
        NVM_FLUSH_ACQ(&(*last_header)->Next_);
#endif                    
    }
    *last_header = new_ls;
}

void CollectLogs(Log2Bool *logs, OcsMarker *ocs)
{
    assert(ocs);
    assert(ocs->Last_);
    
    LogEntry *curr = ocs->First_;
    do
    {
        assert(curr);
        (*logs)[curr] = true;
        if (curr == ocs->Last_) break;
        curr = curr->Next_;
    }while (true);
}

// the problem is that as the helper thread is working, new acquire nodes
// may be added that reference release nodes being deleted. No corruption
// is possible because we wait for this condition to disappear before
// actually removing those release logs. But the problem is that after these
// logs are destroyed and we subsequently examine those acquire nodes in the
// next pass in the helper thread, they all _seem_ unstable. We need to
// distinguish this condition from the other where acquire nodes depend on
// incomplete TCSes. Today, these are conflated into one and hence we cannot
// remove any nodes from the graph after a point in the helper.
// Note: the above problem has been fixed.

// Create a multimap in the
// helper thread that is a mapping from a lock to a number of log entries
// equal to the number of threads. For a given lock,
// each log entry denotes the last release of that lock in the corresponding
// thread. This data structure needs to be versioned as well but needs to
// be cumulative, meaning that the multimap for a given version must be
// the thread-specific union of its previous version. This thread-specific
// requirement is interesting because it means that the helper thread
// must be able to correlate between threads between different phases. We
// need to incorporate the thread id in the log structure header entries.

// It appears that an optimization of the above idea is possible. Note
// that in the log-level graph, a release node can have a maximum of one
// incoming inter-thread edge. Given the releases of a given lock in the
// TCSes in the deletable sub-graph determined by the helper thread, exactly
// one release log entry will have no incoming edge. Given this insight,
// we should just keep a map from a lock to its _last_ release for the
// deletable sub-graph. This map still needs to be versioned as mentioned
// above. Note that at this point, we are not building the log-level graph,
// so this needs to be solved. There is no need to keep a map from a lock
// to the log entry, a searchable list (e.g. map) of the relevant release
// log entries should suffice. Even if we keep information about a lock, it
// cannot be used to nullify those lock entries in subsequent logs because
// a subsequent log may not have been created yet. So our best bet is to
// keep the above list and query _all_ versions of such lists when a new
// iteration of the helper thread starts out for a given log entry until
// a match is found. Thus if a match is found in a version, other versions
// will not have to be searched.

// TODO Note that the core design does not take care of reentrant locking.
// Keeping a thread-local map to filter out locks held at a certain point
// of time can help here.

// Note that the source TCS may not be complete. But it does not matter.
// The log entries are complete and no thread other than the helper will
// modify them.

void CreateVersions(const DirectedGraph & dg, LogVersions *log_v,
                    const LogEntryVec & le_vec, Log2Bool *rel_map,
                    const Log2Bool & acq_map, const OcsMap & ocs_map)
{
    // All nodes in the graph at this point are complete and have been
    // resolved. So all of them can be marked deletable. What we do is
    // traverse the graph and mark the corresponding TCSes
    // deletable. We then walk the log-structure-header, and for every
    // entry in it, walk through that thread's TCSes and find the
    // first undeleted one. (In debug mode, assert that no later TCS
    // is marked deleted.) Prepare a list of <log-header-entry-ptr,
    // first undeleted TCS>. Then we build a new version of the log
    // structure from this information. Lastly, the head of the global
    // log structure is switched in an atomic pointer update. Note
    // that we will create a new version of the log structure header
    // as above every time and only periodically remove the unwanted
    // versions. Removing unwanted versions is expensive since it
    // involves walking the OwnerTable.

    if (!num_vertices(dg)) return;

    // TODO: This may not be required any more since we are not
    // nullifying any more. Double check.
    PruneReleaseLog(rel_map, acq_map);
    
    pair<VIter,VIter> vp;
    for (vp = vertices(dg); vp.first != vp.second; ++ vp.first)
    {
        VDesc nid = *vp.first;
        OcsMarker *ocs = dg[nid].OCS_;
        assert(ocs);

        // TODO turn on this assert later on
        // assert(!ocs->isDeleted_);
        ocs->isDeleted_ = true;
    }

    LogStructure *lsp = !is_recovery ?
        (LogStructure *) ALAR(*log_structure_header_p) : recovery_time_lsp;
    
    assert(lsp);
    LogStructure *new_header = 0;
    LogStructure *last_ls = 0;

    // In this version, these are the log entries to be deleted
    Log2Bool deletable_logs;
    while (lsp)
    {
        OcsMarker *ocs_marker = GetOcsMarkerFromMap(ocs_map, lsp);
        bool found_undeleted = false;

        if (!ocs_marker)
        {
            found_undeleted = true;
            AddLogStructure(lsp->Le_, &new_header, &last_ls);
        }
        OcsMarker *last_ocs_marker = NULL;
        while (ocs_marker)
        {
#if !defined(NDEBUG)            
            if (found_undeleted) assert(!ocs_marker->isDeleted_);
#endif            
            
            if (!found_undeleted && !ocs_marker->isDeleted_)
            {
                found_undeleted = true;

                // No new OcsMarker needs to be created since that
                // will be created by the helper driver
                AddLogStructure(ocs_marker->First_, &new_header, &last_ls);
                last_ocs_marker = ocs_marker;
                ocs_marker = ocs_marker->Next_;
                
                // should break out in production mode TODO
            }
            else
            {
                CollectLogs(&deletable_logs, ocs_marker);
                last_ocs_marker = ocs_marker;
                ocs_marker = ocs_marker->Next_;
            }
        }
        // Note that we always leave the last log entry around, for a given
        // thread. Otherwise, it leads to concurrency issues that would
        // require locking 

        // This can happen if all OCSes created are removable for
        // consistency purposes
        if (!found_undeleted) 
        {
            assert(last_ocs_marker);
            assert(last_ocs_marker->Last_);
            assert(last_ocs_marker->Last_->Next_);
            // No new OcsMarker needs to be created since that will be
            // done by the helper driver on the next iteration.
            AddLogStructure(last_ocs_marker->Last_->Next_,
                            &new_header, &last_ls);
        }
        lsp = lsp->Next_;
    }
    assert(new_header);
    LogVer log_ver;
    log_ver.LS_ = new_header;
    log_ver.Rel_ = *rel_map;
    log_ver.Del_ = deletable_logs;
    (*log_v).push_back(log_ver);
}

bool areLogicallySame(LogStructure * gh, LogStructure * cand_gh)
{
    while (gh)
    {
        if (gh == cand_gh) return true;
        gh = gh->Next_;
    }
    return false;
}

uint32_t GetNumNewEntries(LogStructure *new_e, LogStructure *old_e)
{
    uint32_t new_count = 0;
    uint32_t old_count = 0;
    while (new_e)
    {
        ++new_count;
        new_e = new_e->Next_;
    }
    while (old_e)
    {
        ++old_count;
        old_e = old_e->Next_;
    }
    assert(!(new_count < old_count));
    return new_count - old_count;
}

// The version corresponding to the global log-structure-header is called
// the 0-th version. 
// Always try to destroy the deleted TCSes reachable from the global
// log-structure-header. If you can't destroy these TCSes, 
// no newer version can be deleted either. Deletion of a version
// involves traversing the TCSes for that version checking whether there
// is any reference to any of its release log-entries in OwnerTab. If there
// is, then this version cannot be deleted. (Note that entries are not
// deleted from OwnerTab.) If a version can be deleted, we need to make
// sure that the next version is up-to-date before swinging GH atomically.
// If they are not the same, the next version must be modified to match the
// 0-th version. Then GH can be swung atomically (this must be a cmpxchg
// loop). Remove the first entry from the vector of versions.
void DestroyLogs(LogVersions *log_v)
{
    // Nothing to do
    if (!log_v->size()) return;

    if (ALAR(all_done)) return;

    // Once we start destroying the logs, the helper thread may not be
    // able to return even if the user threads are done. If we return
    // in the middle of log pruning as below, we need to make sure
    // that we do not crash before we can remove the log file
    // entirely. So "all_done" is not checked below any more. It is
    // theoretically ok to return after removing a version but we need
    // to investigate further to figure out whether we are currently
    // set up to correctly do that. 
    bool done = false;
    LogIterVec deleted_lsp;
    LogEntryVec deleted_log_entries;
    
#if 0    
    fprintf(stderr, "predump: ");
    LogVersions::iterator print_ci_end = log_v->end();
    for (LogVersions::iterator print_ci = log_v->begin();
         print_ci != print_ci_end; ++ print_ci)
        fprintf(stderr, "%p ", (*print_ci).first);
    fprintf(stderr, "\n");
#endif
    
    LogStructure *cand_gh = 0;
    LogVersions::iterator logs_ci_end = log_v->end();
    LogVersions::iterator logs_ci = log_v->begin();
    LogVersions::iterator last_logs_ci = logs_ci_end; // sorta null value

#if defined(_FLUSH_GLOBAL_COMMIT) && !defined(DISABLE_FLUSHES) && \
    !defined(_DISABLE_DATA_FLUSH)
    bool did_cas_succeed = true;
#endif

    while (logs_ci != logs_ci_end)
    {
        LogStructure *gh = !is_recovery ?
            (LogStructure*) ALAR(*log_structure_header_p) : recovery_time_lsp;
        
        assert(gh);
        LogStructure *tmp_gh = gh;

        cand_gh = (*logs_ci).LS_;
        assert(cand_gh);
        LogStructure *tmp_cand_gh = cand_gh;

        const Log2Bool & deletable_logs = (*logs_ci).Del_;
        
        // Initially, gh is not part of log_v. But if a new gh is formed,
        // gh may actually be the same as cand_gh. In such a case, we just
        // need to skip to the next candidate. Additionally, we need to
        // check for logical identity, meaning that if logs for a new thread
        // were added in between "setting the GH last by the helper thread"
        // and "setting the GH by an application thread because of a new
        // thread creation", we need to identify that scenario as well.
        if (areLogicallySame(gh, cand_gh))
        {
            last_logs_ci = logs_ci;
            ++logs_ci;
            continue;
        }
        
        // Since the time of creation of cand_gh, entries may have been
        // added at the head of gh. By comparing the number of
        // *threads*, we can tell whether this has indeed happened. If
        // yes, the new entries are always found at the head.
        uint32_t num_new_entries = GetNumNewEntries(tmp_gh, tmp_cand_gh);

        // We can't just use "deletable_logs" to collect logs for
        // removal since the elements in "deletable_logs" may have
        // been split into different versions. For removal purposes,
        // we need to collect for only the versions that can be
        // removed. 
        LogEntryVec tmp_deleted_log_entries;
        LSVec new_entries;
        while (tmp_gh)
        {
            if (num_new_entries)
            {
                new_entries.push_back(tmp_gh);
                tmp_gh = tmp_gh->Next_;
                // Do not advance tmp_cand_gh
                --num_new_entries;
                continue;
            }
                
            LogEntry *curr_le = tmp_gh->Le_;

            assert(tmp_cand_gh);
            LogEntry *end_le = tmp_cand_gh->Le_;
            assert(end_le);

            do
            {
                assert(curr_le);

                // I believe we now support both creating and
                // destroying threads. Some of the logic is shown
                // below.
                
                // Check whether this log entry is scheduled to be
                // deleted. The cumulative list of deletable
                // log entries is found for every version and that is
                // used to check here. 

                if (deletable_logs.find(curr_le) == deletable_logs.end())
                    break;

                // Add it tentatively to the list of logs to be
                // deleted
                tmp_deleted_log_entries.push_back(curr_le);
                
                // This Next_ ptr has been read before, so no atomic
                // operation is required.
                curr_le = curr_le->Next_;
            }while (curr_le != end_le);

            if (done) break;

            // We compare the number of headers in GH and cand_GH and
            // if unequal, the difference at the head of GH are the
            // new entries. It appears that the old solution was
            // broken since a TCS might show up as deleted from a
            // version later than the cand_GH. For the above solution
            // to work, insertions must always be made at the head.
            tmp_gh = tmp_gh->Next_;
            tmp_cand_gh = tmp_cand_gh->Next_;
        }

        if (done) break;
        
        // Control coming here means that we can try deleting this version.
        // If new_entries have been found, we need to add those entries
        // to this version before deleting it.

        if (new_entries.size())
        {
            FixupNewEntries(&cand_gh, new_entries);
            (*logs_ci).LS_ = cand_gh;
        }

#if defined(_FLUSH_GLOBAL_COMMIT) && !defined(DISABLE_FLUSHES) && \
        !defined(_DISABLE_DATA_FLUSH)
        // TODO: The first time around, did_cas_succeed is always
        // true. Is this the correct behavior? If we have multiple
        // rounds of flushes, that would slow down the app
        if (did_cas_succeed) FlushCacheLines(tmp_deleted_log_entries);
#endif

        // Atomically flip the global log structure header pointer

        // Note that cache line flushing for this CAS is tricky. The current
        // ad-hoc solution is for user-threads to flush this
        // location after reading it. Since reads of this location are
        // rare enough, this should not hurt performance.
        LogStructure *oldval = !is_recovery ? 
            (LogStructure*) CAS(log_structure_header_p, gh, cand_gh) :
            (LogStructure*) CAS(&recovery_time_lsp, gh, cand_gh);
        if (oldval == gh)
        {
#if !defined(_DISABLE_LOG_FLUSH) && !defined(DISABLE_FLUSHES)
            if (!is_recovery)
                NVM_FLUSH(log_structure_header_p)
            else NVM_FLUSH(&recovery_time_lsp)
#endif
            
#if defined(_FLUSH_GLOBAL_COMMIT) && !defined(DISABLE_FLUSHES) && \
    !defined(_DISABLE_DATA_FLUSH)
            did_cas_succeed = true;
#endif            
            // During the first time through the top-level vector elements,
            // gh is not in log_v. So this special handling must be done
            // where we are destroying the old gh that is not in the vector.
            if (logs_ci == log_v->begin())
            {
                UtilVerboseTrace2(
                    helper_trace_file,
                    "[Atlas] Destroying a single header list\n");
                
                DestroyLS(gh);
            }
            else
            {
                assert(last_logs_ci != logs_ci_end);
                deleted_lsp.push_back(last_logs_ci);
            }

            // This builds the list of log entries which *will* be
            // deleted
            CopyDeletedLogEntries(&deleted_log_entries,
                                  tmp_deleted_log_entries);
            last_logs_ci = logs_ci;
            ++logs_ci;
        }
#if defined(_FLUSH_GLOBAL_COMMIT) && !defined(DISABLE_FLUSHES) && \
    !defined(_DISABLE_DATA_FLUSH)
        else did_cas_succeed = false;
#endif        
        // If the above CAS failed, we don't advance the version but
        // instead try again.
    }

    // Once the root GH pointer has been swung, it does not matter when
    // the log entries are removed or the other stuff is removed. No one can
    // reach these any more.
    DestroyLogEntries(deleted_log_entries);
    
    // No need for any cache flushes for any deletions.
    UtilVerboseTrace2(helper_trace_file,
                      "[Atlas] Destroying headers within a loop\n");
    
    LogIterVec::iterator del_ci_end = deleted_lsp.end();
    LogIterVec::iterator del_ci = deleted_lsp.begin();
    LogIterVec::iterator last_valid_ci;
    for (; del_ci != del_ci_end; ++ del_ci)
    {
        DestroyLS((**del_ci).LS_);
        last_valid_ci = del_ci;
    }

    del_ci = deleted_lsp.begin();
    if (del_ci != del_ci_end)
    {
        LogVersions::iterator lvi = *last_valid_ci;
        assert(lvi != log_v->end());
        ++lvi;
        (*log_v).erase(*del_ci, lvi);
    }
}

void DestroyLS(LogStructure * lsp)
{
    assert(lsp);
    if (!is_recovery)
        assert(lsp != (LogStructure*)ALAR(*log_structure_header_p));
    else assert(lsp != (LogStructure*)ALAR(recovery_time_lsp));
    
    UtilTrace2(helper_trace_file, "[Atlas] Destroying log header ");
    
    while (lsp)
    {
        LogStructure * del_lsp = lsp;
        lsp = lsp->Next_;

        UtilTrace3(helper_trace_file, "%p ", del_lsp);

        if (!is_recovery) nvm_free(del_lsp, true /* do not log */);
    }
    UtilTrace2(helper_trace_file, "\n");
}

#ifdef _PROFILE_HT
uint64_t total_log_destroy_cycles = 0;
#endif
void DestroyLogEntries(const LogEntryVec & le_vec)
{
#ifdef _PROFILE_HT    
    uint64_t start_log_destroy, end_log_destroy;
    start_log_destroy = atlas_rdtsc();
#endif

    UtilTrace2(helper_trace_file, "[Atlas] Destroying log entries ");

    LogEntryVec::const_iterator ci_end = le_vec.end();
    for (LogEntryVec::const_iterator ci = le_vec.begin(); ci != ci_end; ++ ci)
    {
        UtilTrace3(helper_trace_file, "%p ", *ci);

        if (isRelease(*ci))
        {
            AddEntryToDeletedMap(*ci, (*ci)->Size_);
            if (!is_recovery) DeleteOwnerInfo(*ci);
        }

        if (!is_recovery)
        {
#if defined(_LOG_WITH_MALLOC)
            if (isMemop(*ci)) free((void*)(*ci)->ValueOrPtr_);
            free(*ci);
#elif defined(_LOG_WITH_NVM_ALLOC)
            if (isMemop(*ci)) nvm_free((void*)(*ci)->ValueOrPtr_, true);
            nvm_free(*ci, true /* do not log */);
#else        
            if (isMemop(*ci)) nvm_free((void*)(*ci)->ValueOrPtr_, true);
            DeleteEntry<LogEntry>(cb_log_list, *ci);
#endif
        }
        ++removed_log_count;
    }
    
    UtilTrace2(helper_trace_file, "\n");

#ifdef _PROFILE_HT    
    end_log_destroy = atlas_rdtsc();
    total_log_destroy_cycles += end_log_destroy - start_log_destroy;
#endif    
}

void CopyDeletedLogEntries(LogEntryVec *deleted_les,
                    const LogEntryVec & tmp_deleted_les)
{
    LogEntryVec::const_iterator ci_end = tmp_deleted_les.end();
    for (LogEntryVec::const_iterator ci = tmp_deleted_les.begin();
         ci != ci_end; ++ ci)
        (*deleted_les).push_back(*ci);
}

void FixupNewEntries(LogStructure **cand, const LSVec & new_entries)
{
    LogStructure *new_header = 0;
    LogStructure *last_ls = 0;
    LSVec::const_iterator ci_end = new_entries.end();

    UtilVerboseTrace2(
        helper_trace_file, "[Atlas] Fixup: created header nodes: ");
    for (LSVec::const_iterator ci = new_entries.begin(); ci != ci_end; ++ci)
    {
        LogStructure *ls = (LogStructure *)
            nvm_alloc(sizeof(LogStructure), nvm_logs_id);
        assert(ls);

        UtilVerboseTrace3(helper_trace_file, "%p ", ls);
        
        ls->Le_ = (*ci)->Le_;
#if !defined(_DISABLE_LOG_FLUSH) && !defined(DISABLE_FLUSHES)
        NVM_FLUSH_ACQ(ls);
#endif
        if (!new_header) new_header = ls;
        else 
        {
            assert(last_ls);
            last_ls->Next_ = ls;
#if !defined(_DISABLE_LOG_FLUSH) && !defined(DISABLE_FLUSHES)
            NVM_FLUSH_ACQ(&last_ls->Next_);
#endif            
        }
        last_ls = ls;
    }

    UtilVerboseTrace2(helper_trace_file, "\n");
    
    last_ls->Next_ = *cand;
#if !defined(_DISABLE_LOG_FLUSH) && !defined(DISABLE_FLUSHES)
    NVM_FLUSH_ACQ(last_ls);
#endif
    (*cand) = new_header;
}

void AddEntryToDeletedMap(LogEntry *le, uint64_t gen_num)
{
    deleted_rel_logs.insert(make_pair(le, gen_num));
}

bool isDeletedByHelperThread(LogEntry *le, uint64_t gen_num)
{
    pair<DelIter,DelIter> del_iter = deleted_rel_logs.equal_range(le);
    for (DelIter di = del_iter.first; di != del_iter.second; ++di)
    {
        if (di->second == gen_num)
        {
            deleted_rel_logs.erase(di);
            // Must return since the iterator is broken by the above erase.
            // Any subsequent use of the iterator may fail.
            return true;
        }
    }
    return false;
}

// TODO
// (3) A deleted TCS needs to know which deleted header version it is
// associated with. This is required during garbage collection so that we
// know which TCSes to delete for a given version of the header. This may
// not be required since the versions will have to be maintained sequentially
// and you can look at the next version and its TCS pointers to understand
// when to stop. What if there is no next version. Then you can stop when
// the TCS is not marked deleted. Since all of this happens in a serial
// helper thread, it works.

extern SetOfInts *global_flush_ptr;
void Finalize_helper()
{
#if defined(_FLUSH_GLOBAL_COMMIT) && !defined(DISABLE_FLUSHES) && \
    !defined(_DISABLE_DATA_FLUSH)
    if (global_flush_ptr) delete global_flush_ptr;
#endif

#if defined(_NVM_TRACE) || defined(_NVM_VERBOSE_TRACE)
    assert(helper_trace_file);
    fclose(helper_trace_file);
#endif
}

#ifdef _NVM_TRACE
void PrintGraph(const DirectedGraph & dg)
{
    pair<VIter,VIter> vp;
    for (vp = vertices(dg); vp.first != vp.second; ++ vp.first)
    {
        VDesc nid = *vp.first;
        OcsMarker *ocs = dg[nid].OCS_;
        fprintf(helper_trace_file, "======================\n");
        fprintf(helper_trace_file, "\tNode id: %ld ", (size_t)nid);
        fprintf(helper_trace_file, "\tOCS: %p isStable: %d\n", ocs, dg[nid].isStable_);
        

        fprintf(helper_trace_file, "\tHere are the log records:\n");
        PrintLogs(ocs);

        pair<IEIter, IEIter> iep;
        int count = 0;
        fprintf(helper_trace_file, "\tHere are the sources:\n");
        for (iep = in_edges(nid, dg); iep.first != iep.second; ++ iep.first)
        {
            EDesc eid = *iep.first;
            VDesc src = source(eid, dg);
            OcsMarker *src_ocs = dg[src].OCS_;
            ++count;
            fprintf(helper_trace_file, "\t\tNode id: %ld OCS: %p\n",
                    (size_t)src, src_ocs);
        }
        fprintf(helper_trace_file, "\t# incoming edges is %d\n", count);
    }
}

void PrintLogs(OcsMarker *ocs)
{
    LogEntry *current_le = ocs->First_;
    assert(current_le);

    LogEntry *last_le = ocs->Last_;
    assert(last_le);

    do
    {
        switch(current_le->Type_) 
        {
            case LE_dummy:
                PrintDummyLog(current_le);
                break;
            case LE_acquire:
                PrintAcqLog(current_le);
                break;
            case LE_rwlock_rdlock:
                PrintRdLockLog(current_le);
                break;
            case LE_rwlock_wrlock:
                PrintWrLockLog(current_le);
                break;
            case LE_begin_durable:
                PrintBeginDurableLog(current_le);
                break;
            case LE_release:
                PrintRelLog(current_le);
                break;
            case LE_rwlock_unlock:
                PrintRWUnlockLog(current_le);
                break;
            case LE_end_durable:
                PrintEndDurableLog(current_le);
                break;
            case LE_str:
                PrintStrLog(current_le);
                break;
            case LE_memset:
            case LE_memcpy:
            case LE_memmove:
                PrintMemOpLog(current_le);
                break;
            default:
                assert(0);
        }
        if (current_le == last_le) break;
        current_le = current_le->Next_;
    }while (true);
}

void PrintDummyLog(LogEntry *le)
{
    fprintf(helper_trace_file, "\t\tle=%p type=dummy\n", le);
}

void PrintAcqLog(LogEntry *le)
{
    fprintf(helper_trace_file, "\t\tle=%p lock=%p ha=%p type=acq next=%p\n",
            le, le->Addr_, (intptr_t*)(le->ValueOrPtr_), le->Next_);
}

// TODO complete
void PrintRdLockLog(LogEntry *le)
{
    fprintf(helper_trace_file, "\t\tle=%p lock=%p type=rw_rd next=%p\n",
            le, le->Addr_, le->Next_);
}

// TODO complete
void PrintWrLockLog(LogEntry *le)
{
    fprintf(helper_trace_file, "\t\tle=%p lock=%p type=rw_wr next=%p\n",
            le, le->Addr_, le->Next_);
}

void PrintBeginDurableLog(LogEntry *le)
{
    fprintf(helper_trace_file, "\t\tle=%p type=begin_durable\n", le);
}

void PrintRelLog(LogEntry *le)
{
    assert(!le->ValueOrPtr_);
    fprintf(helper_trace_file, "\t\tle=%p lock=%p type=rel gen=%ld next=%p\n",
            le, le->Addr_, le->Size_, le->Next_);
}

// TODO complete
void PrintRWUnlockLog(LogEntry *le)
{
    fprintf(helper_trace_file, "\t\tle=%p lock=%p type=rw_unlock next=%p\n",
            le, le->Addr_, le->Next_);
}

void PrintEndDurableLog(LogEntry *le)
{
    fprintf(helper_trace_file, "\t\tle=%p type=end_durable\n", le);
}

void PrintStrLog(LogEntry *le)
{
    fprintf(helper_trace_file,
            "\t\tle=%p addr=%p val=%ld size=%ld type=str next=%p\n",
            le, le->Addr_, le->ValueOrPtr_, le->Size_, le->Next_);
}

// TODO complete
void PrintMemOpLog(LogEntry *le)
{
    fprintf(helper_trace_file,
            "\t\tle=%p addr=%p size=%ld type=memop next=%p\n",
            le, le->Addr_, le->Size_, le->Next_);
}
#endif
