#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(int argc, char** argv) {

  struct timeval start, end;
  unsigned long long elapsed;
  int fd;
  void* p;
  int cacheline_size = 8;
  unsigned long filesize = 12 * 1024 * 1024;
  unsigned long i;


  if (argc < 2) {
    printf("usage: %s mmap-file\n", argv[0]);
    exit(-1);
  }

  fd = open(argv[1], O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    perror("open");
    exit(-1);
  }
  if (ftruncate(fd, filesize) == -1){
    perror("ftruncate");
    exit(-1);
  }

  p = mmap(0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }

  gettimeofday(&start, NULL);
  for (i = 0; i < (filesize / cacheline_size); i = i + cacheline_size) {
    memset(p + (i), 'a', cacheline_size);
    if (msync(p, cacheline_size, MS_SYNC) == -1) {
      perror("msync");
      exit(-1);
    }
//  printf("p[%d / %d]\n", i, filesize / cacheline_size);
  }
  gettimeofday(&end, NULL);
  elapsed = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;

  if (close(fd) == -1) {
    perror("close");
    exit(-1);
  }
  printf("%llu ms\n", elapsed);
  return 0;
}
