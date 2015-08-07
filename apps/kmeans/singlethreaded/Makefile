all: kmeans

kmeans: kmeans.o
	g++ kmeans.o -o kmeans

kmeans.o: kmeans.cpp
	g++ -O3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o kmeans.o kmeans.cpp

clean: 
	rm -rf *.d *.o kmeans
