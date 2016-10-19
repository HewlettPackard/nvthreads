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

#ifndef _NVRECOVERY_H_
#define _NVRECOVERY_H_

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

// readlink
#include <unistd.h>

// rand()
#include <time.h>

#include "logger.h"

#define PATH_CONFIG "/home/terry/workspace/nvthreads/config/nvthread.config"
enum RECOVER_STATUS {
    RECOVER_SUCCESS,
    RECOVER_FAIL
};


extern "C"
{
    bool isCrashed(void);
    unsigned long nvrecover(void *dest, size_t size, char *name);
    void* nvmalloc(size_t size, char *name);
}

struct lookupinfo {
    unsigned short xactID;
    unsigned short threadID;
    unsigned long memlogOffset;    
    bool dirtied;   
};

struct pageDependence {
    unsigned long threadID; //record the threadID last touched this page
};

/* struct for log entry to be appended to the end of per-thread MemoryLog */
struct varmap_entry {
//      unsigned long addr;
    char name[128];
    size_t size;
    int pageNo;
    int pageOffset;
};

class nvrecovery {

public:
    nvrecovery() {
//      lprintf("Constructing an instance of nvmemory\n");
    }
    ~nvrecovery() {
//      lprintf("Destructing an instance of nvmemory\n");
    }

    // FIXME: Why limit the file size of varmap?? make it appendable
    enum {NumEntryVarMap = 10000};
    enum {MaxEntrySize = 64};
    enum {MapSizeOfLog = NumEntryVarMap * MaxEntrySize};
    enum {MAXPID = 32768};

    int nvid;
    int nvlib_linenum;
    char nvlib_crash[FILENAME_MAX];
    char logPath[FILENAME_MAX];
    char memLogPath[FILENAME_MAX];
    bool crashed;
    bool _main_thread;
    bool _initialized;
    bool _logging_enabled;
    LOG_DEST log_dest;
    int threadID;

    /* For file mapped varmap log */
    DIR *logdir;
    FILE *_varmap_fptr;
    void *_varmap_ptr;
    int _varmap_fd;
    char _varmap_filename[FILENAME_MAX];

    // Store a list of file names for memory pages
    std::vector<std::string> *mempage_file_vector;

    // Store a list of varmap file names
    std::vector<std::string> *varmap_file_vector;
    
    // Map of the actual variable and address mapping
    std::map<std::string, unsigned long> *varmap;
   
    // Unique list of PID
    std::map<unsigned long, unsigned long> *PIDs;

    // Unique list of GID
    std::map<unsigned long, unsigned long> *GIDs;

    // Fast lookup
    struct lookupinfo *_pageLookupHeap;
    size_t numPagesHeap;
    struct lookupinfo *_pageLookupGlobals;
    size_t numPagesGlobals;
    
    // Variable mapping info
    std::map<std::string, struct varmap_entry> *recoveredVarmap;

    void initialize(bool main_thread) {
        _main_thread = main_thread;
        threadID = getpid();
#ifdef NVLOGGING
        _logging_enabled = true;
#else
        _logging_enabled = false;
#endif

//      return;
        if ( !_logging_enabled ) {
            return;
        }
        
//      lprintf("----------initializing nvrecovery class--------------\n");
//      if ( !_initialized ) {        
//      if ( getpid() != 0 ) {
            
            srand(time(NULL));
            strcpy(nvlib_crash, "/tmp/nvlib.crash");

            // TODO: Read from config
//          log_dest = MemoryLog::log_dest;
            log_dest = NVM_RAMDISK;
//          log_dest = SSD;
            // Read the configuration file to decide where to log the
            
//          logdir = NULL;
//          mempage_file_vector = new std::vector<std::string>;
//          varmap = new std::map<std::string, unsigned long>;
//          varmap_file_vector = new std::vector<std::string>;
//          PIDs = new std::map<unsigned long, unsigned long>;
//          GIDs = new std::map<unsigned long, unsigned long>;

            // Main thread checks whether the program crashed before and sets the crash flag
            if ( _main_thread ) {
                SetCrashedFlag();
            }

//          lprintf("crashed: %d\n", crashed);
            if ( isCrashed() ) {
                // Recover the variable and address mapping and memory pages
//                  CreateVariableMap();

                // Collect all the file names of the memory log files
//                  CreateMemlogMap();
            }
            recoveredVarmap = NULL;
            _pageLookupHeap = NULL;
            _pageLookupGlobals = NULL;

            // Create new VarMap file for current run
            OpenVarMap();
            _initialized = true;
//      }
    }

    void finalize(void) {
        if ( !_logging_enabled ) {
            return;
        }

//      lprintf("----------finalizing nvrecovery class--------------\n");
        _initialized = false;
        if ( _main_thread ) {
//          lprintf("Main thread removing crash flag files\n");
            Delete_nvlibcrash_entry();
            Delete_varmap();
            Delete_memlog();
            CloseVarMap(); 
        }
        if ( logdir != NULL ) {
//          lprintf("closing logdir\n");
//          closedir(logdir);   // Heap Layer error: _magic != MAGIC
        }
//      delete mempage_file_vector;
//      delete varmap_file_vector;
//      delete varmap;
        close(_varmap_fd);
    }

