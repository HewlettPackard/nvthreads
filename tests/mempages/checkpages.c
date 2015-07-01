#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>

#define block_size 4104
int main(int argc, char ** argv){
    char filename[100];
    int i, j;
    size_t fsize;
    int nblocks;
    if ( argc <= 1 ) {
        printf("Please input memory log filename\n");
        return -1;
    }

    /* Check file size */
    FILE *fptr = fopen(argv[1], "r");
    fseek(fptr, 0L, SEEK_END);
    fsize = ftell(fptr);
    printf("file size: %zu bytes\n", fsize);
    fclose(fptr); 

    /* Open file descriptor */
    int fd = open(argv[1], O_RDWR, S_IRUSR | S_IWUSR);
    if ( fd == -1 ) {
        printf("failed to open file\n");
        return -1;
    }
    printf("opened fd: %d\n", fd);
    char *ptr = (char *)mmap(NULL, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if( ptr == MAP_FAILED){
		printf("failed to mmap\n");
		return -1;
	}
    
    nblocks = fsize / block_size;   
    printf("%d blocks, dumping memory page content:\n", nblocks);
    for (j = 0; j < nblocks; j++) {
        printf("\n--------Page %d: ", j);
        for (i = 0; i < block_size; i++) {
            if ( i < sizeof(void*) ) {
                printf("%c", ptr[j*block_size] & 0xffU);                
            }
            if ( i == sizeof(void *) ) {
                printf("--------\n");
            }
            
            if ( i % 50 == 0 && i > 0 ) {
                printf("\n");
            }
//          if ( ptr[j * block_size + i] >= 97 && ptr[j * block_size + i] <= 122  ) {
//              printf("\nrecovered: %c\n", ptr[j * block_size + i]);
//          }
            printf("%02x ", (unsigned)ptr[j * block_size + i] & 0xffU);
            
        }
    }
    printf("\n");

	return 0;
}
