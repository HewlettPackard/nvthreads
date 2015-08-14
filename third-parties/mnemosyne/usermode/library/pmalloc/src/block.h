///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// The Hoard Multiprocessor Memory Allocator
// www.hoard.org
//
// Author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2001, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "config.h"

#include <assert.h>


class superblock;

class block {
public:

  block (superblock * sb)
    : 
#if HEAP_DEBUG
      _magic (FREE_BLOCK_MAGIC),
#endif
      _next (NULL),
      _mySuperblock (sb)
    {}

  block& operator= (const block& b) {
#if HEAP_DEBUG
    _magic = b._magic;
#endif
    _next = b._next;
    _mySuperblock = b._mySuperblock;
#if HEAP_FRAG_STATS
    _requestedSize = b._requestedSize;
#endif
    return *this;
  }

  enum { ALLOCATED_BLOCK_MAGIC = 0xcafecafe,
	 FREE_BLOCK_MAGIC = 0xbabebabe };

  // Mark this block as free.
  inline void markFree (void);

  // Mark this block as allocated.
  inline void markAllocated (void);

  // Is this block valid? (i.e.,
  // does it have the right magic number?)
  inline int isValid (void) const;

  // Return the block's superblock pointer.
  inline superblock * getSuperblock (void);

#if HEAP_FRAG_STATS
  void setRequestedSize (size_t s) 
  {   
    _requestedSize = s;
  }

  size_t getRequestedSize (void) { return _requestedSize; }
#endif

  void setNext (block * b) { _next = b; }
  block * getNext (void) { return _next; }

  void setId(int id) { _id = id; }
  int getId(void) { return _id; }

private:

#if HEAP_DEBUG
  union {
    unsigned long _magic;
    double _d3; // For alignment.
  };
#endif

  block *	_next;		// The next block in a linked-list of blocks.
  superblock *	_mySuperblock;	// A pointer to my superblock.
  int _id; // My index identifier as known by the persistent superblock.

#if HEAP_FRAG_STATS
  union {
    double _d4; // This is just for alignment purposes.
    size_t _requestedSize;	// The amount of space requested (vs. allocated).
  };
#endif

  // Disable copying.

  block (const block&);
};


superblock * block::getSuperblock (void)
{
#if HEAP_DEBUG
  assert (isValid());
#endif
  return _mySuperblock;
}


void block::markFree (void)
{
#if HEAP_DEBUG
  assert (_magic == ALLOCATED_BLOCK_MAGIC);
  _magic = FREE_BLOCK_MAGIC;
#endif
}


void block::markAllocated (void)
{
#if HEAP_DEBUG
  assert (_magic == FREE_BLOCK_MAGIC);
  _magic = ALLOCATED_BLOCK_MAGIC;
#endif
}


int block::isValid (void) const 
{
#if HEAP_DEBUG
  return ((_magic == FREE_BLOCK_MAGIC)
	  || (_magic == ALLOCATED_BLOCK_MAGIC));
#else
  return 1;
#endif
}

#endif // _BLOCK_H_
