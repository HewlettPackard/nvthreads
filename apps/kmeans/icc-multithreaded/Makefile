GCC=/home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icpc
#GCC=g++
GCCFLAGS=-O3 -c -MMD -MP -std=c++0x

NVPATH=$(PWD)/../../../src/libnvthread.so
DPATH=$(PWD)/../../../third-parties/dthreads/libdthread.so
all: kmeans-mt-pt kmeans-mt-dt kmeans-mt-nvt

kmeans-mt-pt: kmeans-mt-pt.o
	$(GCC) -pthread kmeans-mt-pt.o -o kmeans-mt-pt

kmeans-mt-pt.o: kmeans.cpp
	$(GCC) -pthread $(GCCFLAGS) -o kmeans-mt-pt.o kmeans.cpp

kmeans-mt-dt: kmeans-mt-dt.o
	$(GCC) -rdynamic kmeans-mt-dt.o -o kmeans-mt-dt $(DPATH) -ldl 

kmeans-mt-dt.o: kmeans.cpp
	$(GCC) $(GCCFLAGS) -o kmeans-mt-dt.o kmeans.cpp

kmeans-mt-nvt: kmeans-mt-nvt.o
	$(GCC) kmeans-mt-nvt.o -o kmeans-mt-nvt -rdynamic $(NVPATH) -ldl 

kmeans-mt-nvt.o: kmeans.cpp
	$(GCC) $(GCCFLAGS) -o kmeans-mt-nvt.o kmeans.cpp

clean: 
	rm -rf *.d *.o kmeans-mt-pt kmeans-mt-dt kmeans-mt-nvt datagen dthreads*
