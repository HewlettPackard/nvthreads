all: kmeans-singlethreaded kmeans-multithreaded datagen

kmeans-multithreaded: kmeans-mt.o
	g++ -pthread kmeans-mt.o -o kmeans-multithreaded

kmeans-mt.o: kmeans-multithreaded.cpp
	g++ -pthread -O0 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o kmeans-mt.o kmeans-multithreaded.cpp

kmeans-singlethreaded: kmeans-st.o
	g++ kmeans-st.o -o kmeans-singlethreaded

kmeans-st.o: kmeans-singlethreaded.cpp
	g++ -O0 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o kmeans-st.o kmeans-singlethreaded.cpp

datagen: datagen.o
	g++ datagen.o -o datagen

datagen.o: datagen.cpp
	g++ -O0 -g3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o datagen.o datagen.cpp

clean: 
	rm *d *o "kmeans-singlethreaded" "kmeans-multithreaded" "datagen"
