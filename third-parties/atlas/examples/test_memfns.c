#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atlas_api.h"
#include "atlas_alloc.h"

#define MAXLEN 256
    
int main()
{
    NVM_Initialize();
    uint32_t rgn_id = NVM_FindOrCreateRegion("memtest", O_RDWR, NULL);
    
    char * buf1 = (char *) nvm_alloc(MAXLEN, rgn_id);
    char * buf2 = (char *) nvm_alloc(MAXLEN, rgn_id);

    strcpy(buf1, "This is a test                                    \n");

    NVM_BEGIN_DURABLE();
    
    NVM_MEMSET(buf1+15, ':', 1);
    NVM_MEMCPY(buf2, buf1, MAXLEN);
    NVM_MEMMOVE(buf2+20, buf2, MAXLEN/2);

    NVM_END_DURABLE();
    
    fprintf(stderr, "buf1=%s\n", buf1);
    fprintf(stderr, "buf2=%s\n", buf2);

    NVM_CloseRegion(rgn_id);
    NVM_Finalize();
    
    return 0;
}
