/*
  Copyright 2015-2017 Hewlett Packard Enterprise Development LP
  
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

#ifndef _LOGGER_H_
#define _LOGGER_H_

/*
 *  @file       logger.h
 *  @brief      Main engine for loggin memory pages.
 *  @author     Terry Ching-Hsiang Hsu  <terryhsu@purdue.edu> 
 *  @author     Indrajit Roy            <indrajitr@hp.com>
 *  @author     Helge Bruegner          <helge.bruegner@hp.com>
 *  @author     Kimberly Keeton         <kimberly.keeton@hp.com>  
 *  @author     Patrick Eugster         <p@cs.purdue.edu>
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <vector>
#include <iterator>
#include <iostream>
#include <sys/types.h>
#include <syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <fstream>
#include "xdefines.h"
#include "prof.h"

//#define DIFF_LOGGING
#define ADDRBYTE sizeof(void*)
#define NVLOGGING
#define LDEBUG 1
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define lprintf(...) \
    do{\
        if ( LDEBUG ) {\
            char str_line[15];\
            char str_pid[15];\
            sprintf(str_pid, "%d", getpid());\
            sprintf(str_line, "%d", __LINE__);\
            fprintf(stderr, "\t|%15s | %5s | %25s | %5s | ", __FILENAME__, str_line, __FUNCTION__, str_pid);\
            fprintf(stderr, __VA_ARGS__);\
            fflush(stderr);\
        }\
    }while (0)\

const char eol_symbol[] = "EOL";

class LogDefines {
 public:
  enum {PageSize = 4096UL };
  enum {PAGE_SIZE_MASK = (PageSize - 1)};
};

enum DurableMethod {
  MSYNC,
  MFENCE,
  MFENCEMSYNC,
  FSYNC
};

#define PATH_CONFIG "/tmp/nvthread.config"

enum LOG_DEST {
  SSD,
  NVM_RAMDISK,
};

class LogAtomic {
 public:
  static inline void add(int i, volatile unsigned long* obj) {
    asm volatile("lock; addl %0, %1"
        : "+r" (i), "+m" (*obj)
        : : "memory");
  }
};

/* Class for log entry to be appended to the end of per-thread MemoryLog */
class LogEntry {
 public:
  struct log_t {
    unsigned long addr;
    char* after_image;
  };
  enum {LogEntrySize = sizeof(unsigned long) + LogDefines::PageSize};

  static struct log_t* alloc(void) {
    static char buf[LogEntry::LogEntrySize];
    static struct log_t *theOneTrueObject = new(buf)log_t;
//      printf("alloc: sizeof LogEntry: %zu\n", sizeof(*theOneTrueObject));
    return theOneTrueObject;
  }
};

#define MAX_PID 32768

/* Class for logging memory operations */
class MemoryLog {
 public:

  LOG_DEST log_dest;
  bool _logging_enabled;

  int nvid;
  int threadID;

  /* For memory pages logging */
  int _mempages_fd;
  int _mempages_file_count;
  unsigned long _local_transaction_id;
  unsigned long _mempages_offset;
  unsigned long _mempages_filesize;
  char _mempages_filename[FILENAME_MAX];
  char* _mempages_ptr;
  static int _DurableMethod;
  int _dirtiedPagesCount;
  unsigned long _buffered_bytes;
  int _log_flags;
  char logPath[FILENAME_MAX];

  /* For heap */
  int _heap_log_fd;
  char _heap_log_filename[FILENAME_MAX];
  void* _heap_base;
  size_t _heap_size;

  /* For global */
  int _global_log_fd;
  char _global_log_filename[FILENAME_MAX];
  void* _global_base;
  size_t _global_size;

  /* For end of log */
  int _eol_fd;
  char* _eol_ptr;
  char _eol_filename[FILENAME_MAX];
  int _eol_size;

  MemoryLog() {
  }
  ~MemoryLog() {
  }