    // Create the path to store memory log and variable mapping
    void CreateLogPath(void) {
        // Create direcotry for current nvid to save varmap and MemLog
        if ( (mkdir(logPath, 0777)) == -1 ) {
            perror("Error when mkdir(logPath)");
            abort();
        }
        lprintf("Created log path: %s\n", logPath);

        // Create direcotry for current nvid to save varmap and MemLog
        lprintf("Creating memlog path: %s\n", memLogPath);
        if ( (mkdir(memLogPath, 0777)) == -1 ) {
            perror("Error when mkdir(memLogPath)");
            abort();
        }
        lprintf("Created log path: %s\n", memLogPath);
    }

    char *GetLogPath(void){
        return logPath;
    }

    // Open variable mapping file descriptor
    void OpenVarMap(void) {
        // Create a temp file for logging mapping between variable and address
//      sprintf(_varmap_filename, "%svarmap_%d_XXXXXX", logPath, threadID);
//      _varmap_fd = mkostemp(_varmap_filename, O_RDWR | O_ASYNC | O_APPEND);
        sprintf(_varmap_filename, "%svarmap", logPath);

        // Main thread (!_intialized) checks if file exists.  If so, rename file to "recover" first
        if ( !_initialized && access(_varmap_filename, F_OK) != -1 ) {
            char _recoverFname[1024];
            sprintf(_recoverFname, "%s_recover", _varmap_filename);
            if ( rename(_varmap_filename, _recoverFname) != 0 ){
                lprintf("error: unable to rename file\n");
            }
            lprintf("File %s exists, rename to %s\n", _varmap_filename, _recoverFname);
        }

        _varmap_fd = open(_varmap_filename, O_RDWR | O_SYNC | O_CREAT | O_APPEND, 0644);    

        if ( _varmap_fd == -1 ) {
            fprintf(stderr, "%d: Error creating %s\n", getpid(), _varmap_filename);
            perror("OpenVarMap");
            abort();
        }
//      lprintf("Opened varmap file at: %s\n", _varmap_filename);
    }

    // Close variable mapping file descriptor and remove it
    void CloseVarMap(void) {
        close(_varmap_fd);
        unlink(_varmap_filename);
        lprintf("Closed %s\n", _varmap_filename);
    }

    // Add a entry for variable mapping. Called by nvmalloc
    void AppendVarMapLog(void *start, size_t size, char *name, int pageNo, int pageOffset) {
        // Append record to the end of the varmap log
        char tmp[1024];
        struct varmap_entry varmap;
//      varmap.addr = (unsigned long)start;
        sprintf(varmap.name, "%s", name);
        varmap.size = size;
        varmap.pageNo = pageNo;
        varmap.pageOffset = pageOffset;
//      sprintf(tmp, "%s:%lx\n", varmap.name, varmap.addr);
        sprintf(tmp, "%s:%zu:%d:%d\n", varmap.name, varmap.size, varmap.pageNo, varmap.pageOffset);

        // Write log to varmap file
        if ( write(_varmap_fd, tmp, strlen(tmp)) == -1 ) {
            fprintf(stderr, "Error writing log to varmap file: %s\n", _varmap_filename);
            perror("write");
            abort();
        }

        fdatasync(_varmap_fd);
//      lprintf("Added variable address map record: %s\n", tmp);
    }

    // Delete line number of the current program entry in nvlib.crash
    void Delete_nvlibcrash_entry(void) {
        int line = 0;
        char tmp[FILENAME_MAX];
        char buf[FILENAME_MAX];
        sprintf(tmp, "%s.tmp", nvlib_crash);
        FILE *infp = fopen(nvlib_crash, "r");
        FILE *outfp = fopen(tmp, "w");
        // Copy lines except for the current entry
        while (fgets(buf, FILENAME_MAX, infp) != NULL) {
            if ( line != nvlib_linenum ) {
                fprintf(outfp, "%s", buf);
                lprintf("Copied %s from %s to %s\n", buf, nvlib_crash, tmp);
            } else {
                lprintf("Skip copying %s\n", buf);
            }
            line++;
        }
        fclose(infp);
        fclose(outfp);

        // Rename new file to nvmlib_crash
        rename(tmp, nvlib_crash);

        // Delete old file
        unlink(tmp);
    }

    void Delete_varmap(void) {

    }

    void Delete_memlog(void) {

    }

    // Lookup the NVID (=PID) in the crash record and return the line number in nvlib to the caller
    // An existing entry in nvlib.crash means the corresponding exe has crashed before
    int LookupCrashRecord(void) {
        bool crashed = false;
        char exe[FILENAME_MAX];
        int ret = 0;
        int line = 0;

        // Get absolute exe name
        ret = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if ( ret == -1 ) {
            fprintf(stderr, "readlink() error when lookup nvid.\n");
            abort();
        }
        exe[ret] = 0;
        lprintf("exe: %s\n", exe);

        // Lookup in crash record file at /tmp/nvlib.crash
        FILE *fp = fopen(nvlib_crash, "r+");

        // nvlib.crash doesn't exist, create a new one.
        if ( fp == NULL ) {
            lprintf("%s doesn't exist, create it now\n", nvlib_crash);
            fp = fopen(nvlib_crash, "w");
        }
        // Lookup absolute exe name in nvlib.crash
        else {
            char buf[FILENAME_MAX];
            while (fgets(buf, FILENAME_MAX, fp) != NULL) {
                char *token;
                token = strtok(buf, ", \n");
                // Found record, program has crashed before
                if ( strcmp(token, exe) == 0 ) {
                    crashed = true;
                    token = strtok(NULL, ", \n");
                    // Read old NVID
                    nvid = atoi(token);
                    break;
                }
                line++;
            }
        }
        // Record line number for the entry
        nvlib_linenum = line;

        if ( crashed ) {
            lprintf("%s CRASHED before, Found nvid at line %d in nvlib.crash: %d\n", exe, nvlib_linenum, nvid);
        } else {
            // Generate a new NVID
            nvid = rand();
            lprintf("%s did not crash in previous execution\n", exe);
            fprintf(fp, "%s, %d\n", exe, nvid);
            line = -1;
        }
        fclose(fp);

        // Set up log path after we get nvid
        if ( log_dest == SSD ) {
            sprintf(logPath, "/mnt/ssd2/tmp/%d/", nvid);
        } else if ( log_dest == NVM_RAMDISK ) {
            sprintf(logPath, "/mnt/ramdisk/nvthreads/%d/", nvid);
            sprintf(memLogPath, "/mnt/ramdisk/nvthreads/%d/logs", nvid);
        }

        lprintf("Assigned NVID: %d at line %d to current process\n", nvid, nvlib_linenum);
        return line;
    }

