#ifndef _ATLAS_ALLOC_H
#define _ATLAS_ALLOC_H

#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

// public interfaces
#ifdef __cplusplus
extern "C" {
#endif    
void *nvm_alloc(size_t, uint32_t /* region id */);
    void *nvm_calloc(size_t nmemb, size_t sz, uint32_t /* region id */);
void *nvm_realloc(void *, size_t, uint32_t /* region id */);
void nvm_free(void *);

// Create a persistent region with the provided name and the access flag.
// This interface does not check for an existing entry with
// the same name. If a region with the same name already exists, the
// behavior of the program is undefined.
// Input parameters: region name,
// access flag (one of O_RDONLY, O_WRONLY, O_RDWR)
// Return value: id of the region
uint32_t NVM_CreateRegion(const char *name, int flags);

// Create a persistent region with the provided name.
// If the region already exists, the existing id of the region is returned.
// Otherwise a region is created and its newly assigned id returned.
// The second parameter specifies the access flag.
// The third parameter indicates whether the region got created as a
// result of the call. The caller should provide NULL as the third
// parameter if it does not care.
// Input parameters: region name, access flag (O_RDONLY, O_WRONLY, O_RDWR),
// pointer to int.
// Return value: id of the region
uint32_t NVM_FindOrCreateRegion(const char *name, int flags, int *is_created);
    
// Find the id of a region when it is known to exist already.
// This interface should be used over NVM_FindOrCreateRegion for efficiency
// reasons if the region is known to exist. 
// If a region with the provided name does not exist, an assertion failure
// will occur.
// Input parameters: region name, access flag (O_RDONLY, O_WRONLY, O_RDWR)
// Return value: id of the region
uint32_t NVM_FindRegion(const char * name, int flags);

// Delete the region with the provided name.
// Use this interface to completely destroy a region.
// If the region does not exist, an assertion failure will occur.
// Input parameters: region name
// Return value: none
void NVM_DeleteRegion(const char * name);

// Close the region with the provided name. After closing, it won't be
// available to the calling process without calling NVM_FindOrCreateRegion.
// The region will stay in NVM even after calling this interface.
// This interface allows closing a region with normal bookkeeping.
// Input parameters: region name
// Return value: none
void NVM_CloseRegion(uint32_t rid);

// Get the root pointer of the persistent region rid. The region must
// have been created already. The default root is set to NULL.
// Currently, only one root is provided for
// a given region. The idea is that anything within a region that is not
// reachable from the root during recovery is assumed to be garbage and
// will be recycled. During execution, anything within a region that is
// not reachable from the root or from other _roots_ (in the GC sense) is
// assumed to be garbage as well.
// Input parameters: region id
// Return value: root pointer of the region
void * NVM_GetRegionRoot(uint32_t rid);

// Set the root pointer of the persistent region rid. The region must
// have been created already. Calling this interface multiple times
// results in overwriting the old root with the new root.
// Input parameters: region id, pointer to be used as the root
// Return value: none
void NVM_SetRegionRoot(uint32_t rid, void *);

#ifdef __cplusplus
}
#endif
        
#endif
