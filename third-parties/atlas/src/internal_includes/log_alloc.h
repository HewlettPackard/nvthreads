#ifndef _LOG_ALLOC_H
#define _LOG_ALLOC_H

#include "log_structure.h"

#define DEFAULT_CB_SIZE (1024*16-1)
//#define DEFAULT_CB_SIZE (256-1)

template<class T>
struct CbLog
{
    uint32_t Size_;
    volatile uint32_t isFilled_;
    volatile AO_t Start_;
    volatile AO_t End_;
    T *LogArray_;
};

template<class T>
struct CbListNode
{
    CbLog<T> *Cb_;
    char *StartAddr_;
    char *EndAddr_;
    CbListNode<T> *Next_;
    pthread_t Tid_;
    volatile uint32_t isAvailable_;
};

// CbList is a data structure shared among threads. When a new slot is
// requested, if the current buffer is found full, the current thread
// should acquire a lock, create a new buffer, add it to the CbList and
// return the first slot from this new buffer. If a buffer ever becomes
// empty, it can be reused. A partially empty buffer cannot be reused.

template<class T> CbLog<T> *GetNewCb(
    uint32_t size, uint32_t rid, CbLog<T> **log_p,
    CbListNode<T> * volatile *cb_list_p);
template<class T> T *GetNewSlot(uint32_t rid, CbLog<T> **log_p,
                                CbListNode<T> * volatile *cb_list_p);
template<class T> void DeleteSlot(CbLog<T> *cb, T *addr);
template<class T>
void DeleteEntry(CbListNode<T> * volatile cb_list_p, T *addr);

// TODO eventual GC on cb_list

template<class T>
inline bool isCbFull(CbLog<T> *cb)
{ return (ALAR(cb->End_)+1) % cb->Size_ == ALAR(cb->Start_); }

template<class T>
inline bool isCbEmpty(CbLog<T> *cb)
{ return ALAR(cb->Start_) == ALAR(cb->End_); }

#endif