    // Set crashed flag to true if we can find a crash record in nvlib.crash
    void SetCrashedFlag(void) {
        bool ret = false;
        int line;

        // Lookup the line number that contains current exe.
        // line >= 0 means there exists an entry in nvlib.crash
        line = LookupCrashRecord();
        if ( line >= 0 ) {
            lprintf("Your program CRASHED before.  Please recover your progress using libnvthread API\n");
            crashed = true;
        } else {
            lprintf("Your program did not crash before.  Continue normal execution\n");
            CreateLogPath();
            crashed = false;
        }
    }

    // Return the flag indicating whether the current program crashed before
    bool isCrashed(void) {
        return crashed;
    }

    // Collect the file names that match pattern
    void CollectFiles(char *pattern, const char *path, std::vector<std::string> *container) {
        struct dirent *ent;
        
        // Open directory, will be closed in finalize()
        if ( logdir == NULL ) {
            logdir = opendir(path);
        }

//      lprintf("looking for file with pattern: %s\n", pattern);
        if ( logdir != NULL ) {
            rewinddir(logdir);
            while ((ent = readdir(logdir)) != NULL) {
//              std::string *str = new std::string(ent->d_name);
                std::string str (ent->d_name);
                std::size_t found = str.find(pattern);

//              lprintf("checking file: %s\n", str.c_str());

                // Found match
                if ( found != std::string::npos ) {
                    // Variable mapping
                    if ( strcmp(pattern, "varmap") == 0 ) {
                        container->push_back(str);
//                      lprintf("Push back variable mapping: %s\n", str.c_str());
                    } 
                    // All memory log
                    else if ( strcmp(pattern, "MemLog") == 0 ) {
                        container->push_back(str);
//                      lprintf("Push back memory log: %s\n", str.c_str());
                    }
                    // Memory log of a PID with GID (w/ different LID)
                    else {
                        container->push_back(str);
//                      lprintf("Push back memory log: %s\n", str.c_str());
                    }
                }
            }
        }
        else{
            lprintf("empty directory: %s\n", logPath);
        }
    }

    // Build the variable address hash map for later query
    void BuildVarMap(void) {
        for (std::vector<std::string>::iterator it = varmap_file_vector->begin(); it != varmap_file_vector->end(); ++it) {
            char fn[256];
            char *name;
            char *addr;
            std::string line;
            sprintf(fn, "%s%s", logPath, (*it).c_str());

            std::ifstream ifs(fn);
            while (getline(ifs, line)) {
                name = strtok((char *)line.c_str(), ":");
                addr = strtok(NULL, ":");
//              lprintf("inserting %s at %s\n", name, addr);
                varmap->insert(std::make_pair((std::string)name, (unsigned long)strtoul(addr, NULL, 16)));
            }
        }
    }

    // Create the hash map for heap (only!) variable and address.
    // Multiple threads will have the same address for the same
    // object.
    void CreateVariableMap(void) {
//      lprintf("Building variable hash map for variables\n");
        CollectFiles((char *)"varmap", logPath, varmap_file_vector);
        BuildVarMap();
//      DumpVarMap();
    }

    // Put all MemLog files into a container and CreateMemlogMap() will
    // put them into containers for future fast lookup
    void CollectMemlogFileNames(void) {
        CollectFiles((char *)"MemLog", logPath, mempage_file_vector);
    }

    // Get a list of PID, GID, LID from all the memory logs
    void CreateMemlogMap(void){
        // Get all the memory log file names
        CollectMemlogFileNames();
        
        for (std::vector<std::string>::iterator it = mempage_file_vector->begin(); it != mempage_file_vector->end(); ++it) {
            char *token;
            char buf[FILENAME_MAX];
            unsigned long PID;
            unsigned long GID;
            unsigned long LID;
            
            // Parse PID and GID, file name format: MemLog_PID_GobalTransID_LocalTransID_random
            strcpy(buf, (*it).c_str());
//          lprintf("memory log: %s\n", buf);
            token = strtok(buf, "_"); //MemLog
            token = strtok(NULL, "_");  // PID
            PID = (unsigned long)strtoul(token, NULL, 10);
            token = strtok(NULL, "_");  // Global transaction ID
            GID = (unsigned long)strtoul(token, NULL, 10);
            token = strtok(NULL, "_");  // Local transaction ID
            LID = (unsigned long)strtoul(token, NULL, 10);
//          lprintf("PID: %lu, GID: %lu, LID: %lu\n", PID, GID, LID);

            // Collect unique PID and GID, LocatePage() will later use it to quickly locate the latest memory log
            std::pair<std::map<unsigned long, unsigned long>::iterator, bool> ret;
            ret = PIDs->insert(std::pair<unsigned long, unsigned long>(PID, PID));            
            if ( ret.second == false ) {    
                // PID exists already
//              lprintf("PID %lu already exists, skip inserting to unqie PIDs list\n", PID);
            }
            ret = GIDs->insert(std::pair<unsigned long, unsigned long>(GID, GID));
            if ( ret.second == false ) {
                // GID exists already
//              lprintf("GID %lu already exists, skip inserting to unique GIDs list\n", GID);
            }
        }
    }

