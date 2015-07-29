
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#define BUF_SIZE 8192

int main(int argc, char* argv[]) {

	int input_fd, output_fd;    /* Input and output file descriptors */
	ssize_t ret_in, ret_out;    /* Number of bytes returned by read() and write() */
	char buffer[BUF_SIZE];      /* Character buffer */
	sprintf(buffer, "dummy string");
	/* Create output file descriptor */
	output_fd = open(argv[1], O_WRONLY | O_CREAT, 0644);
	if(output_fd == -1){
		perror("open");
		return 3;
	}

	/* Copy process */
	int i = 0;
	clock_t start, end;
	start = clock();
	while (i < 100000){
		ret_out = write (output_fd, &buffer, (ssize_t) 4096);
		i++;
	}
	end = clock();
	printf( "Number of seconds: %f\n", (end-start)/(double)CLOCKS_PER_SEC );

	//fsync(output_fd);
	/* Close file descriptors */
	close (output_fd);

	return 0; 
}
