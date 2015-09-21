#ifndef _LOGGER_H_
#define _LOGGER_H_

/*
    This file is part of NVTREADS.

    NVTREADS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    NVTREADS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NVTREADS.  If not, see <http://www.gnu.org/licenses/>.
*/

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

class LogDefines {
public:
    enum {PageSize = 4096UL };
    enum {PAGE_SIZE_MASK = (PageSize - 1)};
};


enum DurableMethod{
    MSYNC,
    MFENCE,
    MFENCEMSYNC,
    FSYNC
};

#define PATH_CONFIG "/home/terry/workspace/nvthreads/config/nvthread.config"
enum LOG_DEST {
    DISK,
    DRAM_TMPFS,
    NVRAM_TMPFS
};

class LogAtomic {
public:
    static inline void add(int i, volatile unsigned long *obj) {
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
        char *after_image;
    };
    enum {LogEntrySize = sizeof(unsigned long) + LogDefines::PageSize};

    static struct log_t* alloc(void) {
        static char buf[LogEntry::LogEntrySize];
        static struct log_t *theOneTrueObject = new(buf)log_t;
//      printf("alloc: sizeof LogEntry: %zu\n", sizeof(*theOneTrueObject));
        return theOneTrueObject;
    }
};

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
    unsigned long _transaction_id;
    unsigned long _mempages_offset;
    unsigned long _mempages_filesize;
    char _mempages_filename[FILENAME_MAX];
    char *_mempages_ptr;
    unsigned long _logentry_count;
    static int _DurableMethod;
    int _dirtiedPagesCount; 
    int _log_flags;
    char log_path_prefix[FILENAME_MAX]; 

    /* For heap */
    int _heap_log_fd;
    char _heap_log_filename[FILENAME_MAX];
    void *_heap_base;
    size_t _heap_size;

    /* For global */
    int _global_log_fd;
    char _global_log_filename[FILENAME_MAX]; 
    void *_global_base;
    size_t _global_size; 

    /* For end of log */
    int _eol_fd;
    char *_eol_ptr;
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
        if ( !_logging_enabled ) {
            return;
        }
        lprintf("----------initializing memorylog class--------------\n");
        ReadConfig();
        threadID = getpid();
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
        lprintf("----------finalizing memorylog class--------------\n");
        if ( !_logging_enabled ) {
            return;
        }
    }

    void setNVID(int _nvid){
        nvid = _nvid;
    }

    static bool isCommentStmt(char *stmt) {
        if ( strncmp(stmt, "#", strlen("#")) == 0 ) {
            return true;
        } else {
            return false;
        }
    }

    void ReadConfig(void) {
        log_dest = DRAM_TMPFS;
//      log_dest = DISK;
        _DurableMethod = FSYNC;
        _eol_size = 4;
        _log_flags = O_RDWR | O_ASYNC | O_APPEND;
//      log_dest = DISK;
        return;

        char *field;
        char *token;
        std::string line;
        std::ifstream ifs(PATH_CONFIG);
        while (getline(ifs, line)) {
            field = strtok((char *)line.c_str(), " = ");
            token = strtok(NULL, " = ");
            if ( isCommentStmt(field) ) {
                continue;
            } else if ( strcmp("LOG_DEST", field) == 0 ) {
                if ( strcmp(token, "DISK") == 0 ) {
                    log_dest = DISK;
                } else if ( strcmp(token, "DRAM_TMPFS") == 0 ) {
                    log_dest = DRAM_TMPFS;
                }

                lprintf("Log destination: %s, log_dest = %d\n", token, log_dest);
            } else {
                fprintf(stderr, "unknown field in config file: '%s'\n", field);
                abort();
            }
        }
    }

    void OpenMemoryLog(int dirtiedPagesCount, unsigned long transid) {
        _dirtiedPagesCount = dirtiedPagesCount;
        _mempages_filesize = _dirtiedPagesCount * LogEntry::LogEntrySize;
        _logentry_count = 0;
        _transaction_id = transid;
        if ( log_dest == DISK ) {
            sprintf(log_path_prefix, "/mnt/ssd/terry/tmp/%d", nvid);
            OpenDiskLog();
        } else if ( log_dest == DRAM_TMPFS ) {
            sprintf(log_path_prefix, "/mnt/ramdisk/%d", nvid);
            OpenDramTmpfsLog();
        } else if ( log_dest == NVRAM_TMPFS ) {
            sprintf(log_path_prefix, "/mnt/nvmfs/%d", nvid);
            OpenNvramTmpfsLog();
        } else {
            fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
            abort();
        }

        if ( _mempages_ptr == MAP_FAILED ) {
            lprintf("%d: Failed to open mmap shared file %s for logging, logging dest set to %d\n", getpid(), _mempages_filename, log_dest);
            abort();
        } else {
            lprintf("Opened memory page log. fd: %d, filename: %s, LogEntry::LogEntrySize: %d\n", _mempages_fd, _mempages_filename, LogEntry::LogEntrySize);
        }
    }

    void OpenDiskLog() {
        sprintf(_mempages_filename, "%s/MemLog_%d_%lu_XXXXXXX", log_path_prefix, threadID, _transaction_id);
        _mempages_fd = mkostemp(_mempages_filename, _log_flags);

        if ( _mempages_fd == -1 ) {
            fprintf(stderr, "%d: Error creating %s\n", getpid(), _mempages_filename);
            perror("mkstemp: ");
            abort();
        }       
    }

    /* Create memory log files at tmpfs in ram (make sure the file system is mounted before running this) */
    void OpenDramTmpfsLog(void) {
        sprintf(_mempages_filename, "%s/MemLog_%d_%lu_XXXXXXX", log_path_prefix, threadID, _transaction_id);
        _mempages_fd = mkostemp(_mempages_filename, _log_flags);

        if ( _mempages_fd == -1 ) {
            fprintf(stderr, "%d: Error creating %s\n", getpid(), _mempages_filename);
            perror("mkstemp: ");
            abort();
        }
        _mempages_offset = 0;
    }

    void OpenNvramTmpfsLog(void) {
        sprintf(_mempages_filename, "%s/MemLog_%d_%lu_XXXXXXX", log_path_prefix, threadID, _transaction_id);
        _mempages_fd = mkostemp(_mempages_filename, _log_flags);

        if ( _mempages_fd == -1 ) {
            fprintf(stderr, "%d: Error creating %s\n", getpid(), _mempages_filename);
            abort();
        }
        _mempages_offset = 0;
        /***TODO finish open nvm log file: make sure there's delay */
    }


    void WriteLogBeforeProtection(void *base, size_t size, bool isHeap){
        printf("heap?: %d, base %p, size: %zu\n", isHeap, base, size);
        // Open file descriptor
        int fd = -1;        
        char filename[FILENAME_MAX];
        if ( isHeap ) {
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

        if ( fd == -1 ) {
            fprintf(stderr, "%d: Error creating %s\n", getpid(), _mempages_filename);
            perror("open log: ");
            abort(); 
        }

        // Write actual memory log
        if ( isHeap ) {
            size_t sz;
            int chunks = 0;
            unsigned long offset;
            while (chunks < 4) {           
                sz = write(fd, (void *)base, size);
                if ( sz == size ) {
                    printf("%d: wrote %zu bytes to log: %s\n", getpid(), sz, filename);
                } else {
                    printf("%d: wrote %zu bytes to log: %s, but != size %zu\n", getpid(), sz, filename, size);
                }
            }
        }
        else{
            size_t sz = write(fd, (void *)base, size);
            if ( sz == -1 ) {
                fprintf(stderr, "%d: write base %p, size: %zu error fd: %d, file: %s\n", getpid(), (void *)base, size, fd, filename);
                perror("write (LogBeforeProtection): ");
                close(fd);
                abort();
            }
            if ( sz == size ) {
                printf("%d: wrote %zu bytes to log: %s\n", getpid(), sz, filename);
            } else {
                printf("%d: wrote %zu bytes to log: %s, but != size %zu\n", getpid(), sz, filename, size);
            }

        }


        // Write EOL
        const char eol[] = "EOL";
        size_t sz = write(fd, eol, sizeof(eol));
        if ( sz == -1 ) {
            fprintf(stderr, "%d: write error fd: %d\n", getpid(), fd);
            perror("write (eol for LogBeforeProtection): ");
            abort();
        }

        // Flush log
        int rv = fdatasync(fd);
        if ( rv != 0 ) {
            fprintf(stderr, "fdatasync() failed, please handle error before moving on.\n");
            perror("fdatasync(): ");
            abort();
        }

        // Close log
        close(fd);

    }
    

    /* Append a log entry to the end of the memory log */
    int AppendMemoryLog(void *addr) {
        int fflags;
        /*
        if ( !_logging_enabled ) {
            lprintf("Logging disabled, skip logging for addr %p page 0x%08lx\n", addr, (unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK);
            return 0;
        }
        */
        
        struct LogEntry::log_t newLE;
        newLE.addr = (unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK;
        newLE.after_image = (char *)((unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK);
        lprintf("Appending log at file %s for addr 0x%08lx\n", _mempages_filename, newLE.addr); 
        
        size_t sz;
        sz = write(_mempages_fd, &newLE.addr, sizeof(void*));
        if ( sz == -1 ) {
            fprintf(stderr, "%d: write addr %p error fd: %d, filename: %s\n", getpid(), (void *)newLE.addr, _mempages_fd, _mempages_filename);
            perror("write (addr): ");

            close(_mempages_fd);
            _mempages_fd = open(_mempages_filename, _log_flags);
            if ( _mempages_fd == -1 ) {
                fprintf(stderr, "%d: Still open error, cannot create new log, abort\n", getpid());
                abort();
            } else {
                fprintf(stderr, "opened a file descriptor %d\n", _mempages_fd);
            }
        }
        sz = write(_mempages_fd, (void *)newLE.after_image, LogDefines::PageSize);
        if ( sz == -1 ) {
            fprintf(stderr, "%d: write image %p error fd: %d, filename: %s\n", getpid(), newLE.after_image, _mempages_fd, _mempages_filename);
            perror("write (page): ");

            close(_mempages_fd);
            _mempages_fd = open(_mempages_filename, _log_flags);
            if ( _mempages_fd == -1 ) {
                fprintf(stderr, "%d: Still open error, cannot create new log, abort\n", getpid());
                abort();
            } else {
                fprintf(stderr, "opened a file descriptor %d\n", _mempages_fd);
            } 
        }

//      mfencePage((void *)newLE.addr);

//      memcpy((char *)(_mempages_ptr + _mempages_offset), &newLE.addr, sizeof(unsigned long));
//      _mempages_offset += ADDRBYTE;

//      memcpy((char *)(_mempages_ptr + _mempages_offset), newLE.after_image, LogDefines::PageSize);
//      _mempages_offset += LogDefines::PageSize;
        INC_COUNTER(loggedpages);
        _logentry_count++;
        return 0;
    }

    int AddMapRecord(void) {

    }

    /* Make sure the memory log are durable after this function returns */
    void SyncMemoryLog(void) {
//      printf("Please flush %s to make it durable\n", _mempages_filename);

        close(_mempages_fd);
        unlink(_mempages_filename);
        _mempages_file_count++;
    }

    void CloseDiskMemoryLog(void) {
        if ( _mempages_fd != -1 ) {
//          lprintf("Removing memory log %s\n", _mempages_filename);
            close(_mempages_fd);
//          unlink(_mempages_filename);
        }
    }
    void CloseDramTmpfsMemoryLog() {
        if ( _mempages_fd != -1 ) {
//          lprintf("Removing memory log %s\n", _mempages_filename);
//          printf("Removing memory log %s\n", _mempages_filename);
            close(_mempages_fd);
//          unlink(_mempages_filename);
        }
    }

    void CloseNvramTmpfsMemoryLog(void) {

    }

    void CloseMemoryLog(void) {
        if ( log_dest == DISK ) {
            /* Close the memory mapping log */
            CloseDiskMemoryLog();
        } else if ( log_dest == DRAM_TMPFS ) {
            CloseDramTmpfsMemoryLog();
        } else if ( log_dest == NVRAM_TMPFS ) {
            CloseNvramTmpfsMemoryLog();
        } else {
            fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
            abort();
        }
        _mempages_file_count++; 
    }
    
    void WriteEOLDiskLog() {

    }

    void WriteEOLDramTmpfsLog(){
//      sprintf(_eol_filename, "/mnt/ramdisk/MemLog_EOL_%d_%d_XXXXXXX", threadID, _mempages_file_count);
//      _eol_fd = mkostemp(_eol_filename, O_SYNC);

//      if ( _eol_fd == -1 ) {
//          fprintf(stderr, "%d: Error creating %s\n", getpid(), _eol_filename);
//          abort();
//      }

        // Adjust the file size 
//      if ( ftruncate(_eol_fd, _eol_size) ) {
//          fprintf(stderr, "Error truncating memory log file");
//          abort();
//      }
//      _eol_ptr = (char *)mmap(NULL, _eol_size,
//                                   PROT_READ | PROT_WRITE, MAP_SHARED, _eol_fd, 0);
        int sz = 0;
        const char eol[] = "EOL";
        sz = write(_mempages_fd, eol, sizeof(eol));
        if ( sz == -1 ) {
            fprintf(stderr, "%d: write error fd: %d, filename: %s\n", getpid(), _mempages_fd, _mempages_filename);
            perror("write (addr): ");
            abort();
        }

//      close(_eol_fd);
    }

    void WriteEOLNvramTmpfsLog() {

    }
    
    // Write an end of log flag to file.  It has to be ordered after the actual logs.
    void WriteEndOfLog(void){
        if ( log_dest == DISK ) {
            WriteEOLDiskLog();
        } else if ( log_dest == DRAM_TMPFS ) {
            WriteEOLDramTmpfsLog();
        } else if ( log_dest == NVRAM_TMPFS ) {
            WriteEOLNvramTmpfsLog();
        } else {
            fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
            abort();             
        }
    }

    /* Allocate one log entry using Hoard memory allocator (overloaded new) */
    MemoryLog* alloc(void) {
        static char buf[sizeof(MemoryLog)];
        static class MemoryLog *theOneTrueObject = new(buf)MemoryLog;
        lprintf("alloc: sizeof MemoryLog: %zu\n", sizeof(*theOneTrueObject));
        return theOneTrueObject;
    }

    /* Flush memory page to backend storage */
    inline int msyncPage(volatile void *__p){
        return msync((void *)__p, LogDefines::PageSize, MS_SYNC);
    }

    /* Flush cache line to main memory */
    inline void mfencePage(volatile void *__p) {
//      printf("Flushing %p!\n", __p);
        __asm__ __volatile__("mfence");
        asm volatile("clflush %0" : "+m"(*(volatile char *)__p));
        __asm__ __volatile__("mfence");
    }

    /* Make sure the log is durable (visible by the restart-code after the program crashes) */
    inline void MakeDurable(volatile void *vaddr, unsigned long length){
        int rv = 0;
        if ( _DurableMethod == MSYNC ) {
            rv = msync((void *)vaddr, length, MS_SYNC);
            if( rv != 0 ){
                fprintf(stderr, "msync() failed, please handle error before moving on.\n");
                perror("msync(): ");
                abort();
            }
            lprintf("%d: msync %p for %d bytes\n", getpid(), vaddr, LogDefines::PageSize);
        } else if (_DurableMethod == MFENCE) {
            mfencePage(vaddr);    
            lprintf("%d: mfence+cflush+mfence for %p for %d bytes\n", getpid(), vaddr, LogDefines::PageSize);
        } else if(_DurableMethod == MFENCEMSYNC) {
            mfencePage(vaddr);
            msyncPage(vaddr);
            lprintf("%d: mfence+cflush+mfence+msync for %p for %d bytes\n", getpid(), vaddr, LogDefines::PageSize);
        } else if(_DurableMethod == FSYNC) {            
            rv = fdatasync(_mempages_fd);
            if ( rv != 0 ) {
                fprintf(stderr, "fdatasync() failed, please handle error before moving on.\n");
                perror("fdatasync(): ");
                abort();
            }
        } else {
            fprintf(stderr, "undefined durable method: %d\n", _DurableMethod);
        }
    }

    /* Dump the content of a log entry in hex format */
    void DumpLE(struct LogEntry::log_t *LE) {
        int i;
        char *tmp;
        lprintf("addr: 0x%08lx, before-image:\n", LE->addr);
        tmp = LE->after_image;
        for (i = 0; i < (int)LogDefines::PageSize; i++) {
            lprintf("%02x ", (unsigned)tmp[i] & 0xffU);
            if ( i % 50 == 0 && i != 0 ) {
                lprintf("\n");
            }
        }
        lprintf("\n");
    }

};

#endif
