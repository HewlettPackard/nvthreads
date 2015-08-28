GCC=g++
GCCFLAGS=-O3 -c -MMD -MP -std=c++0x -DENABLE_PROFILING

NVPATH=$(PWD)/../../../src/libnvthread.so
DPATH=$(PWD)/../../../third-parties/dthreads/libdthread.so
all: kmeans-mt-pt kmeans-mt-dt kmeans-mt-nvt

kmeans-mt-pt: kmeans-mt-pt.o logger.o
	$(GCC) -pthread kmeans-mt-pt.o logger.o -o kmeans-mt-pt

kmeans-mt-pt.o: kmeans.cpp
	$(GCC) -pthread $(GCCFLAGS) -o kmeans-mt-pt.o kmeans.cpp

kmeans-mt-dt: kmeans-mt-dt.o logger.o
	$(GCC) kmeans-mt-dt.o logger.o -o kmeans-mt-dt -rdynamic $(DPATH) -ldl 

kmeans-mt-dt.o: kmeans.cpp
	$(GCC) $(GCCFLAGS) -o kmeans-mt-dt.o kmeans.cpp

kmeans-mt-nvt: kmeans-mt-nvt.o logger.o
	$(GCC) kmeans-mt-nvt.o logger.o -o kmeans-mt-nvt -rdynamic $(NVPATH) -ldl

kmeans-mt-nvt.o: kmeans.cpp
	$(GCC) $(GCCFLAGS) -o kmeans-mt-nvt.o kmeans.cpp

logger.o: logger.c logger.h
	$(GCC) $(GCCFLAGS) -o logger.o logger.c

clean: 
	rm -rf *.d *.o kmeans-mt-pt kmeans-mt-dt kmeans-mt-nvt datagen dthreads*
