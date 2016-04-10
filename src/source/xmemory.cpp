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

#include "xmemory.h"

/// The globals region.
xglobals xmemory::_globals;

/// The protected heap used to satisfy small objects requirement. Less than 256 bytes now.
warpheap<xdefines::NUM_HEAPS, xdefines::PROTECTEDHEAP_CHUNK, xoneheap<xheap<xdefines::PROTECTEDHEAP_SIZE> > > xmemory::_pheap;

#ifdef IMPROVE_PERF
warpheap<xdefines::NUM_HEAPS, xdefines::SHAREDHEAP_CHUNK, xoneheap<SourceShareHeap<xdefines::SHAREDHEAP_SIZE> > > xmemory::_sheap;
#endif

/// A signal stack, for catching signals.
stack_t xmemory::_sigstk;

int xmemory::_heapid;
MemoryLog *xmemory::_localMemoryLog;
nvrecovery *xmemory::_localNvRecovery;
