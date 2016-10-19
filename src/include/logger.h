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

#ifndef _LOGGER_H_
#define _LOGGER_H_

/*
 *  @file       logger.h
 *  @brief      Main engine for loggin memory pages.
 *  @author     Terry Ching-Hsiang Hsu  <terryhsu@purdue.edu> 
 *  @author     Indrajit Roy            <indrajitr@hp.com>
 *  @author     Helge Bruegner          <helge.bruegner@hp.com>
 *  @author     Kimberly Keeton         <kimberly.keeton@hp.com>  
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


#define DIFF_LOGGING


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

#define PATH_CONFIG "/home/terry/workspace/nvthreads/config/nvthread.config"
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

  enum {LogEntryPerFile = 10000};
  enum {MapSizeOfLog = LogEntryPerFile * LogEntry::LogEntrySize};

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
  unsigned long _logentry_count;
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
//      threadID = getpid();
    INC_METACOUNTER(globalThreadCount);
    threadID = GET_METACOUNTER(globalThreadCount);
    _mempages_file_count = 0;
    _logentry_count = 0;
    _dirtiedPagesCount = 0;
    nvid = _nvid;
//      sprintf(_heap_log_filename, "/mnt/ramdisk/MemLog_%d_Heap", threadID);
//      sprintf(_global_log_filename, "/mnt/ramdisk/MemLog_%d_Global", threadID);
//      _heap_log_fd = open(_heap_log_filename, _log_flags | O_CREAT | O_EXCL, 0777);
//      close(_heap_log_fd);
//      _global_log_fd = open(_global_log_filename, _log_flags | O_CREAT | O_EXCL, 0777);
//      close(_global_log_fd);

  }

  void finalize() {
//      lprintf("----------finalizing memorylog class--------------\n");
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
//      log_dest = SSD;
    log_dest = NVM_RAMDISK;
    _DurableMethod = FSYNC;
    _eol_size = 4;
    _log_flags = O_RDWR | O_ASYNC | O_APPEND;
    return;

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

  /* Memory log filename: /path/to/MemLog_threadID_globalXactID_localXactID */
  void OpenMemoryLog(int dirtiedPagesCount, bool isHeap, unsigned long XactID) {
    _dirtiedPagesCount = dirtiedPagesCount;
    _mempages_filesize = _dirtiedPagesCount * LogEntry::LogEntrySize;
    _logentry_count = 0;
    _local_transaction_id = XactID;

    if (log_dest == SSD) {
      sprintf(logPath, "/mnt/ssd2/tmp/%d", nvid);
    } else if (log_dest == NVM_RAMDISK) {
      sprintf(logPath, "/mnt/ramdisk/nvthreads/%d", nvid);
//    sprintf(logPath, "/mnt/ramdisk/%d/logs", nvid);
    } else {
      fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
      abort();
    }

//  lprintf("Creating log path: %s\n", logPath);
//  if ((mkdir(logPath, 0777)) == -1) {
//    perror("Error when mkdir(memLogPath)");
//  }
//  lprintf("Created log path: %s\n", logPath);


//  sprintf(logPath, "%s/log", logPath);
//  lprintf("Creating log path: %s\n", logPath);
//  if ((mkdir(logPath, 0777)) == -1) {
//    perror("Error when mkdir(memLogPath)");
//  }
//  lprintf("Created memLog path: %s\n", logPath);


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

  void WriteLogBeforeProtection(void* base, size_t size, bool isHeap) {
    printf("heap?: %d, base %p, size: %zu\n", isHeap, base, size);
    // Open file descriptor
    int fd = -1;
    char filename[FILENAME_MAX];
    if (isHeap) {
      _heap_log_fd = open(_heap_log_filename, O_WRONLY | O_ASYNC, 0644);
      _heap_base = base;
      _heap_size = size;
      fd = _heap_log_fd;
      sprintf(filename, "%s", _heap_log_filename);
    } else {
      _global_log_fd = open(_global_log_filename, O_WRONLY | O_ASYNC, 0644);
      _global_base = base;
      _global_size = size;
      fd = _global_log_fd;
      sprintf(filename, "%s", _global_log_filename);
    }

    if (fd == -1) {
      fprintf(stderr, "%d: Error creating %s\n", getpid(), _mempages_filename);
      perror("open log: ");
      abort();
    }

    // Write actual memory log
    if (isHeap) {
      size_t sz;
      int chunks = 0;
      unsigned long offset;
      while (chunks < 4) {
        sz = write(fd, (void*)base, size);
        if (sz == size) {
          printf("%d: wrote %zu bytes to log: %s\n", getpid(), sz, filename);
        } else {
          printf("%d: wrote %zu bytes to log: %s, but != size %zu\n", getpid(), sz, filename, size);
        }
      }
    } else {
      size_t sz = write(fd, (void*)base, size);
      if (sz == -1) {
        fprintf(stderr, "%d: write base %p, size: %zu error fd: %d, file: %s\n", getpid(), (void*)base, size, fd, filename);
        perror("write (LogBeforeProtection): ");
        close(fd);
        abort();
      }
      if (sz == size) {
        printf("%d: wrote %zu bytes to log: %s\n", getpid(), sz, filename);
      } else {
        printf("%d: wrote %zu bytes to log: %s, but != size %zu\n", getpid(), sz, filename, size);
      }

    }


    // Write EOL
    const char eol[] = "EOL";
    size_t sz = write(fd, eol, sizeof(eol));
    if (sz == -1) {
      fprintf(stderr, "%d: write error fd: %d\n", getpid(), fd);
      perror("write (eol for LogBeforeProtection): ");
      abort();
    }

    // Flush log
    int rv = fdatasync(fd);
    if (rv != 0) {
      fprintf(stderr, "fdatasync() failed, please handle error before moving on.\n");
      perror("fdatasync(): ");
      abort();
    }

    // Close log
    close(fd);

  }


  /* Append a log entry to the end of the memory log */
  int AppendMemoryLog(void* addr) {
    int fflags;
    struct LogEntry::log_t newLE;
    newLE.after_image = (char*)((unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK);

    size_t sz;
    sz = write(_mempages_fd, (void*)newLE.after_image, xdefines::PageSize);
    if (sz == -1) {
      fprintf(stderr, "%d: write image %p error fd: %d, filename: %s\n", getpid(), newLE.after_image, _mempages_fd, _mempages_filename);
      perror("write (page): ");

      close(_mempages_fd);
      _mempages_fd = open(_mempages_filename, _log_flags);
      if (_mempages_fd == -1) {
        fprintf(stderr, "%d: Still open error, cannot create new log, abort\n", getpid());
        abort();
      } else {
        fprintf(stderr, "opened a file descriptor %d\n", _mempages_fd);
      }
    }

    INC_COUNTER(loggedpages);
    _logentry_count++;
    return 0;
  }

  /* Apply diff bytes from src to dest (vs twin) and return copied bytes */
  inline int commitWord(char* src, char* twin) {
    int copied_bytes = 0;
    ssize_t rv = 0;
    lprintf("commiting word\n");
    for (int i = 0; i < sizeof(long long); i++) {
      if (src[i] != twin[i]) {
        lprintf("Writing %d-th byte in this block, copied_bytes: %d\n", i, copied_bytes);
//      dest[i] = src[i];
        START_TIMER(diff_logging);
//      rv = write(_mempages_fd, &src[i], sizeof(char));
        memcpy(&_mempages_ptr[_buffered_bytes], &src[i], sizeof(char));
        STOP_TIMER(diff_logging);
//      PRINT_TIMER(diff_logging);
        _buffered_bytes++;
        copied_bytes++;
      }
    }
    lprintf("committed bytes %d\n", copied_bytes);
    return copied_bytes;
  }

  /* Append page diffs to the end of the memory log */
  void AppendDiffsToMemoryLog(const void* local, const void* twin) {
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
        diff_bytes += commitWord((char*)&mylocal[i], (char*)&mytwin[i]);
        lprintf("Wrote %d-th block, diff_bytes: %d\n", i, diff_bytes);
      }
    }
    STOP_TIMER(diff_calculation);
    lprintf("Write diffs for a total of %d / %lu bytes\n", diff_bytes, _buffered_bytes);
  }


  /* Make sure the memory log are durable after this function returns */
  void SyncMemoryLog(void) {
    close(_mempages_fd);
    unlink(_mempages_filename);
    _mempages_file_count++;
  }

  void CloseDiskMemoryLog(void) {
    if (_mempages_fd != -1) {
//          lprintf("Removing memory log %s\n", _mempages_filename);
      close(_mempages_fd);
//          unlink(_mempages_filename);
    }
  }
  void CloseDramTmpfsMemoryLog() {
    if (_mempages_fd != -1) {
//          lprintf("Removing memory log %s\n", _mempages_filename);
//          printf("Removing memory log %s\n", _mempages_filename);
      close(_mempages_fd);
//          unlink(_mempages_filename);
    }
  }

  void CloseNvramTmpfsMemoryLog(void) {

  }

  void CloseMemoryLog(void) {
#ifdef DIFF_LOGGING
    FlushBufferToLog();
#endif
    if (log_dest == SSD) {
      /* Close the memory mapping log */
      CloseDiskMemoryLog();
    } else if (log_dest == NVM_RAMDISK) {
      CloseDramTmpfsMemoryLog();
    } else {
      fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
      abort();
    }
    _mempages_file_count++;
    lprintf("Closed file: %s\n", _mempages_filename);
  }

  void WriteEOLDiskLog() {

  }

  void WriteEOLDramTmpfsLog() {
    int sz = 0;
    const char eol[] = "EOL";
    sz = write(_mempages_fd, eol, sizeof(eol));
    if (sz == -1) {
      fprintf(stderr, "%d: write error fd: %d, filename: %s\n", getpid(), _mempages_fd, _mempages_filename);
      perror("write (addr): ");
      abort();
    }
  }

  void WriteEOLNvramTmpfsLog() {

  }

  // Write an end of log flag to file.  It has to be ordered after the actual logs.
  void WriteEndOfLog(void) {
    WriteEOLDramTmpfsLog();
  }

  /* Allocate one log entry using Hoard memory allocator (overloaded new) */
  MemoryLog* alloc(void) {
    static char buf[sizeof(MemoryLog)];
    static class MemoryLog *theOneTrueObject = new(buf)MemoryLog;
    lprintf("alloc: sizeof MemoryLog: %zu\n", sizeof(*theOneTrueObject));
    return theOneTrueObject;
  }

  /* Flush memory page to backend storage */
  inline int msyncPage(volatile void* __p) {
    return msync((void*)__p, LogDefines::PageSize, MS_SYNC);
  }

  /* Flush cache line to main memory */
  inline void mfencePage(volatile void* __p) {
//      printf("Flushing %p!\n", __p);
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

  void *InternalMalloc(size_t sz) {
    void *ptr;
    ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
      perror("InternalMalloc mmap failed.");
      abort();
    }
    return ptr;
  }
  void InternalFree(void *ptr, size_t sz){
    if ( (munmap(ptr, sz)) == -1 ) {
      perror("InternalFree munmap mailed");
      abort();
    }
  }
  
};

#endif
