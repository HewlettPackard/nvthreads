NVPATH=$(PWD)/../../../src/libnvthread.so
DPATH=$(PWD)/../../../third-parties/dthreads/libdthread.so
bench_include_dir=$(PWD)/../../../eval/include/
nv_include_dir=$(PWD)/../../../src/include/
GCC=g++
FLAGS=-march=core2 -mtune=core2 -O5 -g -DNDEBUG -std=c++0x

all: kmeans-pthread kmeans-dthread kmeans-nvthread

kmeans-pthread:
	$(GCC) $(FLAGS) -DPTHREADS -I/$(bench_include_dir) kmeans-pthread.c -o kmeans-pthread.o -lpthread
kmeans-dthread:
	$(GCC) $(FLAGS) -I/$(bench_include_dir) kmeans-pthread.c -o kmeans-dthread.o -rdynamic $(DPATH) -ldl
kmeans-nvthread:
	$(GCC) $(FLAGS) -DNVTHREADS -I/$(bench_include_dir) -I/$(nv_include_dir) kmeans-pthread.c -o kmeans-nvthread.o -rdynamic $(NVPATH) -ldl

clean:
	rm *.o
