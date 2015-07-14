#ifndef _HELPER_H
#define _HELPER_H

#include <vector>
#include <map>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

#include "atlas_alloc.h"
#include "region_internal.h"
#include "log_structure.h"

using namespace std;
using namespace boost;

#ifdef _NVM_VERBOSE_TRACE
#define GraphTrace(arg) PrintGraph(arg)
#else
#define GraphTrace(arg)
#endif

typedef struct OcsMarker
{
    LogEntry *First_;
    LogEntry *Last_;
    OcsMarker *Next_;
    bool isDeleted_;
}OcsMarker;

typedef struct VProp
{
    OcsMarker *OCS_;
    intptr_t Sid_;
    bool isStable_;
}VProp;

enum NODE_TYPE {NT_avail, NT_absent, NT_deleted};

typedef adjacency_list<setS, listS, bidirectionalS, VProp> DirectedGraph;
typedef graph_traits<DirectedGraph>::vertex_descriptor VDesc;
typedef graph_traits<DirectedGraph>::edge_descriptor EDesc;
typedef graph_traits<DirectedGraph>::vertex_iterator VIter;
typedef graph_traits<DirectedGraph>::in_edge_iterator IEIter;

#ifdef DEBUG
typedef adjacency_list<setS, listS, bidirectionalS> SimpleDG;
#endif

typedef struct NodeInfo
{
    VDesc Nid_;
    intptr_t Sid_;
    NODE_TYPE Nt_;
}NodeInfo;

typedef struct LogVer
{
    LogStructure *LS_;
    Log2Bool Rel_;
    Log2Bool Del_;
}LogVer;

typedef pair<LogEntry *,VDesc> PendingPair;
typedef vector<PendingPair> PendingList;
typedef map<LogEntry *, NodeInfo> NodeInfoMap;
typedef map<VDesc, bool> MapNodes;
typedef vector<LogStructure *> LSVec;
typedef vector<LogVer> LogVersions;
typedef vector<LogVersions::iterator> LogIterVec;
typedef map<intptr_t *, bool> Addr2Bool;
typedef map<LogStructure*, OcsMarker*> OcsMap;
typedef vector<OcsMarker*> OcsVec;
typedef multimap<LogEntry*, uint64_t> MultiMapLog2Int;
typedef MultiMapLog2Int::iterator DelIter;

VDesc CreateNode(DirectedGraph*, OcsMarker*, intptr_t);
EDesc CreateEdge(DirectedGraph*, VDesc, VDesc);
OcsMarker *CreateOcsMarker(LogEntry*);
void AddOcsMarkerToMap(OcsMap*, LogStructure*, OcsMarker*);
void DeleteOcsMarkerFromMap(OcsMap*, LogStructure*);
OcsMarker *GetOcsMarkerFromMap(const OcsMap &, LogStructure*);
void AddOcsMarkerToVec(OcsVec*, OcsMarker*);
void DestroyOcses(OcsVec*);
void AddToPendingList(PendingList *, LogEntry *, VDesc);
void AddToNodeInfoMap(NodeInfoMap *, LogEntry *, VDesc, intptr_t, NODE_TYPE);
pair<VDesc,NODE_TYPE> GetTargetNodeInfo(
    const DirectedGraph &, const NodeInfoMap &, LogEntry *);
void ResolvePendingList(PendingList *, const NodeInfoMap &,
                        DirectedGraph *, const LogVersions &);
void RemoveUnresolvedNodes(DirectedGraph*, const PendingList &,
                           LogEntryVec*, Log2Bool*, Log2Bool*);
void HandleUnresolved(DirectedGraph *, VDesc, MapNodes *, int *);
void CreateVersions(const DirectedGraph &, LogVersions*,
                    const LogEntryVec &, Log2Bool *rel_map,
                    const Log2Bool & acq_map, const OcsMap & ocs_map);
void CreateMapFromOwnerTab(Log2Bool *);
void DestroyLogs(LogVersions *);
bool hasReferenceFromOwnerTab(LogEntry*, const Log2Bool &);
bool isNewEntry(LogStructure*);
void FixupNewEntries(LogStructure **, const LSVec &);
void CopyDeletedLogEntries(LogEntryVec*, const LogEntryVec &);
void DestroyLogEntries(const LogEntryVec &);
void DestroyLS(LogStructure *);
void TrackLog(LogEntry * le, Log2Bool * log_map);
void RemoveLog(LogEntry * le, Log2Bool * log_map);
void RemoveAcquiresAndReleases(const DirectedGraph & dg, VDesc nid,
                               Log2Bool * acq_map, Log2Bool * rel_map);
void PruneReleaseLog(Log2Bool * rel_map, const Log2Bool & acq_map);
void NullifyAcqToDeletedRel(LogEntry *, const LogVersions &);
void AddEntryToDeletedMap(LogEntry*, uint64_t);
bool isDeletedByHelperThread(LogEntry*, uint64_t);
void Finalize_helper();

void PrintGraph(const DirectedGraph & dg);
void PrintLogs(OcsMarker *ocs);
void PrintDummyLog(LogEntry*);
void PrintAcqLog(LogEntry*);
void PrintRdLockLog(LogEntry*);
void PrintWrLockLog(LogEntry*);
void PrintBeginDurableLog(LogEntry*);
void PrintRelLog(LogEntry*);
void PrintRWUnlockLog(LogEntry*);
void PrintEndDurableLog(LogEntry*);
void PrintStrLog(LogEntry*);
void PrintMemOpLog(LogEntry*);
#endif
