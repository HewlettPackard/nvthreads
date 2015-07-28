NVPATH=$(PWD)/../../src/libnvthread.so
DPATH=$(PWD)/../../third-parties/dthreads/src/libdthread.so
all: kmeans-mt-pt kmeans-mt-dt kmeans-mt-nvt datagen

kmeans-mt-pt: kmeans-mt-pt.o
	g++ -pthread kmeans-mt-pt.o -o kmeans-mt-pt

kmeans-mt-pt.o: kmeans.cpp
	g++ -pthread -O0 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o kmeans-mt-pt.o kmeans.cpp

kmeans-mt-dt: kmeans-mt-dt.o
	g++ -rdynamic kmeans-mt-dt.o -o kmeans-mt-dt $(DPATH) -ldl 

kmeans-mt-dt.o: kmeans.cpp
	g++ -O0 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o kmeans-mt-dt.o kmeans.cpp

kmeans-mt-nvt: kmeans-mt-nvt.o
	g++ -g kmeans-mt-nvt.o -o kmeans-mt-nvt  -rdynamic $(NVPATH) -ldl 

kmeans-mt-nvt.o: kmeans.cpp
	g++ -g -O0 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o kmeans-mt-nvt.o kmeans.cpp

datagen: datagen.o
	g++ datagen.o -o datagen

datagen.o: datagen.cpp
	g++ -O3 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o datagen.o datagen.cpp

clean: 
	rm *.d *.o kmeans-mt-pt kmeans-mt-dt kmeans-mt-nvt datagen
