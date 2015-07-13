#ifndef _NVRECOVERY_H_
#define _NVRECOVERY_H_

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
 *  @file       nvmemory.h
 *  @brief      The main engine for crash recovery. Only the main thread
 *              should have an instance of this recovery class.
 *  @author     Terry Ching-Hsiang Hsu  <terryhsu@purdue.edu> 
 *  @author     Indrajit Roy            <indrajitr@hp.com>
 *  @author     Helge Bruegner          <helge.bruegner@hp.com>
 *  @author     Kimberly Keeton         <kimberly.keeton@hp.com>  
*/

#include <stdlib.h>
#include <stdio.h>

// open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>

// opendir()
#include <unistd.h>
#include <vector>

// read file
#include <iostream>
#include <fstream>

// std map<>
#include <string>
#include <map>

#include "logger.h"

#define PATH_CONFIG "/home/terry/workspace/nvthreads/config/nvthread.config"
enum RECOVER_STATUS{
    RECOVER_SUCCESS,
    RECOVER_FAIL
};

extern "C"
{
    bool _isCrashed(void);
    bool isCrashed(void);
    unsigned long nvrecover(void *dest, size_t size, char *name);
    void* nvmalloc(size_t size, char *name);
}


class nvrecovery {

public:
    nvrecovery(){
//      lprintf("Constructing an instance of nvmemory\n");
    }
    ~nvrecovery(){
//      lprintf("Destructing an instance of nvmemory\n");
    }

    bool _main_thread;
    bool _initialized;
    bool _logging_enabled;
    LOG_DEST log_dest;
    char running_filename[FILENAME_MAX];
    int running_fd;
    char crash_filename[FILENAME_MAX];
    int crash_fd;
    char log_path_prefix[FILENAME_MAX];

    // Store a list of file names for memory pages
    std::vector<std::string> *mempage_file_vector;

    // Store a list of varmap file names
    std::vector<std::string> *varmap_file_vector;
    
    // Hash map of the actual variable and address mapping
    std::map<std::string, unsigned long> *varmap; 


    void initialize(bool main_thread){
        _main_thread = main_thread;
        
#ifdef NVLOGGING
        _logging_enabled = true;
#else
        _logging_enabled = false;
#endif

        if ( !_logging_enabled ) {
            return;
        }

        lprintf("----------initializing nvrecovery class--------------\n");
        if (!_initialized){
            // TODO: Read from config
//          log_dest = MemoryLog::log_dest;
            log_dest = DRAM_TMPFS;
            // Read the configuration file to decide where to log the
            // <variable, address> mapping
            mempage_file_vector = new std::vector<std::string>;
            varmap = new std::map<std::string, unsigned long>;
            varmap_file_vector = new std::vector<std::string>; 

            if (log_dest == DISK){
                sprintf(log_path_prefix, "./");
            } else if (log_dest == DRAM_TMPFS){
                sprintf(log_path_prefix, "/mnt/tmpfs/");
            }

            if ( _main_thread ) {
                sprintf(running_filename, "_running");
                sprintf(crash_filename, "_crashed");

                if (_isCrashed()){
                    // Recover the variable and address mapping
                    createRecoverHashMap(); 
                }
                else{
                    running_fd = create_flag_file(running_filename);
                }
                
            }

            _initialized = true;
        }
    }

    void finalize(void) {
        if ( !_logging_enabled ) {
            return;
        }

        lprintf("----------finalizing nvrecovery class--------------\n");
        _initialized = false;
        if ( _main_thread ) {
            lprintf("Main thread removing crash flag files\n");
//          close(running_fd);
//          close(crash_fd);
            unlink(running_filename);
            unlink(crash_filename);
        }
//      delete mempage_file_vector;
//      delete varmap_file_vector;
//      delete varmap;
    }

