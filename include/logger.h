#ifndef _LOGGER_H_
#define _LOGGER_H_

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
#include <errno.h>

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

#define PATH_CONFIG "/home/terry/workspace/nvthreads/config/nvthread.config"
enum LOG_DEST {
    DISK,
    RAM,
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
        char *before_image;
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
    enum {LogEntryPerFile = 1};
    enum {MapSizeOfLog = LogEntryPerFile * LogEntry::LogEntrySize};

    LOG_DEST log_dest;
    bool _enable_logging;

    int threadID;

    /* For memory pages logging */
    int _mempages_fd;
    int _mempages_file_count;
    unsigned long _mempages_offset;
    char _mempages_filename[FILENAME_MAX];
    void *_mempages_ptr;
    unsigned long _logentry_count;

    MemoryLog() {
//      initialize();
    }
    ~MemoryLog() {
//      finalize();
    }

    void initialize() {
        lprintf("----------initializing memorylog class--------------\n");
        _enable_logging = true;
        ReadConfig();
        threadID = getpid();
        _mempages_file_count = 0;
        _logentry_count = 0;
        OpenMemoryLog();
    }
    void finalize() {
        lprintf("----------finalizing memorylog class--------------\n");
        CloseMemoryLog();
    }

    void ReadConfig(void) {
        log_dest = DISK;
    }

    void OpenMemoryLog(void) {
        if ( log_dest == DISK ) {
            OpenDiskLog();
        } else if ( log_dest == RAM ) {
            OpenRamLog();
        } else if ( log_dest == DRAM_TMPFS ) {
            OpenDramTmpfsLog();
        } else if ( log_dest == NVRAM_TMPFS ) {
            OpenNvramTmpfsLog();
        } else {
            fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
            abort();
        }

        if ( _mempages_ptr == MAP_FAILED ) {
            fprintf(stderr, "%d: Failed to open mmap shared file for logging\n", getpid());
            abort();
        }
    }

    void OpenDiskLog(void) {
        // Create a temp file for logging memory pages
        sprintf(_mempages_filename, "MemLog_%d_%d_XXXXXXX", threadID, _mempages_file_count);
        _mempages_fd = mkstemp(_mempages_filename);
        if ( _mempages_fd == -1 ) {
            fprintf(stderr, "%d: Error creating temp log file\n", getpid());
            abort();
        }
        _mempages_offset = 0;

        // Adjust the file size
        if ( ftruncate(_mempages_fd, LogEntry::LogEntrySize * MemoryLog::LogEntryPerFile) ) {
            fprintf(stderr, "Error truncating memory log file");
            abort();
        }

        // Map the temp file to process memory space
        _mempages_ptr = (void *)mmap(NULL, LogEntry::LogEntrySize * MemoryLog::LogEntryPerFile,
                                     PROT_READ | PROT_WRITE, MAP_SHARED, _mempages_fd, _mempages_offset);

        lprintf("Created fd: %d, filename: %s, LogEntry::LogEntrySize: %d\n", _mempages_fd, _mempages_filename, LogEntry::LogEntrySize);
    }

    /* Map a anonymous memory region for logging */
    void OpenRamLog(void) {
        _mempages_fd = -1;
        _mempages_offset = 0;
        _mempages_ptr = (void *)mmap(NULL, MapSizeOfLog,
                                     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        lprintf("mmap to anonymous region for logging, mapped size: %d bytes\n", MapSizeOfLog);
    }

    void OpenDramTmpfsLog(void) {

    }

    void OpenNvramTmpfsLog(void) {

    }

    /* Append a log entry to the end of the memory log */
    int AppendMemoryLog(void *addr) {
        if ( !_enable_logging ) {
            lprintf("Logging disabled, skip logging for addr %p page 0x%08lx\n", addr, (unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK);
            return 0;
        }
        
        /** TODO: change to stack variable, no need to allocate log
         *  entry on heap anymore */
        struct LogEntry::log_t *newLE = LogEntry::alloc();
        newLE->addr = (unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK;
        newLE->before_image = (char *)((unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK);

        lprintf("Append log for addr %p page %lu: 0x%08lx\n", addr, _logentry_count, newLE->addr);

        if ( _mempages_offset >= MapSizeOfLog ) {
            lprintf("Memory mapped file %s is full!\n", _mempages_filename);
            /* Flush out old log */
            SyncMemoryLog();

            /* Create a new log */
            OpenMemoryLog();
        }
        memcpy((char *)_mempages_ptr + _mempages_offset, &(newLE->addr), sizeof(unsigned long));

//      unsigned long ptr;
//      memcpy(&ptr, (char *)_mempages_ptr + _mempages_offset, sizeof(unsigned long));
//      lprintf("Copied back to ptr: 0x%08lx\n", ptr);

        LogAtomic::add((int)sizeof(void *), &_mempages_offset);

//      lprintf("%s ptr moves to %p, offset: %lu\n", filename, ptr, offset);
        memcpy((char *)_mempages_ptr + _mempages_offset, newLE->before_image, LogDefines::PageSize);
        LogAtomic::add((int)LogDefines::PageSize, &_mempages_offset);
//      lprintf(" %s ptr moves to %p, offset: %lu\n", filename, ptr, offset);
//      DumpLE(newLE);

        _logentry_count++;
        return 0;
    }

    int AddMapRecord(void) {

    }

    /* Make sure the memory log are durable after this function returns */
    void SyncMemoryLog(void) {
        lprintf("Please flush %s to make it durable\n", _mempages_filename);

        /* TODO: Make sure I'm durable after this point!! */
        unlink(_mempages_filename);
        close(_mempages_fd);
        _mempages_file_count++;
    }

    void CloseDiskMemoryLog(void) {
        if ( _mempages_fd != -1 ) {
            lprintf("removing memory log %s\n", _mempages_filename);
            unlink(_mempages_filename);
            close(_mempages_fd);
        }
    }

    void CloseRamMemoryLog(void) {

    }

    void CloseDramTmpfsMemoryLog(void) {

    }

    void CloseNvramTmpfsMemoryLog(void) {

    }

    void CloseMemoryLog(void) {
        if ( log_dest == DISK ) {
            /* Close the memory mapping log */
            CloseDiskMemoryLog();
        } else if ( log_dest == RAM ) {
            CloseRamMemoryLog();
        } else if ( log_dest == DRAM_TMPFS ) {
            CloseDramTmpfsMemoryLog();
        } else if ( log_dest == NVRAM_TMPFS ) {
            CloseNvramTmpfsMemoryLog();
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

    /* Dump the content of a log entry in hex format */
    void DumpLE(struct LogEntry::log_t *LE) {
        int i;
        char *tmp;
        lprintf("addr: 0x%08lx, before-image:\n", LE->addr);
        tmp = LE->before_image;
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