    void DumpVarMap(void) {
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

    // Copy data from memory log to dest (if addr exists in memlog)
    // Return 0 if successful, -1 if data aren't copied to dest
    // Memlog Log entry format (one log may contain multiple entries):
    // ----------------------------------------------
    // | page addr |   data stored in memory page   | 
    // ----------------------------------------------
    // addr: 8 bytes; data: 4096 bytes, no space/symbol in between
    int CopyFromMemlogToPtr(const char *memlog, void *dest, unsigned long addr, size_t size){
        int rv = -1;
        int fd;
        off_t fsize;
        char *ptr;
        size_t offset = 0;
        int EOFflag_bytes = 4;
        size_t addr_bytes = sizeof(void*);
        unsigned long nentry = 0;
         
        // Set pointer to the start of a memory page
        fd = open(memlog, O_RDONLY);
        if ( fd == -1 ) {
            lprintf("can't read memory page: %s\n", memlog);
            abort();
        }

        // TODO: check whether file is corrupted: 
        // (EOF flag should be at the end of the log)

        // Compute file size
        fsize = lseek(fd, 0, SEEK_END) - EOFflag_bytes;
        lseek(fd, 0, SEEK_SET);

        // Map memory log file to process address space and read later
        ptr = (char *)mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
        if ( ptr == MAP_FAILED ) {
            printf("can't mmap memory page %s\n", memlog);
            abort();
        }  

        nentry = fsize / (addr_bytes + xdefines::PageSize);
        unsigned long i;
        
//      lprintf("checking memory log %s size %zu with %lu entries for addr 0x%08lx\n", memlog, fsize, nentry, addr);
        while (i != nentry) {
            // Read address of the memory page
            unsigned long paddr;
            memcpy(&paddr, (char *)ptr, sizeof(void*));
//          lprintf("checking entry %lu/%lu (MemLog): paddr 0x%08lx, (Request): addr  0x%08lx\n", i, nentry, paddr, addr);
            ptr = ptr + addr_bytes; // move ptr to the start of the page address in log            

            // Check if address of this entry matches the addr we are searching
            if ( (addr & ~LogDefines::PAGE_SIZE_MASK) == paddr ) {
                // Found memory page, calculate relative offset
                // This is necessary because the destination may not be page aligned
//              lprintf("located memory page: 0x%08lx in memlog: %s\n", paddr, memlog);
                if ( paddr == addr ) {
//                  lprintf("page aligned! paddr (log) 0x%08lx, addr (request) 0x%08lx\n", paddr, addr);
                    offset = 0;
                } else if ( paddr > addr ) {
                    offset = paddr - addr;
                } else {
                    offset = addr - paddr;
                }

                // Copy data
                if ( size >= xdefines::PageSize ) {
                    memcpy((char *)dest, (char *)(ptr + offset), xdefines::PageSize - offset);
//                  lprintf("Memcpy starting from 0x%08lx, offset 0x%lx, size %zu bytes, dest: %p\n",
//                          paddr, offset, xdefines::PageSize - offset, dest);
                } else {
                    // Data smaller than page size
                    memcpy((char *)dest, (char *)(ptr + offset), size);
//                  lprintf("Memcpy starting from 0x%08lx, offset 0x%lx, size %zu bytes, dest: %p\n",
//                          paddr, offset, size, dest);
                }
                

//              lprintf("ptr %p\n", ptr);
                close(fd);
                return 0;
            }
            
            i++;
            ptr = ptr + xdefines::PageSize;
        }
        
        close(fd);
        return -1;
    }

    // Copy the latest memory page and return 0 if successfull, -1 if failed
    int CopyOnePage(void *dest, unsigned long addr, size_t size){
        std::map<unsigned long, unsigned long>::reverse_iterator rito;
        std::map<unsigned long, unsigned long>::reverse_iterator riti;
        std::vector<std::string> tmp_mempage_file_vector; // Store a list of temporary memory logs
        std::vector<std::string>::iterator rit;
        char memlog[FILENAME_MAX];
        unsigned long maxGID = 0;

        // Find memory log 
        // From the last PID to the first PID
//      lprintf("Copying one page for 0x%08lx\n", addr);
        for (rito = PIDs->rbegin(); rito != PIDs->rend(); ++rito) {
            // From the last GID to the first GID
//          lprintf("Searching PID: %lu\n", rito->first);
            bool found = false;
            for (riti = GIDs->rbegin(); riti != GIDs->rend(); ++riti) {        
                // Only check memory log if current GID is greater than the previously found GID
                if ( riti->first > maxGID ) {
//                  lprintf("Searching ThreadID %lu GlobalTransactionID: %lu, current max GID %lu\n", rito->first, riti->first, maxGID);
                    sprintf(memlog, "MemLog_%lu_%lu_", rito->first, riti->first);

                    // From the last LID to the first LID
                    tmp_mempage_file_vector.clear();                    
                    CollectFiles(memlog, memLogPath, &tmp_mempage_file_vector);                    
                    for (rit = tmp_mempage_file_vector.begin(); rit != tmp_mempage_file_vector.end(); ++rit) {                    
                        sprintf(memlog, "%s%s", memLogPath, (*rit).c_str());                                    
                        
                        // Copy data from memory log to dest if the log contains addr
                        if ( CopyFromMemlogToPtr(memlog, dest, addr, size) == 0){ 
//                          lprintf("update maxGID from %lu to %lu\n", maxGID, riti->first);
                            maxGID = riti->first;
//                          lprintf("succefully copied one page from memlog %s for addr 0x%08lx, max GID %lu\n", memlog, addr, maxGID);
                            found = true;
                            // TODO: quick hack for kmeans!!!!!!!!!!!!!!!!!!!!!!!!!!
                            return 0;
                        }
                    }
                }
                if ( found || riti->first < maxGID) {
                    // Skip checking the older memory logs
//                  lprintf("Skip checking the remaining older memory logs\n");
                    break;
                }
            }
        }
//      lprintf("Copied one page for 0x%08lx\n", addr);
            
        // Found page and copied data
        if ( maxGID != 0 ) {
            return 0;
        }

        return -1;
    }               


   // FIXME: quick hack for kmeans!!!!!!!!!!!!!
   int CopyPagesFromMemlogToPtr(const char *memlog, void *dest, unsigned long addr, size_t size, unsigned long npages){
        int rv = -1;
        int fd;
        off_t fsize;
        char *ptr;
        size_t offset = 0;
        int EOFflag_bytes = 4;
        size_t addr_bytes = sizeof(void*);
        unsigned long nentry = 0;
        int copied = 0;
         
        // Set pointer to the start of a memory page
        fd = open(memlog, O_RDONLY);
        if ( fd == -1 ) {
            lprintf("can't read memory page: %s\n", memlog);
            abort();
        }

        // TODO: check whether file is corrupted: 
        // (EOF flag should be at the end of the log)

        // Compute file size
        fsize = lseek(fd, 0, SEEK_END) - EOFflag_bytes;
        lseek(fd, 0, SEEK_SET);

        // Map memory log file to process address space and read later
        ptr = (char *)mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
        if ( ptr == MAP_FAILED ) {
            printf("can't mmap memory page %s\n", memlog);
            abort();
        }  

        nentry = fsize / (addr_bytes + xdefines::PageSize);
        unsigned long i;
        
//      lprintf("checking memory log %s size %zu with %lu entries for addr 0x%08lx\n", memlog, fsize, nentry, addr);
        while (i != nentry) {
            // Read address of the memory page
            unsigned long paddr;
            memcpy(&paddr, (char *)ptr, sizeof(void*));
//          lprintf("checking entry %lu/%lu (MemLog): paddr 0x%08lx, (Request): addr  0x%08lx\n", i, nentry, paddr, addr);
            ptr = ptr + addr_bytes; // move ptr to the start of the page address in log            

            // Check if address of this entry matches the addr we are searching
            if ( (addr & ~LogDefines::PAGE_SIZE_MASK) == paddr ) {
                // Found memory page, calculate relative offset
                // This is necessary because the destination may not be page aligned
//              lprintf("located memory page: 0x%08lx in memlog: %s\n", paddr, memlog);
                if ( paddr == addr ) {
//                  lprintf("page aligned! paddr (log) 0x%08lx, addr (request) 0x%08lx\n", paddr, addr);
                    offset = 0;
                } else if ( paddr > addr ) {
                    offset = paddr - addr;
                } else {
                    offset = addr - paddr;
                }

                // Copy data
                if ( size >= xdefines::PageSize ) {
                    memcpy((char *)dest, (char *)(ptr + offset), xdefines::PageSize - offset);
//                  lprintf("Memcpy starting from 0x%08lx, offset 0x%lx, size %zu bytes, dest: %p, copied: %d\n",
//                  paddr, offset, xdefines::PageSize - offset, dest, copied);
                } else {
                    // Data smaller than page size
                    memcpy((char *)dest, (char *)(ptr + offset), size);
//                  lprintf("Memcpy starting from 0x%08lx, offset 0x%lx, size %zu bytes, dest: %p, copied: %d\n",
//                          paddr, offset, size, dest, copied);
                }
                
                // Page alignment after the first run
                addr = addr + xdefines::PageSize;
                addr = addr & ~LogDefines::PAGE_SIZE_MASK;
                dest = (char*)dest + xdefines::PageSize;
                dest = (char*)((unsigned long)dest & ~LogDefines::PAGE_SIZE_MASK);                
                copied++;
//              lprintf("ptr %p\n", ptr);
                if ( npages == copied ) {
                    close(fd);
                    return copied;
                }
            }
            
            i++;
            ptr = ptr + xdefines::PageSize;
        }
        
        close(fd);
        return copied;
    }

    // FIXME: QUICK HACK FOR KMEANS to quickly copy N pages instead going one page after another (slow!!)
    // Correct way is to design a lookup table to quickly locate the pages 
    int CopyPages(char *dest, unsigned long addr, size_t size, unsigned long napges){
        int copied = 0;
        std::map<unsigned long, unsigned long>::reverse_iterator rito;
        std::map<unsigned long, unsigned long>::reverse_iterator riti;
        std::vector<std::string> tmp_mempage_file_vector; // Store a list of temporary memory logs
        std::vector<std::string>::iterator rit;
        char memlog[FILENAME_MAX];
        unsigned long maxGID = 0;

        // Find memory log 
        // From the last PID to the first PID
//      lprintf("Copying one page for 0x%08lx\n", addr);
        for (rito = PIDs->rbegin(); rito != PIDs->rend(); ++rito) {
            // From the last GID to the first GID
            bool found = false;
            for (riti = GIDs->rbegin(); riti != GIDs->rend(); ++riti) {        
                // Only check memory log if current GID is greater than the previously found GID
                if ( riti->first > maxGID ) {
//                  lprintf("Searching ThreadID %lu GlobalTransactionID: %lu, current max GID %lu\n", rito->first, riti->first, maxGID);
                    sprintf(memlog, "MemLog_%lu_%lu_", rito->first, riti->first);

                    // From the last LID to the first LID
                    tmp_mempage_file_vector.clear();                    
                    CollectFiles(memlog, memLogPath, &tmp_mempage_file_vector);                    
                    for (rit = tmp_mempage_file_vector.begin(); rit != tmp_mempage_file_vector.end(); ++rit) {                    
                        sprintf(memlog, "%s%s", memLogPath, (*rit).c_str());                                    
                        
                        // Copy data from memory log to dest if the log contains addr
                        copied = CopyPagesFromMemlogToPtr(memlog, dest, addr, size, napges);
                        if ( copied > 0){ 
//                          lprintf("update maxGID from %lu to %lu\n", maxGID, riti->first);
                            maxGID = riti->first;
//                          lprintf("succefully copied one page from memlog %s for addr 0x%08lx, max GID %lu\n", memlog, addr, maxGID);
                            found = true;
                            // TODO: quick hack for kmeans!!!!!!!!!!!!!!!!!!!!!!!!!!
                            return copied;
                        }
                    }
                }
                if ( found || riti->first < maxGID) {
                    // Skip checking the older memory logs
//                  lprintf("Skip checking the remaining older memory logs\n");
                    break;
                }
            }
        }
//      lprintf("Copied one page for 0x%08lx\n", addr);
            
        // Found page and copied data
        if ( maxGID != 0 ) {
            return 0;
        }
//      lprintf("page not found for 0x%08lx\n", addr);
        return 0;
    }

    // Copy memory pages from the memory log and return the actual number of pages (checked + copied)
    int CopyPagesFromMemLog(char *dest, unsigned long addr, unsigned long npages, size_t size){
        unsigned long handled_this_run = 0;
        unsigned long copied = 0;
        unsigned long checked = 0;
//      lprintf("Copying %lu pages\n", npages);
        while ( copied < npages ) {      
            handled_this_run = 0;      
            // FIXME: Quick hack for kmeans
            if ( npages > 1 ) {
                handled_this_run = CopyPages(dest, addr, size, npages - copied);
                copied += handled_this_run;
//              lprintf("Copied %lu pages starting at 0x%08lx in one go\n\n", copied, addr);
            }
            // Copy one page at a time
            else if ( CopyOnePage(dest, addr, size) == 0 ) {
                handled_this_run = 1;
                copied++;
//              lprintf("Copied page %lu-th page starting at 0x%08lx\n\n", copied, addr);
            }
            else{
                handled_this_run = 1;
                checked++;
//              lprintf("addr 0x%08lx is not dirtied so no record found\n", addr);
            }

            // Page alignment after the first run
            addr = addr + (xdefines::PageSize * handled_this_run);
            addr = addr & ~LogDefines::PAGE_SIZE_MASK;
            dest = dest + (xdefines::PageSize * handled_this_run);
            dest = (char*)((unsigned long)dest & ~LogDefines::PAGE_SIZE_MASK);
//          lprintf("dest %p, addr 0x%08lx\n", dest, addr);
        }
        return (copied+checked);
    }

    // Computer number of pages needed for copying data.
    // Note that addr may not be page aligned.
    // Therefore we need to make sure that, for example, 4KB data that span multiple pages 
    // will need 2 pages instead of 1 to copy the data
    unsigned long ComputeNumOfPagesNeeded(unsigned long addr, size_t size){
        if ( addr & 0x7 ) {
            // Addr is page aligned
//          lprintf("addr 0x%08lx is page aligned\n", addr);
            return size / xdefines::PageSize;            
        }
        else{
            // Addr is not page aligned        
//          lprintf("addr 0x%08lx is not page aligned\n", addr);
            return size / xdefines::PageSize + 1;
        }
    }

    // Return the address of the variable in nvm
    unsigned long RecoverVarAddr(void *dest, size_t size, char *name) {
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
//      lprintf("Found %s at 0x%08lx\n", name, addr);

        return addr;
    }

    void RecoverLookup(bool isHeap){
        char _lookupFname[1024];
        struct lookupinfo *_pageLookup;
                
        if ( isHeap ) {
            sprintf(_lookupFname, "%slookup_heap_recover", logPath);
//          if ( _pageLookupHeap ) {
//              lprintf("_pageLookupHeap is already set %p, return\n", _pageLookupHeap);
//              dumpLookup(true);
//              return;
//          }
        }
        else{
            sprintf(_lookupFname, "%slookup_globals_recover", logPath);
//          if ( _pageLookupGlobals ) {
//              lprintf("_pageLookupGlobals is already set %p, return\n", _pageLookupGlobals);
//              dumpLookup(false);
//              return;
//          }
        }

        // Open lookup record stored in file
        int _lookupFd = open(_lookupFname, O_RDONLY, 0644);
        if ( _lookupFd == -1 ) {
            fprintf(stderr, "Failed to make persistent file at %s.\n", _lookupFname);
            perror("open failed: ");
            ::abort();
        }

        // Get file size then mmap lookup record file
        off_t fsize = lseek(_lookupFd, 0, SEEK_END);
        lseek(_lookupFd, 0, SEEK_SET);       
        _pageLookup = (struct lookupinfo *)mmap(NULL, fsize,
                                              PROT_READ, MAP_SHARED, _lookupFd, 0);

        if ( _pageLookup == MAP_FAILED){
            perror("mmap failed for _pageLookup");
            abort();
        }

        // Set heap/globals pointer
        if ( isHeap ) {
            _pageLookupHeap = _pageLookup;
            numPagesHeap = fsize / sizeof(struct lookupinfo);
        }
        else {
            _pageLookupGlobals = _pageLookup;
            numPagesGlobals = fsize / sizeof(struct lookupinfo);        
        }

//      lprintf("recovered from file: %s, size: %zu, _pageLookup: %p\n", _lookupFname, fsize, _pageLookup);
        close(_lookupFd);

//      dumpLookup(true);
    }

    // Read the variable mapping info from file
    void RecoverVarmap(void){
        char varmapFname[1024];
        char *token;

        if ( recoveredVarmap == NULL ) {
            recoveredVarmap = new std::map<std::string, struct varmap_entry>;
        }
        if ( recoveredVarmap->size() != 0 ) {
//          lprintf("recoveredVarmap already set with %zu records\n", recoveredVarmap->size());
            return;
        }

        sprintf(varmapFname, "%svarmap_recover", logPath);
        std::string line;        
        std::ifstream ifs(varmapFname);
        while (getline(ifs, line)) {
            struct varmap_entry v;
            
            // Name
            token = strtok((char *)line.c_str(), ":");
            sprintf(v.name, "%s", token);

            // Size
            token = strtok(NULL, ":");
            v.size = (size_t)atoi(token);

            // pageNo
            token = strtok(NULL, ":");
            v.pageNo = atoi(token);

            // pageOffset
            token = strtok(NULL, ":");
            v.pageOffset = atoi(token);

//          lprintf("inserting %s size %zu at pageNo: %d pageOffset: %d\n", v.name, v.size, v.pageNo, v.pageOffset);
            recoveredVarmap->insert(std::make_pair((std::string)v.name, v));
        }
        
    }

    // Return how many bytes are checked/copied
    int RecoverOnePage(void *dest, struct varmap_entry *v, size_t remaining_bytes, size_t total_size, int pagecount){
        size_t bytes = 0;
        char memlogFn[1024];
        int memlogFd;
        int pageNo = v->pageNo + pagecount; // calculate the correct pageNo        
        int pageOffset;
        unsigned short xactID = _pageLookupHeap[pageNo].xactID;
        unsigned short threadID = _pageLookupHeap[pageNo].threadID;
        unsigned long memlogOffset = _pageLookupHeap[pageNo].memlogOffset;

        lprintf("copying pageNo %d, memlogOffset: %lu\n", pageNo, memlogOffset);

        // First page, take care of offset to prevent overflow
        if ( remaining_bytes == total_size ) {
            // calculate offset within a page
            pageOffset = v->pageOffset;

            // Only need 1 page
            if ( pageOffset + total_size <= xdefines::PageSize ) {
                bytes = total_size;
                lprintf("First page (only need 1 page), starting from pageOffset: %d, copy %zu bytes\n", pageOffset, bytes);
            }
            // Need more than 1 page
            else{
                bytes = xdefines::PageSize - pageOffset;
                lprintf("First page (need more than 1 page), starting from pageOffset: %d, copy %zu bytes\n", pageOffset, bytes);
            }
        }
        else{
            // Last page, take care of #bytes to copy to prevent overflow
            pageOffset = 0;
            if (remaining_bytes < xdefines::PageSize){
                bytes = remaining_bytes;
                lprintf("Last page, starting from pageOffset: 0, copy %zu bytes\n", bytes);
            }
            else{
                bytes = xdefines::PageSize;
                lprintf("Middle page, starting from pageOffset: 0, copy %zu bytes\n", bytes);
            }
        }
        lprintf("memlogOffset: %lu, pageOffset: %d\n", memlogOffset, pageOffset);

        if ( _pageLookupHeap[pageNo].dirtied ) {

            // Get the file name of memory log
            sprintf(memlogFn, "%sMemLog_%d_%d", memLogPath, threadID, xactID);
            memlogFd = open(memlogFn, O_RDONLY);
            if ( memlogFd == -1 ) {
                perror("RecoverOnePage open()");
                abort();
            }    

            // Move file descriptor to correct offset
            memlogOffset += pageOffset;
            lseek(memlogFd, memlogOffset, SEEK_SET);
            size_t sz;

            // Recover data from page
            sz = read(memlogFd, dest, bytes);
            if ( sz != bytes ) {
                lprintf("Error: copy only %zu bytes, should've copied %zu bytes\n", sz, bytes);
            }
            close(memlogFd);
            lprintf("copied %zu bytes for pageNo %d from %s, memlogOffset: %lu\n", sz, pageNo, memlogFn, memlogOffset);
        }
        else{
            lprintf("pageNo %d is not dritied, checked %zu bytes, skip recoverying this page\n", pageNo, bytes);
        }

        return bytes;
    }

    // return number of bytes checked
    size_t RecoverDataFromMemlog(char *dest, struct varmap_entry *v, size_t size){
        unsigned long xactID = 0;
        unsigned long threadID = 0;
        unsigned long memlogOffset = 0;
        int i;
        size_t bytes = 0;   // number of bytes checked
        size_t rv = 0;
        int pagecount = 0;
        while ( bytes != size ) {
            rv = RecoverOnePage(dest, v, size - bytes, size, pagecount);
            bytes += rv;

//          lprintf("dest: %p, checked %zu bytes\n", dest, bytes);

            // Advance page aligned address
            dest = dest + xdefines::PageSize;
            dest = (char*)((unsigned long)dest & ~LogDefines::PAGE_SIZE_MASK);
            pagecount++;
        }
        return bytes;
    }


    struct varmap_entry *RecoverVarmapInfo(char *name){
        struct varmap_entry *v;

        // Look for variable name
        std::map<std::string, struct varmap_entry>::iterator it;
        it = recoveredVarmap->find((std::string)name);
        if ( it == recoveredVarmap->end() ) {
            lprintf("Cannot find %s\n", name);
            return NULL;
        }

        // Found variable mapping
        v = &it->second;
        lprintf("Found %s with size %zu at pageNo %d pageOffset %d\n", v->name, v->size, v->pageNo, v->pageOffset);

        return v;
    }

    // Computer number of pages needed for copying data.
    // Note that addr may not be page aligned.
    // Therefore we need to make sure that, for example, 4KB data that span multiple pages 
    // will need 2 pages instead of 1 to copy the data    
    int ComuptePagesNeeded(int offset, size_t size){
        int npages = 0;
        // Page aligned
        if ( offset == 0 ) {
            npages = size / xdefines::PageSize;
        }
        // Not page aligned, consider offset
        else {
            // No page aligned but object stay within one page
            if ( offset + size < xdefines::PageSize ) {
                npages = 1;
            }
            // No page aligned and object span pages
            else {
                npages = (size / xdefines::PageSize) + 1;
            }
        }

        lprintf("offset: %d, size: %zu, napges: %d\n", offset, size, npages);
        return npages;
    }

    unsigned long nvrecover(void *dest, size_t size, char *name) {
        size_t bytes_checked;

        if ( _pageLookupHeap == NULL ) {
            // Recover quick lookup info
            RecoverLookup(true);
        }
        if ( recoveredVarmap == NULL ) {
            // Recover variable mapping info
            RecoverVarmap();
        }
//      lprintf("Hint: 0x%p, size: %zu, name: %s\n", dest, size, name);
        // Find the address of the variable in varmap log
        unsigned long addr;
        struct varmap_entry *v = RecoverVarmapInfo(name);
        if ( !v ) {
            lprintf("Error, can't find variable named %s\n", name);
            return 0;
        }

        // Calculate number of pages needed for copying data 
//      unsigned long npages = ComputeNumOfPagesNeeded(addr, size);
//      int npages = ComuptePagesNeeded(v->pageOffset, size);
//      if ( npages <= 0 ) {
//          lprintf("Error, can't recover variable %s with size 0\n", name);
//          return 0;
//      }

        // Copy data from memory pages to destination
//      rv = CopyPagesFromMemLog((char*)dest, addr, npages, size);
        bytes_checked= RecoverDataFromMemlog((char*)dest, v, size);
        if ( bytes_checked != size ) {
            lprintf("Error, should've checked %zu bytes, but instead checked %zu bytes for variable named %s\n", size, bytes_checked, name);
            return 0;            
        }

        // Find the data by address in memory pages
        lprintf("nvrecover-ed %s at old addr 0x%08lx\n", name, addr);
        
        return addr;
    }

    void dumpLookup(bool isHeap){
        struct lookupinfo *_pageLookup;
        unsigned long numPages;
//      lprintf("dumping lookup isHeap: %d\n", isHeap);
        if ( isHeap ) {
            _pageLookup = _pageLookupHeap;
            numPages = numPagesHeap;
        }
        else{
            _pageLookup = _pageLookupGlobals;
            numPages = numPagesGlobals;
        }
        if ( _pageLookup == NULL ) {
            lprintf("Record is empty\n");
            return;
        }
        lprintf("dumping lookup record, npages: %lu\n", numPages);
        unsigned long i;
        for (i = 0; i < numPages; i++) {
            if ( _pageLookup[i].dirtied ) {
                lprintf("_pageLookup[%lu].xactID: %d\n", i, _pageLookup[i].xactID);
                lprintf("_pageLookup[%lu].threadID: %d\n", i, _pageLookup[i].threadID);
                lprintf("_pageLookup[%lu].memlogOffset: %lu\n", i, _pageLookup[i].memlogOffset);
            }
        }
    }

};

#endif
