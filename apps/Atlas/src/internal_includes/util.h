#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>
#include <string.h>
#include <map>
#include <set>

#include "atomic_ops.h"

using namespace std;

// TODO use getconf for portability
#define DCACHE_LINESIZE 64
#define MAXLEN DCACHE_LINESIZE
#define TRACE_GRANULE 1000

typedef volatile intptr_t vintp;
typedef volatile size_t vsz;
typedef volatile uint32_t vint;

#define ALAR(var) \
    AO_load_acquire_read((vsz*)(void*)&(var))
#define ALAR_int(var) \
    AO_int_load_acquire_read((vint*)&(var))
#define ASRW(var,val) \
    AO_store_release_write((vsz*)(void*)&(var), (size_t)(void*)(val))
#define ASRW_int(var,val) \
    AO_int_store_release_write((vint*)&(var), (uint32_t)(val))

#define msg_assert(msg, c) \
    if (!(c))            \
    {                    \
        fprintf(stderr, "%s\n", msg);           \
        assert(0);                              \
    }                                           \

#ifdef _NVM_TRACE
#define UtilTrace2(a1, a2) AtlasTrace(a1, a2)
#define UtilTrace3(a1, a2, a3) AtlasTrace(a1, a2, a3)
#define UtilTrace4(a1, a2, a3, a4) AtlasTrace(a1, a2, a3, a4)
#else
#define UtilTrace2(a1, a2)
#define UtilTrace3(a1, a2, a3)
#define UtilTrace4(a1, a2, a3, a4)
#endif

#ifdef _NVM_VERBOSE_TRACE
#define UtilVerboseTrace2(a1, a2) AtlasTrace(a1, a2)
#define UtilVerboseTrace3(a1, a2, a3) AtlasTrace(a1, a2, a3)
#define UtilVerboseTrace4(a1, a2, a3, a4) AtlasTrace(a1, a2, a3, a4)
#else
#define UtilVerboseTrace2(a1, a2) 
#define UtilVerboseTrace3(a1, a2, a3) 
#define UtilVerboseTrace4(a1, a2, a3, a4)
#endif

#if defined(_NVM_TRACE) || defined(_NVM_VERBOSE_TRACE)
#define InitializeThreadLocalTraceFile(p) InitializeTLFile((p))
#else
#define InitializeThreadLocalTraceFile(p)
#endif

extern FILE *helper_trace_file;

__inline__ intptr_t
cas (volatile intptr_t *ptr, intptr_t oldVal, intptr_t newVal)
{
    intptr_t prevVal;

    __asm__ __volatile__ (
        "lock \n"
#ifdef __LP64__
        "cmpxchgq %1,%2 \n"
#else
        "cmpxchgl %k1,%2 \n"
#endif
        : "=a" (prevVal)
        : "q"(newVal), "m"(*ptr), "0" (oldVal)
        : "memory"
    );

    return prevVal;
}

#define CAS(p,o,n) cas((volatile intptr_t*)(p),(intptr_t)(o),(intptr_t)(n))

typedef pair<uint64_t,uint64_t> UInt64Pair;
class CmpUInt64
{
public:
    bool operator()(const UInt64Pair & c1, const UInt64Pair & c2) const
        {
            return (c1.first < c2.first) && (c1.second < c2.second);
        }
};
typedef map<UInt64Pair,uint32_t,CmpUInt64> MapInterval;

typedef pair<void*,size_t> AddrSizePairType;
class CmpAddrSizePair
{
public:
    bool operator()(const AddrSizePairType & c1, const AddrSizePairType & c2) const 
        {
            if ((uintptr_t)c1.first < (uintptr_t)c2.first) return true;
            if ((uintptr_t)c1.first > (uintptr_t)c2.first) return false;
            if (c1.second < c2.second) return true;
            return false;
        }
};
typedef set<AddrSizePairType, CmpAddrSizePair> SetOfPairs;

// This is not thread safe. Currently ok to call from recovery code but
// not from anywhere else.
inline void InsertToMapInterval(
    MapInterval *m, uint64_t e1, uint64_t e2, uint32_t e3)
{
    (*m)[make_pair(e1,e2)] = e3;
}

// This can be called from anywhere.
inline MapInterval::const_iterator FindInMapInterval(
    const MapInterval & m, const uint64_t e1, const uint64_t e2)
{
    return m.find(make_pair(e1, e2));
}

inline void InsertSetOfPairs(SetOfPairs *setp, void *addr, size_t sz)
{
    (*setp).insert(make_pair(addr, sz));
}

inline SetOfPairs::const_iterator FindSetOfPairs(
    const SetOfPairs & setp, void *addr, size_t sz)
{
    return setp.find(make_pair(addr, sz));
}

typedef set<uint64_t> SetOfInts;

extern __thread SetOfInts *tl_flush_ptr;
extern __thread SetOfPairs *tl_uniq_loc;

template <class ElemType>
class SimpleHashTable
{
public:
    SimpleHashTable(uint32_t size=0)
        {
            if (size) Size_ = size;
            Tab_ = (intptr_t * volatile *) new intptr_t * volatile [Size_];
            memset((void*)Tab_, 0, Size_*sizeof(intptr_t * volatile));
        }
    void Insert(void*, const ElemType &);
    ElemType *Find(void*);
private:
    class ElemInfo
    {
    public:
        ElemInfo(void *addr, const ElemType & elem)
            { Addr_ = addr; Elem_ = new ElemType(elem); Next_ = 0; }
        void *Addr_;
        ElemType *Elem_;
        ElemInfo *Next_;
    };
    intptr_t * volatile *Tab_;
    static uint32_t Size_;
    intptr_t * volatile *GetTableEntry(void *addr)
        { return (Tab_ + (((uintptr_t(addr)+128) >> 3) & (Size_-1))); }
    ElemInfo *GetElemInfoHeader(void *addr)
        {
            intptr_t * volatile *entry = GetTableEntry(addr);
            return (ElemInfo * volatile) ALAR(*entry);
        }
    ElemInfo * volatile * GetPointerToElemInfoHeader(void *addr)
        {
            intptr_t * volatile *entry = GetTableEntry(addr);
            return (ElemInfo * volatile *) entry;
        }
};

// This insertion interface is thread safe since it employs a
// copy-on-write implementation. However, the old map needs to be
// GC-ed, today it is leaked.
void NVM_CowAddToMapInterval(MapInterval * volatile *ptr,
                             uint64_t e1, uint64_t e2, uint32_t e3);
void NVM_CowDeleteFromMapInterval(MapInterval * volatile *ptr,
                                  uint64_t e1, uint64_t e2, uint32_t e3);

void FlushCacheLines(const SetOfInts &);

void AtlasTrace(FILE *, const char *fmt, ...);

const char *NVM_GetLogDir();
void NVM_CreateLogDir();
char *NVM_GetFullyQualifiedRegionName(const char *name);

// Derive the log name from the program name for the running process
char *NVM_GetLogRegionName(); 
char *NVM_GetLogRegionName(const char *prog_name);
bool NVM_doesLogExist(const char *log_path_name);
void NVM_qualifyPathName(char *s, const char *name);

void PrintBackTrace();
void InitializeTLFile(FILE **);

#endif
