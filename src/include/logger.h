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
#include <errno.h>
#include <fstream>

#define ADDRBYTE sizeof(void*)
#define NVLOGGING

#define LDEBUG 0
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
    enum {LogEntryPerFile = 10000};
    enum {MapSizeOfLog = LogEntryPerFile * LogEntry::LogEntrySize};

    LOG_DEST log_dest;
    bool _logging_enabled;

    int threadID;

    /* For memory pages logging */
    int _mempages_fd;
    int _mempages_file_count;
    unsigned long _mempages_offset;
    char _mempages_filename[FILENAME_MAX];
    char *_mempages_ptr;
    unsigned long _logentry_count;

    MemoryLog() {
    }
    ~MemoryLog() {
    }

    void initialize() {
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
        OpenMemoryLog();
    }

    void finalize() {
        if ( !_logging_enabled ) {
            return;
        }
        lprintf("----------finalizing memorylog class--------------\n");
        CloseMemoryLog();
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

    void OpenMemoryLog(void) {
        if ( log_dest == DISK ) {
            OpenDiskLog();
        } else if ( log_dest == DRAM_TMPFS ) {
            OpenDramTmpfsLog();
        } else if ( log_dest == NVRAM_TMPFS ) {
            OpenNvramTmpfsLog();
        } else {
            fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
            abort();
        }

        if ( _mempages_ptr == MAP_FAILED ) {
            fprintf(stderr, "%d: Failed to open mmap shared file %s for logging, logging dest set to %d\n", getpid(), _mempages_filename, log_dest);
            abort();
        } else {
//          printf("Created fd: %d, filename: %s, LogEntry::LogEntrySize: %d\n", _mempages_fd, _mempages_filename, LogEntry::LogEntrySize);
        }
    }

    void OpenDiskLog(void) {
        // Create a temp file for logging memory pages
        sprintf(_mempages_filename, "MemLogDISK_%d_%d_XXXXXXX", threadID, _mempages_file_count);
        _mempages_fd = mkstemp(_mempages_filename);
        if ( _mempages_fd == -1 ) {
            fprintf(stderr, "%d: Error creating %s\n", getpid(), _mempages_filename);
            abort();
        }
        _mempages_offset = 0;

        // Adjust the file size
        if ( ftruncate(_mempages_fd, LogEntry::LogEntrySize * MemoryLog::LogEntryPerFile) ) {
            fprintf(stderr, "Error truncating memory log file");
            abort();
        }

        // Map the temp file to process memory space
        _mempages_ptr = (char *)mmap(NULL, LogEntry::LogEntrySize * MemoryLog::LogEntryPerFile,
                                     PROT_READ | PROT_WRITE, MAP_SHARED, _mempages_fd, _mempages_offset);

    }

    /* Map a anonymous memory region for logging */
    void OpenRamLog(void) {
    }

    /* Create memory log files at tmpfs in ram (make sure the file system is mounted before running this) */
    void OpenDramTmpfsLog(void) {
        sprintf(_mempages_filename, "/mnt/tmpfs/MemLogDRAM_%d_%d_XXXXXXX", threadID, _mempages_file_count);
        _mempages_fd = mkstemp(_mempages_filename);

        if ( _mempages_fd == -1 ) {
            fprintf(stderr, "%d: Error creating %s\n", getpid(), _mempages_filename);
            abort();
        }
        _mempages_offset = 0;

        // Adjust the file size
        if ( ftruncate(_mempages_fd, LogEntry::LogEntrySize * MemoryLog::LogEntryPerFile) ) {
            fprintf(stderr, "Error truncating memory log file");
            abort();
        }

        // Map the temp file to process memory space
        _mempages_ptr = (char *)mmap(NULL, LogEntry::LogEntrySize * MemoryLog::LogEntryPerFile,
                                     PROT_READ | PROT_WRITE, MAP_SHARED, _mempages_fd, _mempages_offset);
//      printf("New log: %s\n", _mempages_filename);
    }

    void OpenNvramTmpfsLog(void) {

    }

    /* Append a log entry to the end of the memory log */
    int AppendMemoryLog(void *addr) {

        if ( !_logging_enabled ) {
//          printf("Logging disabled, skip logging for addr %p page 0x%08lx\n", addr, (unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK);
            return 0;
        }

        struct LogEntry::log_t newLE;
        newLE.addr = (unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK;
        newLE.before_image = (char *)((unsigned long)addr & ~LogDefines::PAGE_SIZE_MASK);

//      printf("page fault at: 0x%lx, offset %lu, logentry: %lu\n", newLE.addr, _mempages_offset, _logentry_count);

        memcpy((char *)(_mempages_ptr + _mempages_offset), &newLE.addr, sizeof(unsigned long));
        _mempages_offset += ADDRBYTE;

        memcpy((char *)(_mempages_ptr + _mempages_offset), newLE.before_image, LogDefines::PageSize);
        _mempages_offset += LogDefines::PageSize;

        if ( _mempages_offset >= MapSizeOfLog ) {
//          printf("Memory mapped file %s is full, reset mempage offset (%lu) to 0\n", _mempages_filename, _mempages_offset);

            // Flush out old log
            SyncMemoryLog();

            // Create a new log
            OpenMemoryLog();
        }

//      printf("Append log at file %s for addr %p page %lu: 0x%08lx\n", _mempages_filename, addr, _logentry_count, newLE.addr);

        _logentry_count++;
        return 0;
    }

    int AddMapRecord(void) {

    }

    /* Make sure the memory log are durable after this function returns */
    void SyncMemoryLog(void) {
//      printf("Please flush %s to make it durable\n", _mempages_filename);

        /* TODO: Make sure I'm durable after this point!! */
        close(_mempages_fd);
        unlink(_mempages_filename);
        _mempages_file_count++;
    }

    void CloseDiskMemoryLog(void) {
        if ( _mempages_fd != -1 ) {
            lprintf("Removing memory log %s\n", _mempages_filename);
            close(_mempages_fd);
            unlink(_mempages_filename);
        }
    }
    void CloseDramTmpfsMemoryLog() {
        if ( _mempages_fd != -1 ) {
            lprintf("Removing memory log %s\n", _mempages_filename);
            close(_mempages_fd);
            unlink(_mempages_filename);
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
