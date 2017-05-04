/*
(c) Copyright [2017] Hewlett Packard Enterprise Development LP

This program is free software; you can redistribute it and/or modify it under 
the terms of the GNU General Public License, version 2 as published by the 
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
Place, Suite 330, Boston, MA 02111-1307 USA

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

#ifndef _XONEHEAP_H_
#define _XONEHEAP_H_

/**
 * @class xoneheap
 * @brief Wraps a single heap instance.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

template<class SourceHeap>
class xoneheap {
public:
    xoneheap() {
    }

    void initialize(void) {
        getHeap()->initialize();
    }
    void finalize(void) {
        getHeap()->finalize();
    }
    void begin(bool cleanup) {
        getHeap()->begin(cleanup);
    }

#ifdef LAZY_COMMIT
    void finalcommit(bool release) {
        getHeap()->finalcommit(release);
    }

    void forceCommit(int pid, void *end) {
        getHeap()->forceCommitOwnedPages(pid, end);
    }
#endif

    void checkandcommit(bool update, MemoryLog *localMemoryLog) {
        getHeap()->checkandcommit(update, localMemoryLog);
    }

    void* getend(void) {
        return getHeap()->getend();
    }

    void stats(void) {
        getHeap()->stats();
    }

    void openProtection(void *end, MemoryLog *localMemoryLog) {
        getHeap()->openProtection(end, localMemoryLog);
    }
    void closeProtection(void) {
        getHeap()->closeProtection();
    }

    bool nop(void) {
        return getHeap()->nop();
    }

    bool inRange(void *ptr) {
        return getHeap()->inRange(ptr);
    }
    void handleWrite(void *ptr) {
        getHeap()->handleWrite(ptr);
    }

#ifdef LAZY_COMMIT
    void cleanupOwnedBlocks(void) {
        getHeap()->cleanupOwnedBlocks();
    }

    void commitOwnedPage(int page_no, bool set_shared) {
        getHeap()->commitOwnedPage(page_no, set_shared);
    }
#endif

    bool mem_write(void *dest, void *val) {
        return getHeap()->mem_write(dest, val);
    }

    void setThreadIndex(int index) {
        return getHeap()->setThreadIndex(index);
    }
    
    void createLookupInfo(void){
        getHeap()->createLookupInfo();
    }

    void createDependenceInfo(void){
        getHeap()->createDependenceInfo();
    }

    void commitCacheBuffer(void){
        getHeap()->commitCacheBuffer();
    }

    void cleanupDependence(void){
        getHeap()->cleanupDependence();
    }
    size_t computePageNo(void* addr){
        return getHeap()->computePageNo(addr);
    }
    
    int computePageOffset(void* addr){
        return getHeap()->computePageOffset(addr);
    }

    void setLogPath(char *path){
        getHeap()->setLogPath(path);
    }

    void* malloc(size_t sz) {
        return getHeap()->malloc(sz);
    }
    void free(void *ptr) {
        getHeap()->free(ptr);
    }
    size_t getSize(void *ptr) {
        return getHeap()->getSize(ptr);
    }
private:

    SourceHeap* getHeap(void) {
        static char heapbuf[sizeof(SourceHeap)];
        static SourceHeap *_heap = new(heapbuf) SourceHeap;
        return _heap;
    }
};

#endif // _XONEHEAP_H_
