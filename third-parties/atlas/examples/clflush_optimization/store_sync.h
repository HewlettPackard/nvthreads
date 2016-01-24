#define STSY_TABLE_SZ 16 /* Maximum number of to-be-flushed lines */
			/* Must be power of 2.			 */
#define STSY_LOG_MIN_LINE_SZ 6 /* Log(smallest supported cache line sz) */
#define STSY_MIN_LINE_SZ (1 << STSY_LOG_MIN_LINE_SZ)

#include <inttypes.h>
#include <stdlib.h>

extern int NUM_CLFLUSH;
extern int NUM_SYNC_CLFLUSH;
extern int NUM_SYNCS;

/* From Alistair: */
static inline void STSY_clflush(intptr_t p) {
    NUM_CLFLUSH++;
    asm volatile("clflush %0" :: "m"(*(char *)p) : "memory");  /* The gcc/x86 way */
}

#if 0
  static inline void STSY_full_fence() {
    atomic_thread_fence();  /* The closest C11 construct */
  }
#elif 0
  static inline void STSY_full_fence() {
    asm volatile("dmb" ::: "memory"); /* Possibly the ARM way */
  }
#else
  static inline void STSY_full_fence() {
    asm volatile("mfence" ::: "memory"); /* The gcc/x86 way */
  }
#endif

/* Table of cache lines that have yet to be flushed.  All entries are	*/
/* addresses of the first byte of a MIN_LINE_SZ sized cache line,	*/
/* right-shifted by STSY_LOG_MIN_LINE_SZ bits.				*/
#ifdef THREADS
  #if 0
     thread_local /* C11 spelling */
  #else
     extern __thread intptr_t STSY_not_yet_flushed[STSY_TABLE_SZ];
  #endif
#else
  extern intptr_t STSY_not_yet_flushed[STSY_TABLE_SZ];
#endif /* THREADS */

/* Compute a hash from a cache line, i.e. right shifted address. */
static inline intptr_t STSY_hash(intptr_t cl) {
  return cl & (intptr_t)(STSY_TABLE_SZ - 1);
}

static inline intptr_t STSY_cache_line(void *p) {
  return (intptr_t)(p) >> STSY_LOG_MIN_LINE_SZ;
}

/* Define STSY_store_int(int *, int), STSY_store_ptr(void **, void *) etc. */
/* These perform the indicated assignment, and remember that it needs	   */
/* to be flushed.							   */

/* Poor man's template.  Convert to C++? */
#define STSY_GEN_STORE(T) \
  static inline T STSY_store_##T(T *p, T val) { \
    intptr_t ln = STSY_cache_line(p); /* shifted address */ \
    intptr_t *entry_ptr = STSY_not_yet_flushed + STSY_hash(ln); \
    intptr_t entry = *entry_ptr; \
    assert(STSY_cache_line((char *)(p+1) - 1) == ln); /* aligned */\
    if (entry != ln) { \
      if (entry == 0) { \
	*entry_ptr = ln; \
      } else { \
        STSY_full_fence(); \
        STSY_clflush(entry << STSY_LOG_MIN_LINE_SZ); \
        *entry_ptr = ln; \
      } \
    } \
    assert(*entry_ptr == ln); \
    return (*p = val); \
  }
    
STSY_GEN_STORE(long);
STSY_GEN_STORE(int);
STSY_GEN_STORE(short);
STSY_GEN_STORE(char);
STSY_GEN_STORE(double);
STSY_GEN_STORE(float);
typedef void *STSY_ptr;
STSY_GEN_STORE(STSY_ptr);
static inline void * STSY_store_ptr(void **p, void *val) {
  return STSY_store_STSY_ptr(p, val);
}

/* Perform all outstanding flushes, thus ensuring that every prior	*/
/* STSY_store_* call has been flushed.					*/
void STSY_sync_all(void);
void* STSY_memcpy(void * destination, const void * source, size_t num);
