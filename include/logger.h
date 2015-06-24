#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdio.h>
#include <vector>
#include <iterator>
#include <iostream>
#include "xatomic.h"
#include "xglobals.h"

#define LDEBUG 0
#define lprintf(...) \
    do{\
        if ( LDEBUG ) {\
            fprintf(stderr, __VA_ARGS__);\
        }\
    }while (0)\

class LogDefines {
public:
    enum {PageSize = 4096UL };
};

#define PATH_CONFIG "/home/terry/workspace/nvthreads/config/nvthread.config"
enum LOG_DEST{
    DISK,
    RAM,
    DRAM_TMPFS,
    NVRAM_TMPFS
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
        static struct log_t *theOneTrueObject = new(buf) log_t;
//      printf("alloc: sizeof LogEntry: %zu\n", sizeof(*theOneTrueObject));
        return theOneTrueObject;
    }
}; 

/* Class for logging memory operations */
class MemoryLog{
public:
    enum {LogEntryPerFile = 1000};
    enum {MapSizeOfLog = LogEntryPerFile * LogEntry::LogEntrySize};   

    LOG_DEST log_dest;
    int threadID;
    int fd;
    int file_count;
    unsigned long offset;
    char filename[FILENAME_MAX];
    void *ptr;
    bool logging;
    
    
    MemoryLog(){
    }
    ~MemoryLog(){
    }   

    void initialize() {
        ReadConfig();
        if ( logging ) {
            threadID = getpid();
            file_count = 0;
            OpenMemoryLog();
            lprintf("%d: Initialized memory log for thread %d\n", getpid(), threadID);
        }
    }
    void finalize() {
        if( logging ){
            ClearMemoryLog();       
            lprintf("%d: finalized memory log for thread %d\n", getpid(), threadID);
        }
    }

    void ReadConfig(void){
        log_dest = DISK;
        logging = false;
    }

    void OpenMemoryLog(void) {
        if ( log_dest == DISK ) {
            OpenDiskLog();
        }
        else if ( log_dest == RAM ){
            OpenRamLog();
        }
        else if ( log_dest == DRAM_TMPFS ) {
            OpenDramTmpfsLog();
        }
        else if ( log_dest == NVRAM_TMPFS ) {
            OpenNvramTmpfsLog();
        }
        else{
            fprintf(stderr, "Error: unknown logging destination: %d\n", log_dest);
            ::abort();
        }

        if ( ptr == MAP_FAILED ) {
            fprintf(stderr, "%d: Failed to open mmap shared file for logging\n", getpid());
            ::abort();
        }
    }

    void OpenDiskLog(void){
        // Create a temp file for logging memory pages
        sprintf(filename, "MemLog_%d_%d_XXXXXXX", threadID, file_count);
        fd = mkstemp(filename);
        if ( fd == -1 ) {
            fprintf(stderr, "%d: Error creating temp log file\n", getpid());
            ::abort();
        }
        lprintf("%d: fd: %d, filename: %s, LogEntry::LogEntrySize: %d\n", getpid(), fd, filename, LogEntry::LogEntrySize);
        offset = 0;

        // Adjust the file sizes
        if ( ftruncate(fd, LogEntry::LogEntrySize * MemoryLog::LogEntryPerFile) ) {
            fprintf(stderr, "Error truncating memory log file");
            ::abort();
        }

        // Map the temp file to process memory space
        ptr = (void *)mmap(NULL, LogEntry::LogEntrySize * MemoryLog::LogEntryPerFile,
                           PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset); 
    }

    /* Map a anonymous memory region for logging */
    void OpenRamLog(void){
        fd = -1;
        offset = 0;
        ptr = (void *)mmap(NULL, MapSizeOfLog,
                           PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        lprintf("%d: mmap to anonymous region for logging, mapped size: %d bytes\n", getpid(), MapSizeOfLog);
    }
       
    void OpenDramTmpfsLog(void){
        
    }

    void OpenNvramTmpfsLog(void){

    }

    /* Append a log entry to the end of the memory log */
    int AppendMemoryLog(void *addr) {
        if ( !logging ) {
            return 0;
        }

        int i;
        char *tmp;
        struct LogEntry::log_t *newLE = LogEntry::alloc();
        newLE->addr =  (unsigned long)addr & ~xdefines::PAGE_SIZE_MASK;
        newLE->before_image = (char *)((unsigned long)addr & ~xdefines::PAGE_SIZE_MASK);
        
        lprintf("%d: append log for addr %p page 0x%08lx\n", getpid(), addr, newLE->addr);

        if ( offset >= MapSizeOfLog ) {
            lprintf("%d: Memory mapped file %s is full!\n", getpid(), filename);
            /* Flush out old log */
            SyncMemoryLog();
            
            /* Create a new log */
            OpenMemoryLog();
        }
        memcpy((char*)ptr + offset, &(newLE->addr), sizeof(unsigned long));
        xatomic::add((int)sizeof(void*), &offset); 

//      lprintf("%d: %s ptr moves to %p, offset: %lu\n", getpid(), filename, ptr, offset);
        memcpy((char *)ptr + offset, newLE->before_image, LogDefines::PageSize);
        xatomic::add((int)LogDefines::PageSize, &offset); 
//      lprintf("%d: %s ptr moves to %p, offset: %lu\n", getpid(), filename, ptr, offset);
//      DumpLE(newLE);

        return 0;
    }

    /* Make sure the memory log are durable after this function returns */
    void SyncMemoryLog(void){
        lprintf("Please flush %s to make it durable\n", filename);

        /* TODO: Make sure I'm durable after this point!! */
            
        unlink(filename);
        close(fd);
        file_count++;
    }
    
    void ClearMemoryLog(void) {
        if ( fd != -1 ) {
            lprintf("%d: removing memory log %s\n", getpid(), filename);
            unlink(filename);
            close(fd);
        }

//      off_t fsize;
//      fsize = lseek(fd, 0, SEEK_END);
//      lprintf("closed file size: %zu\n", fsize);
    }

    /* Dump the content of a log entry in hex format */
    void DumpLE(struct LogEntry::log_t *LE){                
        int i;
        char *tmp;
        lprintf("%d: addr: 0x%08lx, before-image:\n", getpid(), LE->addr);
        tmp = LE->before_image;
        for (i = 0; i < (int)LogDefines::PageSize; i++) {
            lprintf("%02x ", (unsigned)tmp[i] & 0xffU);
            if ( i % 50 == 0 && i != 0) {
                lprintf("\n");
            }
        }
        lprintf("\n"); 

    }
    /* Allocate one log entry using Hoard memory allocator (overloaded new) */
    MemoryLog* alloc(void) {
        static char buf[sizeof(MemoryLog)];
        static class MemoryLog *theOneTrueObject = new(buf) MemoryLog;
        lprintf("alloc: sizeof MemoryLog: %zu\n", sizeof(*theOneTrueObject));
        return theOneTrueObject;
    }

};

#endif