  void initialize(int _nvid) {
#ifdef NVLOGGING
    _logging_enabled = true;
#else
    _logging_enabled = false;
#endif
    if (!_logging_enabled) {
      return;
    }
    lprintf("----------Initializing memorylog class--------------\n");
    ReadConfig();
    INC_METACOUNTER(globalThreadCount);
    threadID = GET_METACOUNTER(globalThreadCount);
    _mempages_file_count = 0;
    _dirtiedPagesCount = 0;
    nvid = _nvid;
  }

  void finalize() {
    if (!_logging_enabled) {
      return;
    }
  }

  void setNVID(int _nvid) {
    nvid = _nvid;
  }

  static bool isCommentStmt(char* stmt) {
    if (strncmp(stmt, "#", strlen("#")) == 0) {
      return true;
    } else {
      return false;
    }
  }

  void ReadConfig(void) {
    log_dest = NVM_RAMDISK;
    _DurableMethod = FSYNC;
    _eol_size = sizeof(eol_symbol);
    _log_flags = O_RDWR | O_ASYNC | O_APPEND;
    return;

    /* Hooks for reading config file*/
    char* field;
    char* token;
    std::string line;
    std::ifstream ifs(PATH_CONFIG);
    while (getline(ifs, line)) {
      field = strtok((char*)line.c_str(), " = ");
      token = strtok(NULL, " = ");
      if (isCommentStmt(field)) {
        continue;
      } else if (strcmp("LOG_DEST", field) == 0) {
        if (strcmp(token, "SSD") == 0) {
          log_dest = SSD;
        } else if (strcmp(token, "NVM_RAMDISK") == 0) {
          log_dest = NVM_RAMDISK;
        }
        lprintf("Log destination: %s, log_dest = %d\n", token, log_dest);
      } else {
        fprintf(stderr, "unknown field in config file: '%s'\n", field);
        abort();
      }
    }
  }

  void OpenMemoryLog(int dirtiedPagesCount, bool isHeap, unsigned long XactID) {
    _dirtiedPagesCount = dirtiedPagesCount;
    _mempages_filesize = _dirtiedPagesCount * LogEntry::LogEntrySize;
    _local_transaction_id = XactID;

    if (log_dest == SSD) {
      sprintf(logPath, "/mnt/ssd2/nvthreads/%d/", nvid);
    } else if (log_dest == NVM_RAMDISK) {
      sprintf(logPath, "/mnt/ramdisk/nvthreads/%d/logs/", nvid);
    } else {
      fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
      abort();
    }
  
    // Create memlog 
    sprintf(_mempages_filename, "%s/MemLog_%d_%lu", logPath, threadID, XactID);
    _mempages_fd = open(_mempages_filename, O_RDWR | O_ASYNC | O_CREAT, 0644);
    if (_mempages_fd == -1) {
      fprintf(stderr, "%d: Error creating %s\n", getpid(), _mempages_filename);
      perror("mkstemp: ");
      abort();
    }
    _mempages_offset = 0;

#ifdef DIFF_LOGGING
    // Allocate memory buffer to store dirtied bytes
    _dirtiedPagesCount = dirtiedPagesCount;
    _buffered_bytes = 0;
    _mempages_ptr = (char*)InternalMalloc(LogDefines::PageSize * dirtiedPagesCount);
    lprintf("Opened memory page log. fd: %d, filename: %s, ptr: %p, offset: %lu\n",
            _mempages_fd, _mempages_filename, _mempages_ptr, _mempages_offset);
#endif
  }

