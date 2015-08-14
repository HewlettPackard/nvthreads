#ifndef _ATLAS_ALLOC_PRIV_H
#define _ATLAS_ALLOC_PRIV_H

uint32_t NVM_CreateRegion_priv(
    const char *name, int flags, bool is_lock_held=false, void *base_addr=0);

uint32_t NVM_FindOrCreateRegion_priv(
    const char *name, int flags, 
    bool is_forced_map=false, int *is_created_p=NULL);

uint32_t NVM_FindRegion_priv(const char *name, int flags,
                             bool is_forced_map=false);

void NVM_CloseRegion_priv(uint32_t rid, bool is_recover=false);

#endif
