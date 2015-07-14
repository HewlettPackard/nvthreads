#ifndef _ATLAS_API_H
#define _ATLAS_API_H

#include <stdlib.h>
#include <stdint.h>

#include "atomic_ops.h"

extern uint32_t cache_line_size;
extern uint64_t cache_line_mask;

#ifdef NVM_STATS
extern __thread uint64_t num_flushes;
#endif

#define NVM_LOG(var, size) {                                \
        nvm_store((void*)&(var), (size));                   \
    }                                                       \

#define NVM_LOCK(lock) {                                    \
        pthread_mutex_lock(&(lock));                        \
        nvm_acquire((void*)&(lock));                        \
    }                                                       \
        
#define NVM_UNLOCK(lock) {                                  \
        nvm_release((void*)&(lock));                        \
        pthread_mutex_unlock(&(lock));                      \
    }                                                       \
        
#define NVM_RWLOCK_RDLOCK(rwlock) {                         \
        pthread_rwlock_rdlock(&(rwlock));                   \
        nvm_rwlock_rdlock((void *)&(rwlock));               \
    }                                                       \
        
#define NVM_RWLOCK_WRLOCK(rwlock) {                         \
        pthread_rwlock_wrlock(&(rwlock));                   \
        nvm_rwlock_wrlock((void *)&(rwlock));               \
    }                                                       \
        
#define NVM_RWLOCK_UNLOCK(rwlock) {                         \
        nvm_rwlock_unlock((void *)&(rwlock));               \
        pthread_rwlock_unlock(&(rwlock));                   \
    }                                                       \
        
#define NVM_BEGIN_DURABLE() nvm_begin_durable()

#define NVM_END_DURABLE() nvm_end_durable()

#if defined(_FLUSH_LOCAL_COMMIT) || defined(_FLUSH_GLOBAL_COMMIT)

#define NVM_STR(var,val,size) {                             \
        nvm_store((void*)&(var),((size)*8));                \
        var=val;                                            \
    }                                                       \
        
#define NVM_STR2(var,val,size) {                            \
        nvm_store((void*)&(var),(size));                    \
        var=val;                                            \
    }                                                       \

#define NVM_MEMSET(s,c,n) {                                 \
        nvm_memset((void *)(s), (size_t)(n));               \
        memset(s, c, n);                                    \
    }                                                       \
        
#define NVM_MEMCPY(dst, src, n) {                           \
        nvm_memcpy((void *)(dst), (size_t)(n));             \
        memcpy(dst, src, n);                                \
    }                                                       \

#define NVM_MEMMOVE(dst, src, n) {                          \
        nvm_memmove((void *)(dst), (size_t)(n));            \
        memmove(dst, src, n);                               \
    }                                                       \
    
#elif defined(_DISABLE_DATA_FLUSH)

#define NVM_STR(var,val,size) {                             \
        nvm_store((void*)&(var),((size)*8));                \
        var=val;                                            \
    }                                                       \

#define NVM_STR2(var,val,size) {                            \
        nvm_store((void*)&(var),(size));                    \
        var=val;                                            \
    }                                                       \

#define NVM_MEMSET(s,c,n) {                                 \
        nvm_memset((void *)(s), (size_t)(n));               \
        memset(s, c, n);                                    \
    }                                                       \
        
#define NVM_MEMCPY(dst, src, n) {                           \
        nvm_memcpy((void *)(dst), (size_t)(n));             \
        memcpy(dst, src, n);                                \
    }                                                       \
        
#define NVM_MEMMOVE(dst, src, n) {                          \
        nvm_memmove((void *)(dst), (size_t)(n));            \
        memmove(dst, src, n);                               \
    }                                                       \

#elif defined(_USE_TABLE_FLUSH)

#define NVM_STR(var,val,size) {                             \
        nvm_store((void*)&(var),((size)*8));                \
        var=val;                                            \
        AsyncDataFlush((void*)&(var));                      \
    }                                                       \

#define NVM_STR2(var,val,size) {                            \
        nvm_store((void*)&(var),(size));                    \
        var=val;                                            \
        AsyncDataFlush((void*)&(var));                      \
    }                                                       \

#define NVM_MEMSET(s,c,n) {                                 \
        nvm_memset((void *)(s), (size_t)(n));               \
        memset(s, c, n);                                    \
        AsyncMemOpDataFlush((void*)(s), n);                 \
    }                                                       \
        
#define NVM_MEMCPY(dst, src, n) {                           \
        nvm_memcpy((void *)(dst), (size_t)(n));             \
        memcpy(dst, src, n);                                \
        AsyncMemOpDataFlush((void*)(dst), n);               \
    }                                                       \
        
#define NVM_MEMMOVE(dst, src, n) {                          \
        nvm_memmove((void *)(dst), (size_t)(n));            \
        memmove(dst, src, n);                               \
        AsyncMemOpDataFlush((void*)(dst), n);               \
    }                                                       \

#else

// TODO for transient locations, a filter avoids logging and flushing.
// Currently, this filter is being called twice. This should be optimized
// to call once only. Ensure that the fix is done in the compiled code-path
// as well, i.e. it should be there for nvm_barrier as well. This implies
// that the compiler needs changes.

#define NVM_STR(var,val,size) {                             \
        nvm_store((void*)&(var),((size)*8));                \
        var=val;                                            \
        NVM_FLUSH_ACQ_COND(&var);                           \
    }                                                       \

#define NVM_STR2(var,val,size) {                            \
        nvm_store((void*)&(var),(size));                    \
        var=val;                                            \
        NVM_FLUSH_ACQ_COND(&var);                           \
    }                                                       \