  /* Append a log entry to the end of a memory log */
  int AppendMemoryLog(void* addr) {
    struct LogEntry::log_t newLE;
    newLE.after_image = (char*)((unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK);

    size_t sz;
    int retry = 0;
    do {
      sz = write(_mempages_fd, (void*)newLE.after_image, xdefines::PageSize);
      if (sz == -1) {
        fprintf(stderr, "%d: write image %p error fd: %d, filename: %s\n", getpid(), newLE.after_image, _mempages_fd, _mempages_filename);
        perror("write (page): ");
        // Close and reopen the file to retry write again
        close(_mempages_fd);
        _mempages_fd = open(_mempages_filename, _log_flags);
        retry++;
        // Filesystem error, cannot open/write file
        if (retry == 3) {
          abort();
        }
      }
    }while (sz == -1 && retry < 3);
    
    INC_COUNTER(loggedpages);
    return 0;
  }


#ifdef DIFF_LOGGING
  /* Apply diff bytes from src to dest (vs twin) and return copied bytes */
  inline int logWord(char* src, char* twin, int block, int pageNo, 
                     unsigned short xactID, unsigned short threadID, struct lookupinfo *pageLookupTmp) {
    int copied_bytes = 0;
    int page_start = block * sizeof(long long);
    ssize_t rv = 0;
    for (int i = 0; i < sizeof(long long); i++) {
      if (src[i] != twin[i]) {
        int page_offset = page_start + i;

        START_TIMER(diff_logging);
        memcpy(&_mempages_ptr[_buffered_bytes], &src[i], sizeof(char));

        // Buffer page lookup info for each byte in the page
        pageLookupTmp[pageNo]->xactIDs[page_offset] = xactID;
        pageLookupTmp[pageNo]->threadIDs[page_offset] = threadID;
        pageLookupTmp[pageNo]->memlogOffsets[page_offset] = _buffered_bytes;
        pageLookupTmp[pageNo]->dirtied[page_offset] = true;

        lprintf("Buffering Page %d: <byte, xactID, threadID, memlogOffset> = <%d, %d, %d, %lu>\n", 
                 pageNo, page_offset, 
                 pageLookupTmp[pageNo]->xactIDs[page_offset],
                 pageLookupTmp[pageNo]->threadIDs[page_offset],
                 pageLookupTmp[pageNo]->memlogOffsets[page_offset]);

        STOP_TIMER(diff_logging);
        _buffered_bytes++;
        copied_bytes++;
      }
    }
    lprintf("committed bytes %d\n", copied_bytes);
    return copied_bytes;
  }

  /* Append page diffs to the end of the memory log */
  void AppendDiffsToMemoryLog(const void* local, const void* twin, int pageNo, 
                              unsigned short xactID, unsigned short threadID, 
                              struct lookupinfo pageLookupTmp) {
    int diff_bytes = 0;
    int count = 0;
    size_t sz;

    START_TIMER(diff_calculation);
    // Count diff in bytes
    long long* mylocal = (long long*)local;
    long long* mytwin = (long long*)twin;

    // #blocks: 512
    lprintf("Checking diffs\n");
    for (int i = 0; i < xdefines::PageSize / sizeof(long long); i++) {
      if (mylocal[i] != mytwin[i]) {
        lprintf("commiting %d-th word in page %d\n", i, pageNo);  
        diff_bytes = diff_bytes + logWord( (char*)&mylocal[i], (char*)&mytwin[i], i, pageNo, xactID, threadID, pageLookupTmp);
        lprintf("Wrote %d-th block, diff_bytes: %d\n", i, diff_bytes);
      }
    }
    STOP_TIMER(diff_calculation);
    lprintf("Write diffs for a total of %d / %lu bytes\n", diff_bytes, _buffered_bytes);
  }
#endif

  void CloseMemoryLog(void) {
#ifdef DIFF_LOGGING
    FlushBufferToLog();
#endif
    if (log_dest == SSD || log_dest == NVM_RAMDISK) {
      /* Close the memory mapping log */
      if (_mempages_fd != -1) {
        close(_mempages_fd);
      }
    } else {
      fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
      abort();
    }
    _mempages_file_count++;
    lprintf("Closed file: %s\n", _mempages_filename);
  }

  /* Write an end of log flag to file. It has to be ordered after the actual logs. */
  void WriteEndOfLog(void) {
    int sz = 0;
    sz = write(_mempages_fd, eol_symbol, sizeof(eol_symbol));
    if (sz == -1) {
      fprintf(stderr, "%d: write error fd: %d, filename: %s\n", getpid(), _mempages_fd, _mempages_filename);
      perror("write (addr): ");
      abort();
    }
  }

  /* Flush memory page to backend storage */
  inline int msyncPage(volatile void* __p) {
    return msync((void*)__p, LogDefines::PageSize, MS_SYNC);
  }

  /* Flush cache line to main memory */
  inline void mfencePage(volatile void* __p) {
    __asm__ __volatile__("mfence");
    asm volatile("clflush %0" : "+m"(*(volatile char *)__p));
    __asm__ __volatile__("mfence");
  }

  /* Make sure the log is durable (visible by the restart-code after the program crashes) */
  inline void MakeDurable(volatile void* vaddr, unsigned long length) {
    int rv = 0;
    if (_DurableMethod == MSYNC) {
      rv = msync((void*)vaddr, length, MS_SYNC);
      if (rv != 0) {
        fprintf(stderr, "msync() failed, please handle error before moving on.\n");
        perror("msync(): ");
        abort();
      }
      lprintf("%d: msync %p for %d bytes\n", getpid(), vaddr, LogDefines::PageSize);
    } else if (_DurableMethod == MFENCE) {
      mfencePage(vaddr);
      lprintf("%d: mfence+cflush+mfence for %p for %d bytes\n", getpid(), vaddr, LogDefines::PageSize);
    } else if (_DurableMethod == MFENCEMSYNC) {
      mfencePage(vaddr);
      msyncPage(vaddr);
      lprintf("%d: mfence+cflush+mfence+msync for %p for %d bytes\n", getpid(), vaddr, LogDefines::PageSize);
    } else if (_DurableMethod == FSYNC) {
      rv = fdatasync(_mempages_fd);
      if (rv != 0) {
        fprintf(stderr, "fdatasync() failed, please handle error before moving on.\n");
        perror("fdatasync(): ");
        abort();
      }
    } else {
      fprintf(stderr, "undefined durable method: %d\n", _DurableMethod);
    }
  }

  /* Dump the content of a log entry in hex format */
  void DumpLE(struct LogEntry::log_t* LE) {
    int i;
    char* tmp;
    lprintf("addr: 0x%08lx, before-image:\n", LE->addr);
    tmp = LE->after_image;
    for (i = 0; i < (int)LogDefines::PageSize; i++) {
      lprintf("%02x ", (unsigned)tmp[i] & 0xffU);
      if (i % 50 == 0 && i != 0) {
        lprintf("\n");
      }
    }
    lprintf("\n");
  }

  /* Flush buffer for diff logging */
  void FlushBufferToLog(void){
    ssize_t rv;
    int i = 0;
    if (_buffered_bytes == 0) {
      return;
    }

    rv = write(_mempages_fd, _mempages_ptr, _buffered_bytes);
    if (rv == -1) {
      perror("FlushBufferToLog write failed");
    }
    lprintf("Dumped %lu bytes from %p to file %s\n", _buffered_bytes, _mempages_ptr, _mempages_filename);
    InternalFree(_mempages_ptr, _dirtiedPagesCount * LogDefines::PageSize);
  }

  /* Malloc for diff logging */
  void *InternalMalloc(size_t sz) {
    void *ptr;
    ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
      perror("InternalMalloc mmap failed.");
      abort();
    }
    return ptr;
  }

  /* Free for diff logging */
  void InternalFree(void *ptr, size_t sz){
    if ( (munmap(ptr, sz)) == -1 ) {
      perror("InternalFree munmap mailed");
      abort();
    }
  }
  
};

#endif
