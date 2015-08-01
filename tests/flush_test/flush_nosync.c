#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

unsigned long long TOUCH_SIZE = 2 * 1024 * 1024;
int main(int argc, char **argv){
	unsigned long i = 0;
	int *a = (int*)malloc(TOUCH_SIZE);
	int fd = open(argv[1], O_CREAT | O_WRONLY | O_ASYNC, 0664);
	while (i < 1024 * 1024){
		write(fd, (void*)a, TOUCH_SIZE);
		i++;
	}
	//fdatasync(fd);
	close(fd);
	return 0;
}
