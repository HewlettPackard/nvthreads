#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(int argc, char **argv){
    
    int fd = open(argv[1], O_RDONLY);
    if ( fd == -1 ) {
        printf("can't read %s\n", argv[1]);
        return -1;
    }

    struct stat sb; 
    if ( fstat(fd, &sb) == -1 ) {
        printf("can't stat %s\n", argv[1]);
        return -1;
    }

    off_t size = sb.st_size; 

	char *ptr = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if ( ptr == MAP_FAILED ) {
        printf("can't mmap %s\n", argv[1]);
        return -1; 
    }
    unsigned long addr;
    memcpy(&addr, (char*)ptr, sizeof(unsigned long));
    printf("read: 0x%08lx\n", addr);

    int c;

    int offset = (8 + 4096) + (8 + 0x30);
    memcpy(&c,(char *)(ptr + offset), sizeof(int));
    printf("read c = %d\n", c);

    return 0;
}

