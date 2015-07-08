/* -*- C++ -*- */

#ifndef LOCK_H_
#define LOCK_H_

#if defined(unix)
#include <sched.h>
#endif

#if defined(__SVR4)
#include <thread.h>
#endif

#if defined(__sgi)
#include <mutex.h>
#endif

#if defined(__APPLE__)
#include <libkern/OSAtomic.h>
#endif

#include "cpuinfo.h"

#if defined(_MSC_VER)

#if !defined(NO_INLINE)
#pragma inline_depth(255)
#define NO_INLINE __declspec(noinline)
#define INLINE __forceinline
#define inline __forceinline
#endif // !defined(NO_INLINE)

#else

#endif // defined(_MSC_VER)


#if defined(__SUNPRO_CC)
// x86-interchange.il, x86_64-interchange.il contributed by Markus Bernhardt.
extern "C" unsigned long MyInterlockedExchange (unsigned long * oldval,
					      unsigned long newval);
#endif

#if defined(_WIN32) && !defined(_WIN64)
#define _WIN32_WINNT 0x0500

// NOTE: Below is the new "pause" instruction, which is inocuous for
// previous architectures, but crucial for Intel chips with
// hyperthreading.  See
// http://www.usenix.org/events/wiess02/tech/full_papers/nakajima/nakajima.pdf
// for discussion.

#define _MM_PAUSE  {__asm{_emit 0xf3};__asm {_emit 0x90}}
#include <windows.h>
#else
#define _MM_PAUSE
#endif // defined(_WIN32) && !defined(_WIN64)

extern volatile int anyThreadCreated;

namespace HL {

class Lock {
private:

  enum { UNLOCKED = 0, LOCKED = 1 };

public:
  
  Lock (void)
#if defined(__APPLE__)
    // : mutex (UNLOCKED)
    : mutex (OS_SPINLOCK_INIT)
#else
    : mutex (UNLOCKED)
#endif
  {}
  
  ~Lock (void)
  {}

  inline void lock (void) {
    if (anyThreadCreated) {
      if (MyInterlockedExchange (const_cast<unsigned long *>(&mutex), LOCKED)
	  != UNLOCKED) {
	contendedLock();
      }
    } else {
      mutex = LOCKED;
    }
  }



  inline void unlock (void) {
    if (anyThreadCreated) {
#if defined(_WIN32) && !defined(_WIN64)
      __asm {}
#elif defined(__GNUC__)
      asm volatile ("" : : : "memory");
#endif
    }
    mutex = UNLOCKED;
  }


#if !defined(__SUNPRO_CC)
  inline static unsigned long MyInterlockedExchange (unsigned long *,unsigned long); 
#endif

private:

#if 0 // defined(__APPLE__)
  OSSpinLock mutex;

#else

  NO_INLINE
  void contendedLock (void) {
    while (true) {
      if (MyInterlockedExchange (const_cast<unsigned long *>(&mutex), LOCKED)
	  == UNLOCKED) {
	return;
      }
      while (mutex == LOCKED) {
	_MM_PAUSE;
	printf ("waiting\n");
	//	yieldProcessor();
      }
    }
  }

  // Is this system a multiprocessor?
  inline bool onMultiprocessor (void) {
    static CPUInfo cpuInfo;
    return (cpuInfo.getNumProcessors() > 1);
  }

  inline void yieldProcessor (void) {
#if defined(_WIN32)
    Sleep(0);
#elif defined(__SVR4)
    thr_yield();
#else
    sched_yield();
#endif
  }

  enum { MAX_SPIN_LIMIT = 1024 };

  volatile unsigned long mutex;
#endif

};

}

// Atomically:
//   retval = *oldval;
//   *oldval = newval;
//   return retval;

#if !defined(__SUNPRO_CC) // && !defined(__APPLE__)
inline unsigned long 
HL::Lock::MyInterlockedExchange (unsigned long * oldval,
					 unsigned long newval)
{
#if defined(_WIN32) && defined(_MSC_VER)
  return InterlockedExchange ((volatile LONG *) oldval, newval);

  //#elif (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100
  //  return __sync_val_compare_and_swap(oldval, *oldval, newval);

#elif defined(__sparc)
  asm volatile ("swap [%1],%0"
		:"=r" (newval)
		:"r" (oldval), "0" (newval)
		: "memory");
  
#elif defined(__i386__)
  asm volatile ("lock; xchgl %0, %1"
		: "=r" (newval)
		: "m" (*oldval), "0" (newval)
		: "memory");

#elif defined(__sgi)
  newval = test_and_set (oldval, newval);

#elif defined(__x86_64__)
  // Contributed by Kurt Roeckx.
  asm volatile ("lock; xchgq %0, %1"
		: "=r" (newval)
		: "m" (*oldval), "0" (newval)
		: "memory");

#elif defined(__ppc) || defined(__powerpc__) || defined(PPC)
  // PPC assembly contributed by Maged Michael.
  int ret; 
  asm volatile ( 
		"La..%=0:    lwarx %0,0,%1 ;" 
		"      cmpw  %0,%2;" 
		"      beq La..%=1;" 
		"      stwcx. %2,0,%1;" 
		"      bne- La..%=0;" 
		"La..%=1:    isync;" 
                : "=&r"(ret) 
                : "r"(oldval), "r"(newval) 
                : "cr0", "memory"); 
  return ret;

#elif defined(__arm__)
  // Contributed by Bo Granlund.
  long result;
  asm volatile (
		       "\n\t"
		       "swp     %0,%2,[%1] \n\t"
		       ""
		       : "=&r"(result)
		       : "r"(oldval), "r"(newval)
		       : "memory");
  return (result);
#elif defined(__APPLE__)
  unsigned long oldValue = *oldval;
  bool swapped = OSAtomicCompareAndSwapLongBarrier (oldValue, newval, (volatile long *) oldval);
  if (swapped) {
    return newval;
  } else {
    return oldValue;
  }
#else
#error "No spin lock implementation is available for this platform."
#endif
  return newval;
}


#endif

#endif // _SPINLOCK_H_