    // Flag file indicates the crash status of the application
    int create_flag_file(char *filename) {
        int fd = open(filename, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        if ( fd < 0 ) {
            lprintf("Error opening %s\n", running_filename);
            abort();
        }
        lprintf("Created %s\n", filename);
        return fd;
    }

    // Check if the application had crashed before this execution
    bool _isCrashed(void) {
        bool rv = false;
        lprintf("Checking running file: %s\n", running_filename);

        // Running file exists before we create it
        if ( access(running_filename, F_OK) != -1 ) {
            lprintf("Your program crashed before.  Please recover your progress using libnvthread API\n");
            rv = true;
            crash_fd = create_flag_file(crash_filename);
        } else {
            lprintf("Your program did not crash before.  Continue normal execution\n");
            rv = false;
        }
        return rv;
    }

    // Collect the file names that match pattern
    void collectFiles(char *pattern) {
        DIR *dir;
        struct dirent *ent;
        lprintf("looking for file with pattern: %s\n", pattern);
        if ( (dir = opendir(log_path_prefix)) != NULL ) {
            while ((ent = readdir(dir)) != NULL) {
                std::string *str = new std::string(ent->d_name);
                std::size_t found = str->find(pattern);
                // Found match
                if ( found != std::string::npos ){
                    if ( strcmp(pattern, "varmap") == 0) {
                        varmap_file_vector->push_back(*str);
                    }
                    else if (strcmp(pattern, "MemLog") == 0){
                        mempage_file_vector->push_back(*str);
                    }
                }
            }
            closedir(dir);
        }
    }

    // Build the variable address hash map for later query
    void buildVarMap(void) {
        for (std::vector<std::string>::iterator it = varmap_file_vector->begin(); it != varmap_file_vector->end(); ++it) {
            char fn[256];
            char *name;
            char *addr;
            std::string line;
            sprintf(fn, "%s%s", log_path_prefix, (*it).c_str());
    
            std::ifstream ifs(fn);
            while (getline(ifs, line)) {
                name = strtok((char *)line.c_str(), ":");
                addr = strtok(NULL, ":");
                varmap->insert(std::make_pair((std::string)name, (unsigned long)strtoul(addr, NULL, 16)));
            }
        }
    }

    // Create the hash map for heap (only!) variable and address.
    // Multiple threads will have the same address for the same
    // object.      
    void createRecoverHashMap(void) {
        lprintf("Building variable hash map for variables\n");
        collectFiles((char*)"varmap");
        buildVarMap();
        dumpVarMap();
        lprintf("Done with hash map.\n");
    }
    
    void collectMemlogFileNames(void){
        collectFiles((char*)"MemLog");
    }

    void recoverFromMemlog(void *dest, size_t size, unsigned long addr){
        for (std::vector<std::string>::iterator it = mempage_file_vector->begin(); it != mempage_file_vector->end(); ++it) {
            char fn[256];
            int fd;
            unsigned long offset;
            char *ptr;

            // Get memlog file name
            sprintf(fn, "%s%s", log_path_prefix, (*it).c_str());
//          lprintf("Memlog: %s\n", fn);

            // Set pointer to the start of a memory page    
            fd = open(fn, O_RDONLY);
            if ( fd == -1 ) {
                lprintf("can't read memory page: %s\n", fn);
                abort();
            }
            ptr = (char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
            if ( ptr == MAP_FAILED ) {
                printf("can't mmap memory page %s\n", fn);
                abort();
            }

            // Read address of the memory page
            unsigned long paddr;
            memcpy(&paddr, (char *)ptr, sizeof(unsigned long));
//          lprintf("Checking page 0x%08lx\n", paddr);

            // Found memory page, copy content to dest
            if ( (addr & ~LogDefines::PAGE_SIZE_MASK) == paddr ) {
//              lprintf("Located memory page: 0x%08lx\n", paddr);
                if ( paddr > addr ) {
                    offset = paddr - addr;
                } else {
                    offset = addr - paddr;
                }

                // Copy data
                LogAtomic::add((int)sizeof(void *), &offset);
                memcpy((char *)dest, (char *)(ptr + offset), size);
//              lprintf("Memcpy starting from 0x%08lx, offset: 0x%lx\n", paddr, offset);
                return;
            }
        }
    }

    unsigned long recoverVarAddr(void *dest, size_t size, char *name) {
        unsigned long addr = 0x00;
        // Look for variable name
        std::map<std::string, unsigned long>::iterator it;
        it = varmap->find((std::string)name);
        if ( it == varmap->end() ) {
            lprintf("Cannot find %s\n", name);
            return addr;
        }
   
        // Found variable name
        addr = (unsigned long)it->second;
        lprintf("Found %s at 0x%08lx\n", name, addr); 
   
        // Copy the memory page content to hint
        collectMemlogFileNames();

        // Actual recover work
        recoverFromMemlog(dest, size, addr);
    
        return addr;
    }

    unsigned long nvrecover(void *dest, size_t size, char *name) {
        lprintf("Hint: 0x%p, size: %zu, name: %s\n", dest, size, name);
        // Find the address of the variable in varmap log
        unsigned long addr = recoverVarAddr(dest, size, name);
        if ( !addr ) {
            lprintf("Error, can't find variable named %s\n", name);
            return 0;
        }

        // Find the data by address in memory pages
//      printf("Recovered addr 0x%08lx %d\n", addr, *(int *)dest);
        return addr;
    }

    void dumpVarMap(void) {
        if ( varmap->size() ) {
            lprintf("walking varmap\n");
            lprintf("size of varmap: %zd\n", varmap->size());
            for (std::map<std::string, unsigned long>::iterator it = varmap->begin(); it != varmap->end(); it++) {
                lprintf("%s, 0x%08lx\n", it->first.c_str(), it->second);
            }
            lprintf("walked varmap\n");
        } else {
            lprintf("No variables found, skip dumping variables\n");
        }
    }

};

#endif
