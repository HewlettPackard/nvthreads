#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <vector>
#include <dirent.h>
#include "nvrecovery.h"

/* Query for user program */
bool isCrashed(void) {
    bool rv = false;
    char crash_filename[256];
    sprintf(crash_filename, "_crashed");

    lprintf("Checking crashed file: %s\n", crash_filename);
    // Crash file exists
    if ( access(crash_filename, F_OK) != -1 ) {
        lprintf("Your program crashed before.  Please recover your progress using libnvthread API\n");
        rv = true;
    } else {
        lprintf("Your program did not crash before.  Continue normal execution\n");
        rv = false;
    }
    return rv;
}

