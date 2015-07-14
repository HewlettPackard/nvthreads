all: kmeans-mt-pt kmeans-mt-dt datagen

kmeans-mt-pt: kmeans-mt-pt.o
	g++ -pthread kmeans-mt-pt.o -o kmeans-mt-pt

kmeans-mt-pt.o: kmeans.cpp
	g++ -pthread -O0 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o kmeans-mt-pt.o kmeans.cpp

kmeans-mt-dt: kmeans-mt-dt.o
	g++ -rdynamic ../../third-parties/dthreads/lib/libdthread.so -ldl kmeans-mt-dt.o -o kmeans-mt-dt

kmeans-mt-dt.o: kmeans.cpp
	g++ -O0 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o kmeans-mt-dt.o kmeans.cpp

datagen: datagen.o
	g++ datagen.o -o datagen

datagen.o: datagen.cpp
	g++ -O3 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o datagen.o datagen.cpp

clean: 
	rm *.d *.o kmeans-mt-pt kmeans-mt-dt datagen
