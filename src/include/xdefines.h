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

// -*- C++ -*-

/*
 Author: Emery Berger, http://www.cs.umass.edu/~emery
 
 Copyright (c) 2007-8 Emery Berger, University of Massachusetts Amherst.

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
 * @file xdefines.h
 * @brief Some definitions which maybe modified in the future.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie>
 */

#ifndef _XDEFINES_H_
#define _XDEFINES_H_
#include <sys/types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "prof.h"

#define METACOUNTER(x) volatile uint64_t x##_count;
#define INC_METACOUNTER(x) global_metadata->metadata.x##_count++
#define DEC_METACOUNTER(x) global_metadata->metadata.x##_count--
#define GET_METACOUNTER(x) global_metadata->metadata.x##_count
#define SET_METACOUNTER(x, y) global_metadata->metadata.x##_count=y

typedef struct runtime_data {
  volatile unsigned long thread_index;
  struct runtime_stats stats;
} runtime_data_t;

extern runtime_data_t *global_data;

struct metadata_t {
    METACOUNTER(globalTransactionCount);
    METACOUNTER(globalThreadCount);
    METACOUNTER(globalLockCountMax);
    METACOUNTER(globalLockCountMaxHolder);
    METACOUNTER(globalBufferedGlobalPageNoCount);
    METACOUNTER(globalBufferedHeapPageNoCount);
};

typedef struct runtime_metadata {
    volatile unsigned long thread_index;
    struct metadata_t metadata;
} runtime_metadata_t;
extern runtime_metadata_t *global_metadata;       

class xdefines {
public:
  enum { STACK_SIZE = 1024 * 1024 } ; // 1 * 1048576 };
  //enum { PROTECTEDHEAP_SIZE = 1048576UL * 2048}; // FIX ME 512 };
#ifdef X86_32BIT
//enum { PROTECTEDHEAP_SIZE = 1048576UL * 1024 * 2 }; // FIX ME 512 };
  enum { PROTECTEDHEAP_SIZE = 1048576UL * 1024 * 1 }; // FIX ME 512 };
#else
//enum { PROTECTEDHEAP_SIZE = 1048576UL * 4096 * 1 }; // FIX ME 512 };
  enum { PROTECTEDHEAP_SIZE = 1048576UL * 4096 * 2 };
#endif
  enum { PROTECTEDHEAP_CHUNK = 10485760 };
  
  enum { MAX_GLOBALS_SIZE = 1048576UL * 40 };
  enum { INTERNALHEAP_SIZE = 1048576UL * 100 }; // FIXME 10M
//enum { INTERNALHEAP_SIZE = 1048576UL * 1000 }; // FIXME 10M
  enum { PageSize = 4096UL };
  enum { PAGE_SIZE_MASK = (PageSize-1) };
  enum { NUM_HEAPS = 32 }; // was 16
  enum { LOCK_OWNER_BUDGET = 10 };
};

#endif
