NVPATH=$(PWD)/../../../src/libnvthread.so
DPATH=$(PWD)/../../../third-parties/dthreads/libdthread.so
include_dir=$(PWD)/../../../eval/include/

GCC=gcc
FLAGS=-march=core2 -mtune=core2 -O5 -g -DNDEBUG 

all: kmeans-pthread kmeans-dthread kmeans-nvthread

kmeans-pthread:
	$(GCC) $(FLAG) -I/$(include_dir) kmeans-pthread.c -o kmeans-pthread.o -lpthread
kmeans-dthread:
	$(GCC) $(FLAG) -I/$(include_dir) kmeans-pthread.c -o kmeans-dthread.o -rdynamic $(DPATH) -ldl
kmeans-nvthread:
	$(GCC) $(FLAG) -I/$(include_dir) kmeans-pthread.c -o kmeans-nvthread.o -rdynamic $(NVPATH) -ldl

clean:
	rm *.o
