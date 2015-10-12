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


/* Class for log entry to be appended to the end of per-thread MemoryLog */
class varmap_entry {
    enum {MaxVarNameLength = 1024};
public:
    struct varmap_t {
        unsigned long addr;
        char *name;
    };
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
    char log_path_prefix[FILENAME_MAX];
    bool crashed;
    bool _main_thread;
    bool _initialized;
    bool _logging_enabled;
    LOG_DEST log_dest;
    int threadID;

    /* For file mapped varmap log */
    FILE *_varmap_fptr;
    void *_varmap_ptr;
    int _varmap_fd;
    unsigned long _varmap_offset;
    char _varmap_filename[FILENAME_MAX];
    bool _first_entry;

    // Store a list of file names for memory pages
    std::vector<std::string> *mempage_file_vector;

    // Store a list of varmap file names
    std::vector<std::string> *varmap_file_vector;
    
    // Store a list of temporary memory logs
    std::vector<std::string> *tmp_mempage_file_vector;

    // Map of the actual variable and address mapping
    std::map<std::string, unsigned long> *varmap;
   
    // Unique list of PID
    std::map<unsigned long, unsigned long> *PIDs;

    // Unique list of GID
    std::map<unsigned long, unsigned long> *GIDs;

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

        lprintf("----------initializing nvrecovery class--------------\n");
        if ( !_initialized ) {
            srand(time(NULL));
            strcpy(nvlib_crash, "/tmp/nvlib.crash");

            // TODO: Read from config
//          log_dest = MemoryLog::log_dest;
            log_dest = DRAM_TMPFS;
            // Read the configuration file to decide where to log the
            
            mempage_file_vector = new std::vector<std::string>;
            tmp_mempage_file_vector = new std::vector<std::string>;
            varmap = new std::map<std::string, unsigned long>;
            varmap_file_vector = new std::vector<std::string>;
            PIDs = new std::map<unsigned long, unsigned long>;
            GIDs = new std::map<unsigned long, unsigned long>;

            // Main thread checks whether the program crashed before and sets the crash flag
            if ( _main_thread ) {
                SetCrashedFlag();
                if ( isCrashed() ) {
                    // Recover the variable and address mapping and memory pages
                    CreateVariableMap();

                    // Collect all the file names of the memory log files
                    CreateMemlogMap();
                }
            }

            // Create new VarMap file for current run
            OpenVarMap();
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
            Delete_nvlibcrash_entry();
            Delete_varmap();
            Delete_memlog();
            CloseVarMap(); 
        }
//      delete mempage_file_vector;
//      delete varmap_file_vector;
//      delete varmap;
    }

    // Create the path to store memory log and variable mapping
    void CreateLogPath(void) {  
        // Create direcotry for current nvid to save varmap and MemLog
        if ( (mkdir(log_path_prefix, 0777)) == -1 ) {
            perror("Error when mkdir(log_path_prefix)");
            abort();
        }
        lprintf("Created log path: %s\n", log_path_prefix);
    }

    // Open variable mapping file descriptor
    void OpenVarMap(void) {
        // Create a temp file for logging mapping between variable and address
        sprintf(_varmap_filename, "%svarmap_%d_XXXXXX", log_path_prefix, threadID);
        _varmap_fd = mkostemp(_varmap_filename, O_RDWR | O_ASYNC | O_APPEND);

        if ( _varmap_fd == -1 ) {
            fprintf(stderr, "%d: Error creating %s\n", getpid(), _varmap_filename);
            perror("mkostemp");
            abort();
        }
        lprintf("Opened varmap file at: %s\n", _varmap_filename);
        _varmap_offset = 0;
    }

    // Close variable mapping file descriptor and remove it
    void CloseVarMap(void) {
        close(_varmap_fd);
        unlink(_varmap_filename);
        lprintf("Closed %s\n", _varmap_filename);
    }

    // Add a entry for variable mapping. Called by nvmalloc
    void AppendVarMapLog(void *start, size_t size, char *name) {
        // Append record to the end of the varmap log
        char tmp[strlen(name) + sizeof(unsigned long)];
        varmap_entry::varmap_t varmap;
        varmap.addr = (unsigned long)start;
        varmap.name = name;
        sprintf(tmp, "%s:%lx\n", varmap.name, varmap.addr);

        if ( _first_entry ) {
            _first_entry = false;
        }

        // Write log to varmap file
        if ( write(_varmap_fd, tmp, strlen(tmp)) == -1 ) {
            fprintf(stderr, "Error writing log to varmap file: %s\n", _varmap_filename);
            perror("write");
            abort();
        }
        _varmap_offset = _varmap_offset + strlen(tmp);

        lprintf("Added variable address map record: %s\n", tmp);
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
            lprintf("%s crashed before, Found nvid at line %d in nvlib.crash: %d\n", exe, nvlib_linenum, nvid);
        } else {
            // Generate a new NVID
            nvid = rand();
            lprintf("%s didn't crash in previous execution\n", exe);
            fprintf(fp, "%s, %d\n", exe, nvid);
            line = -1;
        }
        fclose(fp);

        // Set up log path after we get nvid
        if ( log_dest == DISK ) {
            sprintf(log_path_prefix, "./");
        } else if ( log_dest == DRAM_TMPFS ) {
            sprintf(log_path_prefix, "/mnt/ramdisk/%d/", nvid);
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
            lprintf("Your program crashed before.  Please recover your progress using libnvthread API\n");
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
        DIR *dir;
        struct dirent *ent;
//      lprintf("looking for file with pattern: %s\n", pattern);
        if ( (dir = opendir(path)) != NULL ) {
            while ((ent = readdir(dir)) != NULL) {
                std::string *str = new std::string(ent->d_name);
                std::size_t found = str->find(pattern);

//              lprintf("checking file: %s\n", str->c_str());

                // Found match
                if ( found != std::string::npos ) {
                    // Variable mapping
                    if ( strcmp(pattern, "varmap") == 0 ) {
                        container->push_back(*str);
                        lprintf("Push back variable mapping: %s\n", str->c_str());
                    } 
                    // All memory log
                    else if ( strcmp(pattern, "MemLog") == 0 ) {
                        container->push_back(*str);
                        lprintf("Push back memory log: %s\n", str->c_str());
                    }
                    // Memory log of a PID with GID (w/ different LID)
                    else {
                        container->push_back(*str);
                        lprintf("Push back memory log: %s\n", str->c_str());                        
                    }
                }
            }
//          closedir(dir);
        }
        else{
            lprintf("empty directory: %s\n", log_path_prefix); 
        }
    }

    // Build the variable address hash map for later query
    void BuildVarMap(void) {
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
    void CreateVariableMap(void) {
        lprintf("Building variable hash map for variables\n");
        CollectFiles((char *)"varmap", log_path_prefix, varmap_file_vector);
        BuildVarMap();
        DumpVarMap();
        lprintf("Done with hash map.\n");
    }

    // Put all MemLog files into a container and CreateMemlogMap() will
    // put them into containers for future fast lookup
    void CollectMemlogFileNames(void) {
        CollectFiles((char *)"MemLog", log_path_prefix, mempage_file_vector);
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
            lprintf("memory log: %s\n", buf);
            token = strtok(buf, "_"); //MemLog
            token = strtok(NULL, "_");  // PID
            PID = (unsigned long)strtoul(token, NULL, 10);
            token = strtok(NULL, "_");  // Global transaction ID
            GID = (unsigned long)strtoul(token, NULL, 10);
            token = strtok(NULL, "_");  // Local transaction ID
            LID = (unsigned long)strtoul(token, NULL, 10);
            lprintf("PID: %lu, GID: %lu, LID: %lu\n", PID, GID, LID);

            // Collect unique PID and GID, LocatePage() will later use it to quickly locate the latest memory log
            std::pair<std::map<unsigned long, unsigned long>::iterator, bool> ret;
            ret = PIDs->insert(std::pair<unsigned long, unsigned long>(PID, PID));            
            if ( ret.second == false ) {    
                // PID exists already
                lprintf("PID %lu already exists, skip inserting to unqie PIDs list\n", PID);
            }
            ret = GIDs->insert(std::pair<unsigned long, unsigned long>(GID, GID));
            if ( ret.second == false ) {
                // GID exists already
                lprintf("GID %lu already exists, skip inserting to unique GIDs list\n", GID);                
            }

            // Restore pointer changed by strtok
            lprintf("after: memory log: %s\n", (*it).c_str());
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
    int CopyFromMemlogToPtr(const char *memlog, void *dest, unsigned long addr){
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
        lprintf("memory log %s size %zu\n", memlog, fsize);

        nentry = fsize / (addr_bytes + xdefines::PageSize);
        unsigned long i;
        while (i != nentry) {
            // Read address of the memory page
            unsigned long paddr;
            memcpy(&paddr, (char *)ptr, sizeof(void*));
            lprintf("checking entry %lu addr 0x%08lx\n", i, paddr);
            ptr = ptr + addr_bytes;            

            // Check if address of this entry matches the addr we are searching
            if ( (addr & ~LogDefines::PAGE_SIZE_MASK) == paddr ) {
                // Found memory page, calculate relative offset
                // This is necessary because the destination may not be page aligned
                lprintf("located memory page: 0x%08lx\n", paddr);
                if ( paddr > addr ) {
                    offset = paddr - addr;
                } else {
                    offset = addr - paddr;
                }

                // Copy data
                memcpy((char *)dest, (char *)(ptr+offset), xdefines::PageSize);
                lprintf("Memcpy starting from 0x%08lx, offset %lx\n", paddr, offset);
                lprintf("ptr %p\n", ptr);

                return 0;
            }
            i++;
            ptr = ptr + xdefines::PageSize;
        }
        return -1;
    }

    // Copy the latest memory page and return 0 if successfull, -1 if failed
    int CopyOnePage(void *dest, unsigned long addr){
        std::map<unsigned long, unsigned long>::reverse_iterator rito;
        std::map<unsigned long, unsigned long>::reverse_iterator riti;
        std::vector<std::string>::iterator rit;
        char memlog[FILENAME_MAX];

        // Find memory log 
        // From the last PID to the first PID
        for (rito = PIDs->rbegin(); rito != PIDs->rend(); ++rito) {
            // From the last GID to the first GID
            for (riti = GIDs->rbegin(); riti != GIDs->rend(); ++riti) {
                lprintf("Searching PID: %lu, GID: %lu\n", rito->first, riti->first);
                sprintf(memlog, "MemLog_%lu_%lu_", rito->first, riti->first);
        
                // From the last LID to the first LID    
                CollectFiles(memlog, log_path_prefix, tmp_mempage_file_vector);
                for (rit = tmp_mempage_file_vector->begin(); rit != tmp_mempage_file_vector->end(); rit++) {                    
                    sprintf(memlog, "%s%s", log_path_prefix, (*rit).c_str());                                    
                    lprintf("memlog: %s\n", memlog);                    
                    
                    // Copy data from memory log to dest if the log contains addr
                    if ( CopyFromMemlogToPtr(memlog, dest, addr) == 0){
                        lprintf("succefully copied one page from memlog %s for addr 0x%08lx\n", memlog, addr);
                        return 0;
                    }
                }
                tmp_mempage_file_vector->clear();
            }
        }
    
        return -1;
    }               

    // Copy memory pages from the memory log and return the actual number of pages copied
    int CopyPagesFromMemLog(char *dest, unsigned long addr, unsigned long npages){
        unsigned long copied = 0;
        lprintf("copying %lu pages\n", npages);
        for (unsigned long i = 0; i < npages; i++) {
            if ( CopyOnePage(dest, addr) == 0 ) {
                copied++;
                lprintf("Copied page no. %lu starting at 0x%08lx\n", copied, addr);
            }
            else{
                lprintf("Error: only copied %lu page (out of %lu pages) starting at 0x%08lx\n", copied, npages, addr);
                return copied;
            }

            // Page alignment after the first run
            addr = addr + xdefines::PageSize;
            addr = addr & ~LogDefines::PAGE_SIZE_MASK;
            dest = dest + xdefines::PageSize;
            dest = (char*)((unsigned long)dest & ~LogDefines::PAGE_SIZE_MASK);
            lprintf("dest %p, addr 0x%08lx\n", dest, addr);
        }
        return copied;
    }

    // Computer number of pages needed for copying data.
    // Note that addr may not be page aligned.
    // Therefore we need to make sure that, for example, 4KB data that span multiple pages 
    // will need 2 pages instead of 1 to copy the data
    unsigned long ComputeNumOfPagesNeeded(unsigned long addr, size_t size){
        if ( addr & 0x7 ) {
            // Addr is page aligned
            lprintf("addr 0x%08lx is page aligned\n", addr);
            return size / xdefines::PageSize;            
        }
        else{
            // Addr is not page aligned        
            lprintf("addr 0x%08lx is not page aligned\n", addr);
            return size / xdefines::PageSize + 1;
        }

//      if ( size % xdefines::PageSize == 0 ) {
//          return size / xdefines::PageSize;
//      } else{
//          return size / xdefines::PageSize + 1;
//      }
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
        lprintf("Found %s at 0x%08lx\n", name, addr);

        return addr;
    }

    unsigned long nvrecover(void *dest, size_t size, char *name) {
        int rv;

        lprintf("Hint: 0x%p, size: %zu, name: %s\n", dest, size, name);
        // Find the address of the variable in varmap log
        unsigned long addr = RecoverVarAddr(dest, size, name);
        if ( !addr ) {
            lprintf("Error, can't find variable named %s\n", name);
            return 0;
        }

        // Calculate number of pages needed for copying data 
        unsigned long npages = ComputeNumOfPagesNeeded(addr, size);
        if ( npages <= 0 ) {
            lprintf("Error, can't recover variable %s with size 0\n", name);
            return 0;
        }

        // Copy data from memory pages to destination
        rv = CopyPagesFromMemLog((char*)dest, addr, npages);
        if ( rv != npages) {
            lprintf("Error, should've copied %lu pages, but instead copied %lu pages for variable named %s\n", name, npages, rv);
            return 0;            
        }

        // Find the data by address in memory pages
        printf("Recovered addr 0x%08lx %d in %lu pages\n", addr, *(int *)dest, npages);
        return addr;
    }
};

#endif