#define NVM_MEMSET(s,c,n) {                                 \
        nvm_memset((void *)(s), (size_t)(n));               \
        memset(s, c, n);                                    \
        NVM_PSYNC_ACQ_COND(s, n);                           \
    }                                                       \
        
#define NVM_MEMCPY(dst, src, n) {                           \
        nvm_memcpy((void *)(dst), (size_t)(n));             \
        memcpy(dst, src, n);                                \
        NVM_PSYNC_ACQ_COND(dst, n);                         \
    }                                                       \
        
#define NVM_MEMMOVE(dst, src, n) {                          \
        nvm_memmove((void *)(dst), (size_t)(n));            \
        memmove(dst, src, n);                               \
        NVM_PSYNC_ACQ_COND(dst, n);                         \
    }                                                       \

#endif 

#ifdef NVM_STATS
static __inline void NVM_IncrementNumFlushes()
{
    ++num_flushes;
}
#endif

#define NVM_CLFLUSH(p) nvm_clflush((char*)(void*)(p))

#ifndef DISABLE_FLUSHES
#define NVM_FLUSH(p)                                        \
    { AO_nop_full(); NVM_CLFLUSH((p)); AO_nop_full(); }     \

#define NVM_FLUSH_COND(p)                                   \
    { if (NVM_IsInOpenPR(p, 1)) {                           \
            AO_nop_full(); NVM_CLFLUSH((p)); AO_nop_full(); \
        }                                                   \
    }                                                       \

#define NVM_FLUSH_ACQ(p)                                    \
    { AO_nop_full(); NVM_CLFLUSH(p); }
        
#define NVM_FLUSH_ACQ_COND(p)                               \
    { if (NVM_IsInOpenPR(p, 1)) {                           \
            AO_nop_full(); NVM_CLFLUSH(p);                  \
        }                                                   \
    }                                                       \
        
#define NVM_PSYNC(p1,s) nvm_psync(p1,s)

#define NVM_PSYNC_COND(p1,s)                                \
    { if (NVM_IsInOpenPR(p1, s)) nvm_psync(p1,s); }

#define NVM_PSYNC_ACQ(p1,s)                                 \
    {                                                       \
        nvm_psync_acq(p1,s);                                \
    }                                                       \

#define NVM_PSYNC_ACQ_COND(p1,s)                            \
    {                                                       \
        if (NVM_IsInOpenPR(p1, s)) nvm_psync_acq(p1, s);    \
    }                                                       \
        
#else
#define NVM_FLUSH(p)
#define NVM_FLUSH_COND(p)
#define NVM_FLUSH_ACQ(p)
#define NVM_FLUSH_ACQ_COND(p)
#define NVM_PSYNC(p1,s)
#define NVM_PSYNC_COND(p1,s)
#define NVM_PSYNC_ACQ(p1,s)
#define NVM_PSYNC_ACQ_COND(p1,s)
#endif

static __inline void nvm_clflush(const void *p)
{
#ifndef DISABLE_FLUSHES    
#ifdef NVM_STATS    
    NVM_IncrementNumFlushes();
#endif    
    __asm__ __volatile__ (
        "clflush %0 \n" : "+m" (*(char*)(p))
        );
#endif    
}
static __inline void full_fence() {
    __asm__ __volatile__ ("mfence" ::: "memory");
  }


// Return true if the addresses are on different cache lines, otherwise false.
// Keep in mind that the data entities you are interested in must not cross
// cache lines, otherwise this interface is inadequate.
static __inline int isOnDifferentCacheLine(void *p1, void *p2)
{
    return ((uint64_t)p1 & cache_line_mask) !=
        ((uint64_t)p2 & cache_line_mask);
}

static __inline int isCacheLineAligned(void *p)
{
    return ((uint64_t)p == ((uint64_t)p & cache_line_mask));
}

#ifdef __cplusplus
extern "C" {
#endif
    
    void NVM_Initialize();
    int NVM_IsInOpenPR(void *addr, size_t sz /* in bytes */);
    void NVM_SetCacheParams();
    void nvm_acquire(void *lock_address);
    void nvm_rwlock_rdlock(void *lock_address);
    void nvm_rwlock_wrlock(void *lock_address);
    void nvm_release(void *lock_address);
    void nvm_rwlock_unlock(void *lock_address);
    void nvm_begin_durable();
    void nvm_end_durable();
    void nvm_store(void *addr, size_t size);
    void nvm_store_int8(void *addr, uint8_t old_val);
    void nvm_store_int16(void *addr, uint16_t old_val);
    void nvm_store_int32(void *addr, uint32_t old_val);
    void nvm_store_int64(void *addr, uint64_t old_val);
    void nvm_store_float(void *addr, float old_val);
    void nvm_store_double(void *addr, double old_val);
    void nvm_memset(void *addr, size_t sz);
    void nvm_memcpy(void *dst, size_t sz);
    void nvm_memmove(void *dst, size_t sz);
    // TODO nvm_barrier should really be inlined. For that to happen,
    // the compiler must manually inline its instructions. Don't use this
    // interface within the library, instead use NVM_FLUSH
    void nvm_barrier(void*);
    void nvm_psync(void*, size_t);
    void nvm_psync_acq(void*, size_t);
    void NVM_Finalize();
    void SyncLogFlush();
    void AsyncDataFlush(void*);
    void AsyncMemOpDataFlush(void*, size_t);
    void SyncDataFlush();
#ifdef NVM_STATS    
    // not part of API, for short-term testing?
    void NVM_PrintStats();
    void NVM_PrintNumFlushes();
#endif
    
#ifdef __cplusplus
}
#endif    

#endif
