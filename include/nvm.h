#ifndef _NVMEMORY_H_
#define _NVMEMORY_H_

/** Logging the memory operations to non volatile memory   **/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "logger.h"

#define PATH_CONFIG "/home/terry/workspace/nvthreads/config/nvthread.config"

/* Class for log entry to be appended to the end of per-thread MemoryLog */
class varmap_entry{   
    enum {MaxVarNameLength = 1024};
public:
    struct varmap_t {
        unsigned long addr;
        char *name;
    };
};

class nvmemory{
    enum {NumEntryVarMap = 10000};
    enum {MaxEntrySize = 64};
    enum {MapSizeOfLog = NumEntryVarMap * MaxEntrySize};

public:
    LOG_DEST varlog_dest;
    bool _first_entry;
    int threadID;

    /* For file mapped varmap log */
    FILE *_varmap_fptr;
    void *_varmap_ptr;
    int _varmap_fd;
    unsigned long _varmap_offset;
    char _varmap_filename[FILENAME_MAX];

    nvmemory() {
//      initialize();
    }
    ~nvmemory(){
//      finalize();
    }

    void initialize(void){
        lprintf("----------initializing nvmemory class--------------\n");
        threadID = getpid();
        OpenVarMap();
        varlog_dest = DISK;
        _first_entry = true;
//      lprintf("Inialized VarMap for nvmalloc\n");
    }

    void finalize(void){
        lprintf("----------finalizing nvmemory class--------------\n"); 
        CloseVarMap();
    }

    void OpenVarMap(void){
        if ( varlog_dest == DISK ) {
            OpenDiskVarMap();
        }
    }

    void OpenDiskVarMap(void) {
        // Create a temp file for logging mapping between variable and address

#if 0
        sprintf(_varmap_filename, "varmap_%d_XXXXXX", threadID);
        _varmap_fd = mkstemp(_varmap_filename);
        if ( _varmap_fd == -1 ) {
            fprintf(stderr, "Error creating file %s for variable address mapping\n", _varmap_filename);
            abort();
        }
        _varmap_offset = 0;

        // Adjust the file size
        if ( ftruncate(_varmap_fd, MapSizeOfLog) ) {
            fprintf(stderr, "Error truncating var map log file");
            abort();
        }
        // Map the temp file to process memory space
        _varmap_ptr = (void *)mmap(NULL, MapSizeOfLog, PROT_READ | PROT_WRITE, MAP_SHARED, _varmap_fd, _varmap_offset);
#endif
        sprintf(_varmap_filename, "varmap_%d", threadID);         
        _varmap_fptr = fopen(_varmap_filename, "w");
        if ( !_varmap_fptr ) {
            lprintf("Error creating file %s for variable address mapping\n", _varmap_filename);
        }
        else{
            lprintf("Craeted file %s for variable address mapping\n", _varmap_filename); 
        }
        
    }

    void CloseVarMap(void){
        if ( varlog_dest == DISK ) {
            CloseDiskVarMap();
        }
        lprintf("Closed VarMap handle\n");
    }
    
    void CloseDiskVarMap(void){
        close(_varmap_fd);
        fclose(_varmap_fptr);
        unlink(_varmap_filename);
    }

    void AppendVarMapLog(void *start, size_t size, char *name){
        varmap_entry::varmap_t varmap;
        varmap.addr = (unsigned long)start;
        varmap.name = name;
        
        if ( varlog_dest == DISK) {
            LogToDisk(&varmap);
        }
    }
    
    void LogToDisk(varmap_entry::varmap_t *varmap){
#if 0 
        lprintf("before log varmap, ptr: %p, offset: %lu\n", _varmap_ptr, _varmap_offset);
        memcpy((char *)_varmap_ptr + _varmap_offset, &(varmap->addr), sizeof(unsigned long));
        LogAtomic::add((int)sizeof(void *), &_varmap_offset); 
        memcpy((char *)_varmap_ptr + _varmap_offset, varmap->name, sizeof(varmap->name));
        LogAtomic::add((int)sizeof(varmap->name), &_varmap_offset); 
        lprintf("after log varmap, ptr: %p, offset: %lu\n", _varmap_ptr, _varmap_offset);
#endif
        if ( _first_entry ) {
            fprintf(_varmap_fptr, "%s:%lx\n", varmap->name, varmap->addr);
            _first_entry = false;
        }
        else {
//          fprintf(_varmap_fptr, ",%s:%lx", varmap->name, varmap->addr);
            fprintf(_varmap_fptr, "%s:%lx\n", varmap->name, varmap->addr);
        }

    }
};
#endif
