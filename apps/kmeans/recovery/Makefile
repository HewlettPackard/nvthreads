#GCC=/home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icpc

GCC=g++
GCCFLAGS=-O0 -g3 -MMD -MP -std=c++0x

NVINCLUDE=$(PWD)/../../../src/include
NVSRC=$(PWD)/../../../src/source/nvrecovery.cpp
NVPATH=$(PWD)/../../../src/libnvthread.so

all: kmeans

kmeans: kmeans.cpp $(NVSRC)
	$(GCC) $(GCCFLAGS) -DNVTHREADS -I$(NVINCLUDE) $(NVSRC) kmeans.cpp -o kmeans -rdynamic $(NVPATH) -ldl
	$(GCC) $(GCCFLAGS) -I$(NVINCLUDE) $(NVSRC) kmeans.cpp -o kmeans_malloc -lpthread

clean: 
	rm -rf *.d *.o kmeans _crashed _running /mnt/tmpfs/*
