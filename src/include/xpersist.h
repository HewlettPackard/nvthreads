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

#ifndef DTHREADS_XPERSIST_H
#define DTHREADS_XPERSIST_H

/*
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
 * @file   xpersist.h
 * @brief  Main file to handle page fault, commits.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie>
 */

#include <set>
#include <list>
#include <vector>
#include <map>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

#include "xatomic.h"
#include "heaplayers/ansiwrapper.h"
#include "heaplayers/freelistheap.h"

#include "heaplayers/stlallocator.h"
#include "privateheap.h"
#include "xdefines.h"
#include "xbitmap.h"

#include "debug.h"

#include "xpageentry.h"

#include "logger.h"

#if defined(sun)
extern "C"
int madvise(caddr_t addr, size_t len, int advice);
#endif

#include "nvrecovery.h"

/**
 * @class xpersist
 * @brief Makes a range of memory persistent and consistent.
 */
template<class Type, unsigned long NElts = 1>
class xpersist {
 public:

  enum {SHARED_PAGE = 0xFFFFFFFF };

  enum page_access_info {
    PAGE_ACCESS_NONE = 0,
    PAGE_ACCESS_READ = 1,
    PAGE_ACCESS_WRITE = 2
  };

  /// @arg startaddr  the optional starting address of the local memory.
  xpersist(void* startaddr = 0, size_t startsize = 0)
      : _startaddr(startaddr),
        _startsize(startsize) {

    // Check predefined globals size is large enough or not.
    if (_startsize > 0) {
      if (_startsize > size()) {
        fprintf(stderr, "This persistent region size(): (%zu), _startsize (%zu).\n", size(), _startsize);
        fprintf(stderr, "This persistent region (%ld) is too small (%ld).\n", size(), _startsize);
        ::abort();
      }
    }

    // Get a temporary file name (which had better not be NFS-mounted...).
    char _backingFname[FILENAME_MAX];
    sprintf(_backingFname, "/tmp/nvthreadsMXXXXXX");
    _backingFd = mkstemp(_backingFname);
    if (_backingFd == -1) {
      fprintf(stderr, "Failed to make persistent file.\n");
      perror("mkstemp(): ");
      ::abort();
    }
    DEBUG("_backingFd: %d, %s\n", _backingFd, _backingFname);

    // Set the files to the sizes of the desired object.
    if (ftruncate(_backingFd, size())) {
      fprintf(stderr, "file: %s, fd: %d, size(): %zu\n", _backingFname, _backingFd, size());
      fprintf(stderr, "Mysterious error with ftruncate.NElts %ld\n", NElts);
      perror("ftruncate(): ");
      ::abort();
    }

    // Get rid of the files when we exit.
    unlink(_backingFname);

    char _versionsFname[FILENAME_MAX];
    // Get another temporary file name (which had better not be NFS-mounted...).
    sprintf(_versionsFname, "/tmp/nvthreadsVXXXXXX");
    _versionsFd = mkstemp(_versionsFname);
    if (_versionsFd == -1) {
      fprintf(stderr, "Failed to make persistent file.\n");
      ::abort();
    }

    if (ftruncate(_versionsFd, TotalPageNums * sizeof(unsigned long))) {
      // Some sort of mysterious error.
      // Adios.
      fprintf(stderr, "Mysterious error with ftruncate. TotalPageNums %d\n", TotalPageNums);
      ::abort();
    }

    unlink(_versionsFname);

    //
    // Establish two maps to the backing file.
    // The persistent map (shared maping) is shared.
    _persistentMemory = (Type*)mmap(NULL, size(), PROT_READ
                                    | PROT_WRITE, MAP_SHARED, _backingFd, 0);

    if (_persistentMemory == MAP_FAILED) {
      fprintf(stderr, "arguments: start= %p, length=%ld\n",
              (void*)NULL, size());
      perror("Persistent memory creation:");
      ::abort();
    }

    // If we specified a start address (globals), copy the contents into the
    // persistent area now because the transient memory map is going
    // to squash it.
    if (_startaddr) {
      memcpy(_persistentMemory, _startaddr, _startsize);
      _isHeap = false;
    } else {
      _isHeap = true;
    }

    // The transient map is private and optionally fixed at the desired start address for globals.
    // In order to get the same copy with _persistentMemory for those constructor stuff,
    // we will set to MAP_PRIVATE at first, then memory protection will be opened in initialize().
    _transientMemory = (Type*)mmap(_startaddr, size(),
                                   PROT_READ | PROT_WRITE, MAP_SHARED | (_startaddr != NULL ? MAP_FIXED : 0),
                                   _backingFd, 0);

    if (_transientMemory == MAP_FAILED) {
      fprintf(stderr, "arguments = %p, %ld, %d, %d, %d\n",
              _startaddr, size(), PROT_READ | PROT_WRITE, MAP_SHARED | (_startaddr != NULL ? MAP_FIXED : 0), _backingFd);
      perror("Transient memory creation:");
      ::abort();
    }

    _isProtected = false;

    DEBUG("xpersist intialize: transient = %p, persistent = %p, size = %x", _transientMemory, _persistentMemory, size());

    // We are trying to use page's version number to speedup the commit phase.
    _persistentVersions = (volatile unsigned long*)mmap(NULL,
                                                        TotalPageNums * sizeof(unsigned long), PROT_READ | PROT_WRITE,
                                                        MAP_SHARED, _versionsFd, 0);

    _pageUsers = (struct shareinfo*)mmap(NULL, TotalPageNums * sizeof(struct shareinfo),
                                         PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

#ifdef LAZY_COMMIT
    _pageOwner = (volatile unsigned long*)mmap(NULL, TotalPageNums * sizeof(size_t),
                                               PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // Local
    _pageInfo = (unsigned long*)mmap(NULL, TotalPageNums * sizeof(size_t),
                                     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    _ownedblockinfo = (unsigned long*)mmap(NULL, xdefines::PageSize,
                                           PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (_pageOwner == MAP_FAILED || _pageInfo == MAP_FAILED) {
      fprintf(stderr, "xpersist: mmap about ownership tracking error with %s\n", strerror(errno));
      // If we couldn't map it, something has seriously gone wrong. Bail.
      ::abort();
    }

#endif
    if (_transientMemory == MAP_FAILED ||
        _persistentVersions == MAP_FAILED ||
        _pageUsers == MAP_FAILED ||
        _persistentMemory == MAP_FAILED ||
        _pageLookup == MAP_FAILED) {
      fprintf(stderr, "xpersist: mmap error with %s\n", strerror(errno));
      // If we couldn't map it, something has seriously gone wrong. Bail.
      ::abort();
    }

#ifdef GET_CHARACTERISTICS
    _pageChanges = (struct pagechangeinfo*)mmap(NULL,
                                                TotalPageNums * sizeof(struct pagechangeinfo),
                                                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
#endif

    lprintf("initialized xpersist, dirtyListPages: %p, is_Heap: %d\n", &_dirtiedPagesList, _isHeap);
  }

  void initialize() {
    // A string of one bits.
    allones = _mm_setzero_si128();
    allones = _mm_cmpeq_epi32(allones, allones);

    // Clean the ownership.
    _dirtiedPagesList.clear();
  }

  void finalize() {
    if (_isProtected)
      closeProtection();

#ifdef GET_CHARACTERISTICS
    int pages = getSingleThreadPages();
    fprintf(stderr, "Totally there are %d single thread pages.\n", pages);
#endif

  }

#ifdef GET_CHARACTERISTICS
  int getSingleThreadPages() {
    int pages = 0;
    for (int i = 0; i < TotalPageNums; i++) {
      struct pagechangeinfo* page = (struct pagechangeinfo*)&_pageChanges[i];
      if (page->version > 1 && page->tid != 0xFFFF) {
        //    fprintf(stderr, "i %d with tid %d and version %d\n", i, page->tid, page->version);
        pages += page->version;
      }
    }

    return pages;
  }
#endif

  void* writeProtect(void* start, size_t size) {
    void* area;
    size_t offset = (intptr_t)start - (intptr_t)base();

    // Map to readonly private area.
    area = (Type*)mmap(start, size, PROT_READ, MAP_PRIVATE | MAP_FIXED,
                       _backingFd, offset);
    if (area == MAP_FAILED) {
      fprintf(stderr, "Weird, %d writeProtect failed with error %s!!!\n",
              getpid(), strerror(errno));
      fprintf(stderr, "start %p size %ld!!!\n", start, size);
      exit(-1);
    }
    return (area);
  }

  void* removeProtect(void* start, size_t size) {
    void* area;
    size_t offset = (intptr_t)start - (intptr_t)base();

    // Map to writable share area.
    area = mmap(start, size, PROT_READ | PROT_WRITE, MAP_SHARED
                | MAP_FIXED, _backingFd, offset);

    if (area == MAP_FAILED) {
      fprintf(stderr, "Weird, %d remove protect failed!!!\n", getpid());
      exit(-1);
    }
    return (area);
  }

  void openProtection(void* end, MemoryLog* localMemoryLog) {

#ifdef LAZY_COMMIT
    // We will set the used area as R/W, while those un-used area will be set to PROT_NONE.
    // For globals, all pages are set to SHARED in the beginning.
    // For heap, only those allocated pages are set to SHARED.
    // Those pages that haven't allocated are set to be PRIVATE at first.
    if (!_isHeap) {
      writeProtect(base(), size());
      for (int i = 0; i < TotalPageNums; i++) {
        _pageOwner[i] = SHARED_PAGE;
        _pageInfo[i] = PAGE_ACCESS_READ;
      }
    } else {
#ifdef X86_32BIT
      int allocSize = (intptr_t)end - (intptr_t)base();
#else
      unsigned long allocSize = (unsigned long)end - (unsigned long)base();
#endif
      // For those allocated pages, we set to READ_ONLY.
      if (allocSize > 0) {
        writeProtect(base(), allocSize);
      }

      for (int i = 0; i < allocSize / xdefines::PageSize; i++) {
        _pageOwner[i] = SHARED_PAGE;
        _pageInfo[i] = PAGE_ACCESS_READ;
      }

      // All un-used pages can't be read at the first time.
      if (mmap(end, size() - allocSize, PROT_NONE, MAP_PRIVATE | MAP_FIXED,
               _backingFd, allocSize) == MAP_FAILED) {
        fprintf(stderr, "Open protection failed for heap. \n");
        exit(-1);
      }

      // Those un-allocated pages can be owned.
      for (int i = allocSize / xdefines::PageSize; i < TotalPageNums; i++) {
        _pageOwner[i] = 0;
        _pageInfo[i] = PAGE_ACCESS_NONE;
      }
    }
    _isProtected = true;
    _ownedblocks = 0;
#else
    writeProtect(base(), size());
    _isProtected = true;
#endif


    // Log memory heap and globals if we're the parent and the only threads before moving.
    // This ensures all updates in the serial phases are logged.
    /*
    localMemoryLog->WriteLogBeforeProtection(base(), size(), _isHeap);
    if ( _isHeap ) {
        printf("Heap : base: %p, size: %zu, %lu bytes, %s\n", base(), size(), localMemoryLog->_mempages_filesize, localMemoryLog->_mempages_filename);
    } else {
        printf("Global : base: %p, size: %zu, %lu bytes, %s\n", base(), size(), localMemoryLog->_mempages_filesize, localMemoryLog->_mempages_filename);
    }
    */

    _trans = 0;
  }

  void closeProtection() {
    removeProtect(base(), size());
    _isProtected = false;
  }

  void setThreadIndex(int index) {
    _threadindex = index;
  }

  void setLogPath(char* path) {
    logPath = path;
  }

  // Create shared mapping file and return the corresponding file descriptor
  int createSharedMapFile(char* fn, size_t npages, size_t sz) {
    int fd = 0;
    lprintf("Creating shared mmap file at %s\n", fn);

    // Check if file exists.  If so, rename file to "recover" first
    if (access(fn, F_OK) != -1) {
      char _recoverFname[FILENAME_MAX];
      sprintf(_recoverFname, "%s_recover", fn);
      if (rename(fn, _recoverFname) != 0) {
        lprintf("error: unable to rename file\n");
      }
      lprintf("File %s exists, rename to %s\n", fn, _recoverFname);
    }

    // Create file for mmap
    fd = open(fn, O_RDWR | O_SYNC | O_CREAT, 0644);
    if (fd == -1) {
      fprintf(stderr, "Failed to make persistent file.\n");
      ::abort();
    }

    // Set the files to the sizes of the desired object.
    if (ftruncate(fd, npages * sz)) {
      fprintf(stderr, "Mysterious error with ftruncate.NElts %ld\n", NElts);
      ::abort();
    }

    return fd;
  }

  // Create a quick reference to lookup the buffered pageInfo when commit the tmp pageInfo
  void createPageNoTmp(void) {
    if (!logPath) {
      lprintf("logPath not ready yet %s\n", logPath);
      return;
    }
    if (_pageNoTmp) {
      lprintf("_pageNoTmp is already set at %p\n", _pageNoTmp);
      return;
    }
    lprintf("Creating page lookup info at %s\n", logPath);
    // Quick look up metadata for heap and global
    char _lookupTmpFname[FILENAME_MAX];
    if (_isHeap) {
      sprintf(_lookupTmpFname, "%slookup_heap_tmp", logPath);
      SET_METACOUNTER(globalBufferedHeapPageNoCount, 0);
    } else {
      sprintf(_lookupTmpFname, "%slookup_globals_tmp", logPath);
      SET_METACOUNTER(globalBufferedGlobalPageNoCount, 0);
    }

    _pageNoTmpFd = createSharedMapFile(_lookupTmpFname, TotalPageNums, sizeof(int));
    _pageNoTmp = (int*)mmap(NULL, TotalPageNums * sizeof(int),
                            PROT_READ | PROT_WRITE, MAP_SHARED, _pageNoTmpFd, 0);

    memset((void*)_pageNoTmp, 0, TotalPageNums * sizeof(int));
  }

  // Create _pageLookupTmp that is used to buffer page metadata for page lookup info
  void createLookupInfoTmp(void) {
    if (!logPath) {
      lprintf("logPath not ready yet %s\n", logPath);
      return;
    }
    if (_pageLookupTmp) {
      lprintf("_pageLookupTmp is already set at %p\n", _pageLookupTmp);
      return;
    }
    lprintf("Creating tmp lookup info at %s\n", logPath);

    // Quick look up metadata for heap and global
    char _lookupTmpFname[FILENAME_MAX];
    if (_isHeap) {
      sprintf(_lookupTmpFname, "%slookup_heap_tmp", logPath);
    } else {
      sprintf(_lookupTmpFname, "%slookup_globals_tmp", logPath);
    }

    // Create shared region to store temporary page lookup metadata
    _lookupTmpFd = createSharedMapFile(_lookupTmpFname, TotalPageNums, sizeof(struct lookupinfo));
    _pageLookupTmp = (struct lookupinfo*)mmap(NULL, TotalPageNums * sizeof(struct lookupinfo),
                                              PROT_READ | PROT_WRITE, MAP_SHARED, _lookupTmpFd, 0);
    memset((void*)_pageLookupTmp, 0, TotalPageNums * sizeof(struct lookupinfo));
    lprintf("lookup file: %s, base(): %p, isHeap %d\n", _lookupTmpFname, base(), _isHeap);
  }

  // Create _pageLookup that is used to store page metadata for crash recovery
  void createLookupInfo(void) {
    if (!logPath) {
      lprintf("logPath not ready yet %s\n", logPath);
      return;
    }
    if (_pageLookup) {
      lprintf("_pageLookup is already set at %p\n", _pageLookup);
      return;
    }
    lprintf("Creating lookup info at %s\n", logPath);
    // Quick look up metadata for heap and global
    char _lookupFname[FILENAME_MAX];
    if (_isHeap) {
      sprintf(_lookupFname, "%slookup_heap", logPath);
    } else {
      sprintf(_lookupFname, "%slookup_globals", logPath);
    }

    // Create shared region to store page lookup metadata
    _lookupFd = createSharedMapFile(_lookupFname, TotalPageNums, sizeof(struct lookupinfo));
    _pageLookup = (struct lookupinfo*)mmap(NULL, TotalPageNums * sizeof(struct lookupinfo),
                                           PROT_READ | PROT_WRITE, MAP_SHARED, _lookupFd, 0);
    memset((void*)_pageLookup, 0, TotalPageNums * sizeof(struct lookupinfo));
    lprintf("lookup file: %s, base(): %p, isHeap %d\n", _lookupFname, base(), _isHeap);

    // Create a tmp lookup info to buffer unready pageInfo in a nested section
    createLookupInfoTmp();

    // Create a quick reference to lookup the buffered pageInfo when commit the tmp pageInfo
    createPageNoTmp();
  }

  // Create page dependence info to store which thread last touched a page
  void createDependenceInfo(void) {
    if (!logPath) {
      lprintf("logPath not ready yet %s\n", logPath);
      return;
    }
    if (_pageDependence) {
      lprintf("_pageDependence is already set at %p\n", _pageDependence);
      return;
    }
    lprintf("Creating dependence info at %s\n", logPath);

    // Quick look up metadata for heap and global
    char _pageDependenceFname[FILENAME_MAX];
    if (_isHeap) {
      sprintf(_pageDependenceFname, "%sdependence_heap", logPath);
    } else {
      sprintf(_pageDependenceFname, "%sdependence_globals", logPath);
    }
    _pageDependenceFd = createSharedMapFile(_pageDependenceFname, TotalPageNums, sizeof(struct pageDependence));

    // _pageDependece records which thread last touched a page
    _pageDependence = (struct pageDependence*)mmap(NULL, TotalPageNums * sizeof(struct pageDependence),
                                                   PROT_READ | PROT_WRITE, MAP_SHARED, _pageDependenceFd, 0);
    memset((void*)_pageDependence, 0, TotalPageNums * sizeof(struct pageDependence));
    lprintf("page dependence file: %s, base(): %p, isHeap %d\n", _pageDependenceFname, base(), _isHeap);
    _pageDependenceSize = TotalPageNums * sizeof(struct pageDependence);

  }

  // Copy pageInfo from tmp cache buffer to the real pageInfo used by the recovery code
  void commitCacheBuffer(void) {
    unsigned long i;
    unsigned long num_buffered_pages;
    // Heap and global have their own instances of xpersist class, 
    // i.e., they use different instances of _pageNoTmp, _pageLookup
    if (_isHeap) {  
      num_buffered_pages = GET_METACOUNTER(globalBufferedHeapPageNoCount);
      lprintf("commit %zu pages to pageInfo (heap)\n", num_buffered_pages);
    } else {
      num_buffered_pages = GET_METACOUNTER(globalBufferedGlobalPageNoCount);
      lprintf("commit %zu pages to pageInfo (global)\n", num_buffered_pages);
    }

    // Clean pageDependence metadata
    memset((void*)_pageDependence, 0, TotalPageNums * sizeof(struct pageDependence));
    lprintf("clean up pageDependence, _isHeap: %d\n", _isHeap);

    if (num_buffered_pages == 0) {
      lprintf("No buffered pages to commit, return\n");
      return;
    }

    // Commit all buffered page lookup info from _pageNoTmp to _pageLookup
    // _pageNoTmp records the pageNo of the buffered page, there are a total of num_buffer_pages 
    // buffered pages; _pageNoTmp[] gives the pageNo of the buffered page; _pageLookup is indexed 
    // by page number, So _pageLookupTmp[_pageNoTmp[]] gives you the buffered page matadata
    for (i = 0; i < num_buffered_pages; i++) {
      // Copy pageInfo from tmp to the actual pageInfo
      memcpy(&_pageLookup[_pageNoTmp[i]], &_pageLookupTmp[_pageNoTmp[i]], sizeof(struct lookupinfo));
      lprintf("committed _pageLookup[%d]\n", _pageNoTmp[i]);
      // Clean used tmp pageInfo
      memset(&_pageLookupTmp[_pageNoTmp[i]], 0, sizeof(struct lookupinfo));
      // Clean pageNoTmp
      _pageNoTmp[i] = 0;
    }

    if (_isHeap) {
      SET_METACOUNTER(globalBufferedHeapPageNoCount, 0);
    } else {
      SET_METACOUNTER(globalBufferedGlobalPageNoCount, 0);
    }
  }

  /// @return true iff the address is in this space.
  inline bool inRange(void* addr) {
    if (((size_t)addr >= (size_t)base()) && ((size_t)addr
                                             < (size_t)base() + size())) {
      return true;
    } else {
      return false;
    }
  }

  /// @return the start of the memory region being managed.
  inline Type* base() const {
    return _transientMemory;
  }

  bool mem_write(void* addr, void* val) {
    unsigned long offset = (intptr_t)addr - (intptr_t)base();
    void** ptr = (void**)((intptr_t)_persistentMemory + offset);
    *ptr = val;
    return true;
  }

  /// @return the size in bytes of the underlying object.
  static inline size_t size() {
    return NElts * sizeof(Type);
  }

#ifdef LAZY_COMMIT
  // Change the page to read-only mode.
  inline void mprotectRead(void* addr, int pageNo) {
    _pageInfo[pageNo] = PAGE_ACCESS_READ;
    mprotect(addr, xdefines::PageSize, PROT_READ);
  }
#endif

  // Change the page to writable mode.
  inline void mprotectWrite(void* addr, int pageNo) {
    int rv;
#ifdef LAZY_COMMIT
    if (_pageOwner[pageNo] == getpid()) {
      _pageInfo[pageNo] = PAGE_ACCESS_WRITE;
    }
#endif
    rv = mprotect(addr, xdefines::PageSize, PROT_READ | PROT_WRITE);
    if (rv != 0) {
      fprintf(stderr, "mprotect(%p, %d, PROT_READ|PROT_WRITE) failed...\n", addr,  xdefines::PageSize);
      perror("mprotect() in mprotectWrite");
      exit(-1);
    }
  }

#ifdef LAZY_COMMIT
  inline bool isSharedPage(int pageNo) {
    return (_pageOwner[pageNo] == SHARED_PAGE);
  }

  // Those owned page will also set to MAP_PRIVATE and READ_ONLY
  // in the beginning. The difference is that they don't need to commit
  // immediately in order to reduce the time of serial phases.
  // The function will be called when one thread is getting a new superblock.
  inline void setOwnedPage(void* addr, size_t size) {
    if (!_isProtected)
      return;

    int pid = getpid();
    size_t startPage = computePage((intptr_t)addr - (intptr_t)base());
    size_t pages = size / xdefines::PageSize;
    char* pageStart = (char*)addr;
    int blocks = _ownedblocks;

    if (mprotect(addr, size, PROT_READ)) {
      perror("mprotect: PROT_READ");
      exit(-1);
    }

    for (int i = startPage; i < startPage + pages; i++) {
      _pageOwner[i] = pid;
      _pageInfo[i] = PAGE_ACCESS_READ;
    }

    // This block are now owned by current thread.
    // In the end, all pages in this block are going to be checked.
    _ownedblockinfo[blocks * 2] = startPage;
    _ownedblockinfo[blocks * 2 + 1] = startPage + pages;
    _ownedblocks++;
    if (_ownedblocks == xdefines::PageSize / 2) {
      fprintf(stderr, "Not enought to hold super blocks.\n");
      exit(-1);
    }
  }
#endif

  // @ Page fault handler
  void handleWrite(void* addr) {
    // Compute the page number of this item
    int pageNo = computePage((size_t)addr - (size_t)base());
#ifdef X86_32BIT
    unsigned long* pageStart = (unsigned long*)((intptr_t)_transientMemory + xdefines::PageSize * pageNo);
#else
    unsigned long* pageStart = (unsigned long*)((intptr_t)_transientMemory + (unsigned long)xdefines::PageSize * pageNo);
#endif
    struct xpageinfo* curr = NULL;

#ifdef LAZY_COMMIT
    // Check the access type of this page.
    int accessType = _pageInfo[pageNo];

    // When we are trying to access other-owned page.
    if (accessType == PAGE_ACCESS_NONE) {
      // Current page must be owned by other pages.
      notifyOwnerToCommit(pageNo);

      // Now we set the page to readable.
      mprotectRead(pageStart, pageNo);

      // Add this page to read set??
      // Since this page is already set to SHARED, there is no need for any further
      // operations now.
      return;
    } else if (accessType == PAGE_ACCESS_READ) {
      // There are two cases here.
      // (1) I have read other's owned page, now I am writing on it.
      // (2) I tries to write to my owned page in the first, but without previous version.
      mprotectWrite(pageStart, pageNo);
      // Now page is writable, add this page to dirty set.
    } else if (accessType == PAGE_ACCESS_WRITE) {
      // One case, I am trying to write to those dirty pages again.
      mprotectWrite(pageStart, pageNo);
      // Since we are already wrote to this page before, now we are trying to write to
      // this page again. Now we should commit old version to the shared copy.
      commitOwnedPage(pageNo, false);
    }
#else
    mprotectWrite(pageStart, pageNo);
#endif

    // Now one more user are using this page.
    xatomic::increment((unsigned long*)&_pageUsers[pageNo]);

    // Add this page to the dirty set.
    curr = xpageentry::getInstance().alloc();
    curr->pageNo = pageNo;
    curr->pageStart = (void*)pageStart;
    curr->isUpdated = 0;
    curr->isLogged = 0;
    curr->diriedBy = getpid();
#ifndef LAZY_COMMIT
    curr->release = true;
#endif
    curr->version = _persistentVersions[pageNo];

    INC_COUNTER(faults);
    INC_COUNTER(dirtypage_inserted);

    // Then add current page to dirty list.
    _dirtiedPagesList.insert(std::pair<int, void*>(pageNo, curr));
    return;
  }


  bool nop() {
    return (_dirtiedPagesList.empty());
  }

  /// @brief Commit dirtied pages before open protection
  inline void commitBeforeOpenProtection(MemoryLog* localMemoryLog) {
    checkandcommit(true, localMemoryLog);
  }

  /// @brief Start a transaction.
  inline void begin(bool cleanup) {
    // Update all pages related in this dirty page list
    updateAll(cleanup);
  }

#ifndef SSE_SUPPORT
  inline void commitWord(char* src, char* twin, char* dest) {
    for (int i = 0; i < sizeof(long long); i++) {
      if (src[i] != twin[i]) {
        TRACE("%d: Write %d-th byte.  src: %p, twin: %p, dest: %p\n", getpid(), i, src[i], twin[i], dest[i]);
        dest[i] = src[i];
      }
    }
  }
#endif
  // Write those difference between local and twin to the destination.
  inline void writePageDiffs(const void* local, const void* twin,
                             void* dest, int pageno) {

#ifdef SSE_SUPPORT
    // Now we are using the SSE3 instructions to speedup the commits.
    __m128i* localbuf = (__m128i*)local;
    __m128i* twinbuf = (__m128i*)twin;
    __m128i* destbuf = (__m128i*)dest;
    // Some vectorizing pragamata here; not sure if gcc implements them.
#pragma vector always
    for (int i = 0; i < xdefines::PageSize / sizeof(__m128i); i++) {

      __m128i localChunk, twinChunk, destChunk;

      localChunk = _mm_load_si128(&localbuf[i]);
      twinChunk = _mm_load_si128(&twinbuf[i]);

      // Compare the local and twin byte-wise.
      __m128i eqChunk = _mm_cmpeq_epi8(localChunk, twinChunk);

      // Invert the bits by XORing them with ones.
      __m128i neqChunk = _mm_xor_si128(allones, eqChunk);

      // Write local pieces into destbuf everywhere diffs.
      _mm_maskmoveu_si128(localChunk, neqChunk, (char*)&destbuf[i]);
    }
#else
    /* If hardware can't support SSE3 instructions, use slow commits as following. */
    long long* mylocal = (long long*)local;
    long long* mytwin = (long long*)twin;
    long long* mydest = (long long*)dest;

    for (int i = 0; i < xdefines::PageSize / sizeof(long long); i++) {
      if (mylocal[i] != mytwin[i]) {
        commitWord((char*)&mylocal[i], (char*)&mytwin[i], (char*)&mydest[i]);
      }
    }


#endif
  }

  /* For page density profiling */
#ifdef PAGE_DENSITY
  void ProfileDiffs(const void* local, const void* twin) {
    long long* mylocal = (long long*)local;
    long long* mytwin = (long long*)twin;
    int count = 0;

    // Count diff in bytes
    for (int i = 0; i < xdefines::PageSize; i++) {
      if (mylocal[i] != mytwin[i]) {
        count++;
      }
    }

    // Increment frequency for page with "count" dirty bytes
    INC_COUNTER_ARRAY(pagedensity, count);
    INC_COUNTER(pdcount);
  }

  void PrintPageDensity(void) {
    lprintf("%d:-------page density-------\n", getpid());
    for (int i = 0; i < xdefines::PageSize; i++) {
      lprintf("%d: PageDensity[%d]: %ld\n", getpid(), i, page_density[i]);
    }
    PRINT_COUNTER_ARRAY(pagedensity, 4097UL);
  }
#endif

  // Create the twin page for the page with specified pageNo.
  void createTwinPage(int pageNo) {
    int index;
    unsigned long* twin;
    struct shareinfo* shareinfo = NULL;

    // We will use a multiprocess-shared array(_pageUsers) to save twin page information.
    shareinfo = &_pageUsers[pageNo];

    index = xbitmap::getInstance().get();
    // We can never get bitmap index with 0.
    assert(index != 0);

    shareinfo->bitmapIndex = index;

    //Create the "shared-twin-page" for them
    twin = (unsigned long*)xbitmap::getInstance().getAddress(index);
    memcpy(twin, (void*)((intptr_t)_persistentMemory + xdefines::PageSize * pageNo), xdefines::PageSize);

    INC_COUNTER(twinpage);

    // Set the twin page version number.
    xbitmap::getInstance().setVersion(index, _persistentVersions[pageNo]);
  }

#ifdef GET_CHARACTERISTICS
  inline void recordPageChanges(int pageNo) {
    struct pagechangeinfo* page = (struct pagechangeinfo*)&_pageChanges[pageNo];
    unsigned short tid = page->tid;

    int mine = getpid();

    // If this word is not shared, we should set to current thread.
    if (tid == 0) {
      page->tid = mine;
    } else if (tid != 0 && tid != mine && tid != 0xFFFF) {
      // This word is shared by different threads. Set to 0xFFFF.
      page->tid = 0xFFFF;
    }

    page->version++;
  }
#else
  inline void recordPageChanges(int pageNo) { }
#endif

#ifdef LAZY_COMMIT
  // This function will force the process with specified pid to commit all
  // owned-by-it pages.
  // This happens when one thread are trying to call pthread_kill or
  // pthread_cancel to kill one thread.
  inline void forceCommitOwnedPages(int pid, void* end) {
    size_t startpage = 0;
    size_t endpage = ((intptr_t)end - (intptr_t)base()) / xdefines::PageSize;
    int i;

    // Check all possible pages.
    for (i = startpage; i < endpage; i++) {
      int accessType = _pageInfo[i];

      // When one page is owned by specified thread,
      if (_pageOwner[i] == pid) {
        notifyOwnerToCommit(i);
      }
    }

    return;
  }

  inline void notifyOwnerToCommit(int pageNo) {
    // Get the owner information.
    unsigned int owner = _pageOwner[pageNo];

    if (owner == SHARED_PAGE) {
      // owner has committed page, we can exit now.
      return;
    }

    // Otherwise, we should send a signal to the owner.
    union sigval val;
    val.sival_int = pageNo;
    int i;

   notify_owner:
    i = 0;
    if (sigqueue(owner, SIGUSR1, val) != 0) {
      setSharedPage(pageNo);
      return;
    }

    // Spin here until the page is set to be SHARED; Ad hoc synchronization.
    while (!isSharedPage(pageNo)) {
      i++;
      if (i == 100000)
        goto notify_owner;
    }
  }

  inline void setSharedPage(int pageNo) {
    if (_pageOwner[pageNo] != SHARED_PAGE) {
      xatomic::exchange(&_pageOwner[pageNo], SHARED_PAGE);
      _pageInfo[pageNo] = PAGE_ACCESS_READ;
    }
  }

  inline void cleanupOwnedBlocks() {
    _ownedblocks = 0;
  }

  inline void commitOwnedPage(int pageNo, bool setShared) {
    // Check this page's attribute.
    unsigned int owner;

    // Get corresponding entry.
    void* addr = (void*)((intptr_t)base() + pageNo * xdefines::PageSize);
    void* share = (void*)((intptr_t)_persistentMemory + xdefines::PageSize * pageNo);
#ifdef GET_CHARACTERISTICS
    recordPageChanges(pageNo);
#endif
    INC_COUNTER(dirtypage_owned);
    INC_COUNTER(lazypage);
    // Commit its previous version.
    memcpy(share, addr, xdefines::PageSize);
    if (setShared) {
      // Finally, we should set this page to SHARED state.
      setSharedPage(pageNo);

      // We also release the private copy when one page is already shared.
      madvise(addr, xdefines::PageSize, MADV_DONTNEED);
    }

    // Update the version number.
    _persistentVersions[pageNo]++;
  }

  // Commit all pages when the thread is going to exit
  inline void finalcommit(bool release) {
    int blocks = _ownedblocks;
    int startpage;
    int endpage;
    int j;

    // Only checked owned blocks.
    for (int i = 0; i < blocks; i++) {
      startpage = _ownedblockinfo[i * 2];
      endpage = _ownedblockinfo[i * 2 + 1];

      if (release) {
        for (j = startpage; j < endpage; j++) {
          int accessType = _pageInfo[j];
          if (_pageOwner[j] == getpid()) {
            commitOwnedPage(j, true);
          }
        }
      } else {
        // We do not release the private copy when one thread is exit in order to improve the performance.
        for (j = startpage; j < endpage; j++) {
          int accessType = _pageInfo[j];
          if (_pageOwner[j] == getpid()) {
            commitOwnedPage(j, false);
            setSharedPage(j);
          }
        }
      }
    }
  }
#endif

  // Get the start address of specified page.
  inline void* getPageStart(int pageNo) {
    return ((void*)((intptr_t)base() + pageNo * xdefines::PageSize));
  }

  // Print all the dirty pages so far to stdout
  void dumpDirtiedPages(void) {
    struct xpageinfo* pageinfo = NULL;
    int pageNo;
    int cnt = 0;
    printf("-----------%d dump dirtied pages-------------\n", getpid());
    for (dirtyListType::iterator i = _dirtiedPagesList.begin(); i != _dirtiedPagesList.end(); ++i) {
      pageinfo = (struct xpageinfo*)i->second;
      pageNo = pageinfo->pageNo;
      printf("%d: pid %d, page %d with addr %p dirtied by %d\n", cnt, getpid(), pageNo, pageinfo->pageStart, pageinfo->diriedBy);
      cnt++;
    }
    printf("-----------%d end of dirtied pages--------------\n\n", getpid());
  }

  // Get the number of buffered pages first and then increment the buffered page counter by 1.
  // Global and heap have their own instances of xpersist, but the counter is globally accessible.
  // So we need to check if the current xpersist instance is heap or global
  unsigned long GetAndIncrementBufferedPageCount(int pageNo){
    unsigned long num_buffered_pages = 0;
    if (_isHeap) {
      num_buffered_pages = GET_METACOUNTER(globalBufferedHeapPageNoCount);
      INC_METACOUNTER(globalBufferedHeapPageNoCount);
      lprintf("Record heap page %d at pageNoTmp index %zu\n", pageNo, num_buffered_pages);
    } else {
      num_buffered_pages = GET_METACOUNTER(globalBufferedGlobalPageNoCount);
      INC_METACOUNTER(globalBufferedGlobalPageNoCount);
      lprintf("Record global page %d at pageNoTmp index %zu\n", pageNo, num_buffered_pages);
    }
    return num_buffered_pages;
  }

  // Record the page lookup info for the recovery code to use, called by checkandcommit()
  void recordLookUpInfo(int pageNo, unsigned short globalXactID, unsigned short threadID, unsigned long offset, bool dirtied) {
    // No other thread has touched this page, safe to record pageInfo
    if (_pageDependence[pageNo].threadID == 0) {
      unsigned long npages;
      lprintf("First time touching this page %d, no other thread has touched this page, not sure if there will be, so log to tmp\n", pageNo);
      if (_isHeap) {
        npages = GET_METACOUNTER(globalBufferedHeapPageNoCount);
        INC_METACOUNTER(globalBufferedHeapPageNoCount);
        lprintf("Record heap page %d at pageNoTmp index %zu\n", pageNo, npages);
      } else {
        npages = GET_METACOUNTER(globalBufferedGlobalPageNoCount);
        INC_METACOUNTER(globalBufferedGlobalPageNoCount);
        lprintf("Record global page %d at pageNoTmp index %zu\n", pageNo, npages);
      }
      _pageNoTmp[npages] = pageNo;
      _pageLookupTmp[pageNo].xactID = globalXactID;
      _pageLookupTmp[pageNo].threadID = threadID;
      _pageLookupTmp[pageNo].memlogOffset = offset;
      _pageLookupTmp[pageNo].dirtied = dirtied;
    }
    // Page dependence detected
    else if (_pageDependence[pageNo].threadID != 0) {
      // This thread touched this page before
      if (_pageDependence[pageNo].threadID == threadID) {
        lprintf("Thread %d touched this page %d again\n", (int)threadID, pageNo);
        _pageLookupTmp[pageNo].xactID = globalXactID;
        _pageLookupTmp[pageNo].threadID = threadID;
        _pageLookupTmp[pageNo].memlogOffset = offset;
        _pageLookupTmp[pageNo].dirtied = dirtied;
      }
      // Other thread has touched this page, store page lookup info to cache, commit once we exit the outermost nested section
      else if (_pageDependence[pageNo].threadID != threadID) {
        lprintf("Page %d not ready yet, store page lookup info in cache, commit later\n", pageNo);

        // Add this pageNo to commit later (only once!) if this is the first this page is modified
        if (_pageLookupTmp[pageNo].threadID ==  0) {
          unsigned long npages;
          if (_isHeap) {
            npages = GET_METACOUNTER(globalBufferedHeapPageNoCount);
            INC_METACOUNTER(globalBufferedHeapPageNoCount);
            lprintf("Record heap page %d at pageNoTmp index %zu\n", pageNo, npages);
          } else {
            npages = GET_METACOUNTER(globalBufferedGlobalPageNoCount);
            INC_METACOUNTER(globalBufferedGlobalPageNoCount);
            lprintf("Record global page %d at pageNoTmp index %zu\n", pageNo, npages);
          }
          _pageNoTmp[npages] = pageNo;
        } else {
          lprintf("Page %d was touched by %d\n", pageNo, _pageLookupTmp[pageNo].threadID);
        }

        // Buffer the pageInfo, commit when we exit the outermost nested section

        _pageLookupTmp[pageNo].xactID = globalXactID;
        _pageLookupTmp[pageNo].threadID = threadID;
        _pageLookupTmp[pageNo].memlogOffset = offset;
        _pageLookupTmp[pageNo].dirtied = dirtied;
      }
    }
    lprintf("Page %d modified by thread %d at Xact %d\n", pageNo, threadID, globalXactID);
  }

  // Record which thread touched which page (data dependence)
  void recordDependence(int pageNo, unsigned short threadID, void* pageStart) {
    if (_pageDependence[pageNo].threadID != 0 && _pageDependence[pageNo].threadID != threadID) {
      lprintf("Page %d (addr: %p) previously modified by thread %lu, I am thread %d\n",
              pageNo, pageStart, _pageDependence[pageNo].threadID, threadID);
    }
    lprintf("Page %d (addr: %p) is now dirtied by thread %d\n", pageNo, pageStart, threadID);
    _pageDependence[pageNo].threadID = threadID;
  }

  // Commit local modifications to shared mapping
  inline void checkandcommit(bool update, MemoryLog* localMemoryLog) {
    struct shareinfo* shareinfo = NULL;
    struct xpageinfo* pageinfo = NULL;
    int pageNo;
    unsigned long* share;
    unsigned long* local;
    int mypid = getpid();
    bool logged = false;
    unsigned long globalXactID = GET_METACOUNTER(globalTransactionCount);
    unsigned long memlogOffset = 0;

    INC_COUNTER(commit);

    if (_dirtiedPagesList.size() == 0) {
      return;
    }

    lprintf("globalXactID %lu, GET_METACOUNTER(globalTransactionCount): %lu\n",
            globalXactID, GET_METACOUNTER(globalTransactionCount));

    // Increase local counter _trans
    _trans++;

    INC_COUNTER(transactions); // global transaction counter (debugging only)

    lprintf("globalXactID %lu, GET_METACOUNTER(globalTransactionCount): %lu\n",
            globalXactID, GET_METACOUNTER(globalTransactionCount));

#ifdef ENABLE_PROFILING
    clock_t start_time = clock(), diff;
#endif
    START_TIMER(logging);

    // Log pages if it's heap data
    if (_isHeap) {

      // Open a new log file if we have dirtied pages
      localMemoryLog->OpenMemoryLog(_dirtiedPagesList.size(), _isHeap, globalXactID);

      // Loop through all dirty pages and log them to the backend device
      int page_count = 0;
      for (dirtyListType::iterator i = _dirtiedPagesList.begin(); i != _dirtiedPagesList.end(); ++i) {
        bool needsModify = false;
        pageinfo = (struct xpageinfo*)i->second;
        pageNo = pageinfo->pageNo;
        shareinfo = &_pageUsers[pageNo];
        share = (unsigned long*)((intptr_t)_persistentMemory + xdefines::PageSize * pageNo);
        local = (unsigned long*)pageinfo->pageStart;

        // Check if we can skip this log entry
        if (update) {
          if (pageinfo->version != _persistentVersions[pageNo]) {
            needsModify = true;
          }
        } else {
          if (!pageinfo->isUpdated) {
            needsModify = true;
          }
        }

        // Perform actual logging
        if (needsModify) {

          unsigned long* twin = (unsigned long*)xbitmap::getInstance().getAddress(shareinfo->bitmapIndex);

#ifdef PAGE_DENSITY
          // Profile dirty page density
          ProfileDiffs(local, twin);
#endif

#ifdef DIFF_LOGGING
          // Log diffs in dirty page
          localMemoryLog->AppendDiffsToMemoryLog(local, twin, pageNo, globalXactID, localMemoryLog->threadID, _pageLookupTmp);
//        memlogOffset = localMemoryLog->_mempages_offset - logged_bytes;
#else
          // Log whole page
          localMemoryLog->AppendMemoryLog(local, twin, share, pageNo);
          memlogOffset = lseek(localMemoryLog->_mempages_fd, 0, SEEK_CUR) - LogDefines::PageSize;
#endif
          // Record page lookup info for recovery
          recordLookUpInfo(pageNo, globalXactID, localMemoryLog->threadID, memlogOffset, true);

          // Record data dependency
          recordDependence(pageNo, localMemoryLog->threadID, pageinfo->pageStart);

          page_count++;

#ifdef PAGE_DENSITY
          // Counter sanity checks
          if (global_data->stats.pdcount_count != global_data->stats.loggedpages_count) {
            fprintf(stderr, "pdcount: %lu != loggedpages: %lu\n", (unsigned long)global_data->stats.pdcount_count, (unsigned long)global_data->stats.loggedpages_count);
            abort();
          }
#endif
        }
      }

      // Flush log
      localMemoryLog->MakeDurable(localMemoryLog->_mempages_ptr, localMemoryLog->_mempages_filesize);

      // Write an end of log
      localMemoryLog->WriteEndOfLog();

      // Flush end of log
      localMemoryLog->MakeDurable(localMemoryLog->_mempages_ptr, localMemoryLog->_mempages_filesize);

      // Close log
      localMemoryLog->CloseMemoryLog();

      STOP_TIMER(logging);

#ifdef ENABLE_PROFILING
      diff = clock() - start_time;      
#endif
      ADD_COUNTER(logtimer, (double)diff);
    } // end of logging for data on heap

    // Check all pages in the dirty list
    for (dirtyListType::iterator i = _dirtiedPagesList.begin(); i != _dirtiedPagesList.end(); ++i) {
      bool isModified = false;
      pageinfo = (struct xpageinfo*)i->second;
      pageNo = pageinfo->pageNo;

      // Get the shareinfo and persistent address.
      shareinfo = &_pageUsers[pageNo];
      share = (unsigned long*)((intptr_t)_persistentMemory + xdefines::PageSize * pageNo);
      local = (unsigned long*)pageinfo->pageStart;

      // When there are multiple writers on the page and the twin page is not created (bitmapIndex = 0).
      if (shareinfo->users > 1 && shareinfo->bitmapIndex == 0) {
        createTwinPage(pageNo);
      }

      TRACE("%d: commits local modification to shared mapping, xact %d, pageNo %d\n", getpid(), _trans, pageNo);

      lprintf("commits local modification to shared mapping, xact %d, pageNo %d, pageAddr: %p\n", _trans, pageNo,pageinfo->pageStart);

#ifdef LAZY_COMMIT
      // update is true before entering into the critical sections.
      // do the least upates if possible.
      if (update) {

        // Current page is older than the version of shared mapping, then commit local modifications.
        if (pageinfo->version != _persistentVersions[pageNo]) {
          unsigned long* twin = (unsigned long*)xbitmap::getInstance().getAddress(shareinfo->bitmapIndex);
          assert(shareinfo->bitmapIndex != 0);
          assert(xbitmap::getInstance().getVersion(shareinfo->bitmapIndex) != _persistentVersions[pageNo]);

          recordPageChanges(pageNo);
          INC_COUNTER(slowpage);

          // Use the slower page commit, comparing to "twin".
          writePageDiffs(local, twin, share, pageNo);
          setSharedPage(pageNo);

          // Update this page since it may affect results of next transaction.
          updatePage(pageinfo->pageStart, 1, true);
          pageinfo->isUpdated = true;
          isModified = true;
        }
      } else {
        // Only do commits when the page have not been updated.
        if (!pageinfo->isUpdated) {
          // When there are multiple writes on this pae, the page cannot be owned.
          // When this page is not owned, then we do commit.
          if (shareinfo->users != 1 || _pageOwner[pageNo] != mypid) {
            pageinfo->release = true;

            // If the version is the same as shared, use memcpy to commit.
            if (pageinfo->version == _persistentVersions[pageNo]) {
              memcpy(share, local, xdefines::PageSize);
            } else {
              // slow commits
              unsigned long* twin = (unsigned long*)xbitmap::getInstance().getAddress(shareinfo->bitmapIndex);
              assert(shareinfo->bitmapIndex != 0);

              recordPageChanges(pageNo);
              INC_COUNTER(slowpage);
              // Use the slower page commit, comparing to "twin".
              setSharedPage(pageNo);
              writePageDiffs(local, twin, share, pageNo);
            }

            isModified = true;
          } else {
            // Only one user on it and pageOwner is myself.
            pageinfo->release = false;

            // Now there is one less user on this page.
            shareinfo->users--;
          }
        }
      }
#else
      /// If we are not defining "LAZY_COMMIT", then all local modifications has
      //  to be committed to the shared mapping.
      if (update) {
        if (pageinfo->version != _persistentVersions[pageNo]) {
          TRACE("%d: updating pageNo %d\n", getpid(), pageNo);

          unsigned long* twin = (unsigned long*)xbitmap::getInstance().getAddress(shareinfo->bitmapIndex);
          assert(shareinfo->bitmapIndex != 0);
          assert(xbitmap::getInstance().getVersion(shareinfo->bitmapIndex) != _persistentVersions[pageNo]);

          recordPageChanges(pageNo);
          INC_COUNTER(slowpage);

          // Use the slower page commit, comparing to "twin".
          writePageDiffs(local, twin, share, pageNo);

          // Now we need to update this page since it may affect results of next transaction.
          updatePage(pageinfo->pageStart, 1, true);
          pageinfo->isUpdated = true;
          isModified = true;
        }
      } else {
        TRACE("%d: writing pageNo %d\n", getpid(), pageNo);

        if (!pageinfo->isUpdated) {

          if (pageinfo->version == _persistentVersions[pageNo]) {
            TRACE("%d: memcpy pageNo %d\n", getpid(), pageNo);
            memcpy(share, local, xdefines::PageSize);
          } else {
            unsigned long* twin = (unsigned long*)xbitmap::getInstance().getAddress(shareinfo->bitmapIndex);
            assert(shareinfo->bitmapIndex != 0);

            recordPageChanges(pageNo);
            INC_COUNTER(slowpage);
            // Use the slower page commit, comparing to "twin".
            writePageDiffs(local, twin, share, pageNo);

            TRACE("%d: writePageDiffs %d\n", getpid(), pageNo);

          }
          isModified = true;
        }
      }
#endif

      if (isModified) {
        if (shareinfo->users == 1) {
          // If I am the only user, release the share information.
          shareinfo->bitmapIndex = 0;
        }

        // Now there is one less user on this page.
        shareinfo->users--;

        INC_COUNTER(dirtypage_modified);

        // Update the version number.
        _persistentVersions[pageNo]++;
      }

    }
  }

  /// @brief Update every page frame from the backing file if necessary.
  void updateAll(bool cleanup) {
    struct xpageinfo* pageinfo;
    int pageNo = 0;

    // Dump in-updated page frame for safety!!!
    dirtyListType::iterator i;
    for (i = _dirtiedPagesList.begin(); i != _dirtiedPagesList.end(); ++i) {
      pageinfo = (struct xpageinfo*)i->second;
      pageNo = pageinfo->pageNo;

      // Since some page frame has been updated in check phase,
      // Now we don't need to work on these pages anymore.
      if (!pageinfo->isUpdated) {
        TRACE("%d: updating pageNo: %d\n", getpid(), pageNo);
        updatePage(pageinfo->pageStart, 1, pageinfo->release);
      }
    }
    // Now there is no need to use dirtiedPagesList any more
    if (cleanup) {
      _dirtiedPagesList.clear();
      xpageentry::getInstance().cleanup();
    }
  }

  /// @brief Commit all writes.
  inline void memoryBarrier() {
    xatomic::memoryBarrier();
  }

  inline size_t computePageNo(void* addr) {
    return computePage((size_t)addr - (size_t)base());
  }

  inline int computePageOffset(void* addr) {
    return (size_t)addr % (size_t)xdefines::PageSize;
  }

 private:
  // The objects are pairs, mapping void * pointers to sizes.
  typedef std::pair<int, void*> objType;
  typedef HL::STLAllocator<objType, privateheap> dirtyListTypeAllocator;
  typedef std::less<int> localComparator;
  typedef std::multimap<int, void*, localComparator, dirtyListTypeAllocator> dirtyListType;


  inline size_t computePage(size_t index) {
    return (index * sizeof(Type)) / xdefines::PageSize;
  }

  /// @brief Update the given page frame from the backing file.
  void updatePage(void* local, size_t pages, bool release) {
    if (release) {
      madvise(local, xdefines::PageSize * pages, MADV_DONTNEED);
    }

    // Set this page to PROT_READ again.
    mprotect(local, xdefines::PageSize * pages, PROT_READ);
    //  mprotect(local, xdefines::PageSize, PROT_NONE);
  }

  /// True if current xpersist.h is a heap.
  bool _isHeap;

  /// The starting address of the region.
  void* const _startaddr;

  /// The size of the region.
  const size_t _startsize;

  /// A map of dirtied pages.
  dirtyListType _dirtiedPagesList;

  /// The file descriptor for the backing store.
  int _backingFd;

  /// The transient (not yet backed) memory.
  Type* _transientMemory;

  /// The persistent (backed to disk) memory.
  Type* _persistentMemory;

  bool _isProtected;

  /// The file descriptor for the versions.
  int _versionsFd;

  /// The version numbers that are backed to disk.
  volatile unsigned long* _persistentVersions;

  volatile unsigned int _trans;

#ifdef LAZY_COMMIT
  // Every time when we are getting a super block, we will update this information.
  // Then it is used to reduce the checking time in final commit. We only checked those
  // blocks owned by me.
  unsigned long* _ownedblockinfo;
  unsigned long _ownedblocks; // How manu blocks are owned by myself.

  // This array are used to save access type and the pointer for previous copy.
  unsigned long* _pageInfo;

  volatile unsigned long* _pageOwner;
#endif
  /// Local version numbers for each page.
  __m128i allones;

  struct shareinfo {
    volatile unsigned short users;
    volatile unsigned short bitmapIndex;
  };

  struct shareinfo* _pageUsers;

  /// The length of the version array.
  enum {TotalPageNums = sizeof(Type) * NElts / xdefines::PageSize };

#ifdef GET_CHARACTERISTICS
  struct pagechangeinfo {
    unsigned short tid;
    unsigned short version;
  };
  struct pagechangeinfo* _pageChanges;
#endif

  int _threadindex;

  char* logPath;

  // Page lookup metadata for recovery
  struct lookupinfo* _pageLookup;
  int _lookupFd;

  // Tmp page lookup metadata to buffer intermediate changes
  struct lookupinfo* _pageLookupTmp;
  int _lookupTmpFd;

  // Record which thread last touched which page to track data dependencies
  struct pageDependence* _pageDependence;
  int _pageDependenceFd;
  size_t _pageDependenceSize;

  // _pageNoTmp records the pageNo of the buffered pages, indexed from 0:N 
  // in first-in-first-out order
  int* _pageNoTmp;
  int _pageNoTmpFd;

};

#endif
