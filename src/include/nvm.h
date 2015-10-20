#ifndef _NVMEMORY_H_
#define _NVMEMORY_H_

/*
    This file is part of NVTHREADS.

    NVTHREADS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    NVTHREADS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NVTHREADS.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 *  @file       nvm.h
 *  @brief      Logging the memory operations to non volatile memory.
 *  @author     Terry Ching-Hsiang Hsu  <terryhsu@purdue.edu> 
 *  @author     Indrajit Roy            <indrajitr@hp.com>
 *  @author     Helge Bruegner          <helge.bruegner@hp.com>
 *  @author     Kimberly Keeton         <kimberly.keeton@hp.com>  
*/

/*
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "logger.h"

#define PATH_CONFIG "/home/terry/workspace/nvthreads/config/nvthread.config"

// Class for log entry to be appended to the end of per-thread MemoryLog 
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
    bool _logging_enabled;

    int threadID;

    // For file mapped varmap log 
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
#ifdef NVLOGGING
        _logging_enabled = true;
#else
        _logging_enabled = false;
#endif

        if ( !_logging_enabled ) {
            return;
        }

        lprintf("----------initializing nvmemory class--------------\n");
        threadID = getpid();
        varlog_dest = DRAM_TMPFS;
//      varlog_dest = DISK;
        OpenVarMap();
        _first_entry = true;
    }

    void finalize(void){

        if ( !_logging_enabled ) {
            return;
        }

        lprintf("----------finalizing nvmemory class--------------\n");
        CloseVarMap();
    }

    void OpenVarMap(void){
        if ( varlog_dest == DISK ) {
            OpenDiskVarMap();
        }
        else if ( varlog_dest == DRAM_TMPFS ) {
            OpenDramTmpfsVarMap();
        }
    }

    void OpenDiskVarMap(void) {
        // Create a temp file for logging mapping between variable and address
        sprintf(_varmap_filename, "varmap_%d", threadID);         
        _varmap_fptr = fopen(_varmap_filename, "w");
        if ( !_varmap_fptr ) {
            lprintf("Error creating file %s for variable address mapping\n", _varmap_filename);
            abort();
        }
        else{
            lprintf("Craeted file %s for variable address mapping\n", _varmap_filename); 
        }        
    }

    void OpenDramTmpfsVarMap(void){
        // Create a temp file for logging mapping between variable and address
        sprintf(_varmap_filename, "/mnt/ramdisk/varmap_%d_XXXXXX", threadID);
        _varmap_fd = mkstemp(_varmap_filename);
        if ( _varmap_fd == -1 ) {
            fprintf(stderr, "%d: Error creating %s\n", getpid(), _varmap_filename);
            abort(); 
        }

        _varmap_offset = 0; 
        
        // Adjust the file size
        if ( ftruncate(_varmap_fd, nvmemory::MapSizeOfLog) ) {
            fprintf(stderr, "Error truncating varmap log file");
            abort();
        }
        
        // Map the temp file to process memory space
        _varmap_ptr = (void *)mmap(NULL, nvmemory::MapSizeOfLog,
                                   PROT_READ | PROT_WRITE, MAP_SHARED, _varmap_fd, _varmap_offset); 
        
    }

    void CloseVarMap(void){
        lprintf("Closing %s\n", _varmap_filename);
        close(_varmap_fd);
//      fclose(_varmap_fptr);
        unlink(_varmap_filename);
        lprintf("Closed VarMap handle\n");
    }
    
    void AppendVarMapLog(void *start, size_t size, char *name){
        if ( !_logging_enabled ) {
            return;
        }

        // Append record to the end of the varmap log
        char tmp[strlen(name)+ sizeof(unsigned long)];
        varmap_entry::varmap_t varmap;
        varmap.addr = (unsigned long)start;
        varmap.name = name;
        sprintf(tmp, "%s:%lx\n", varmap.name, varmap.addr);

        if ( _first_entry ) {
            _first_entry = false;
        }

        // Write log to varmap file
        if( write(_varmap_fd, tmp, strlen(tmp)) == -1){
            fprintf(stderr, "Error writing log to varmap file: %s\n", _varmap_filename);
            abort();
        }
        _varmap_offset = _varmap_offset + strlen(tmp); 

        lprintf("Added variable address map record: %s\n", tmp);
    }

};
*/
#endif
