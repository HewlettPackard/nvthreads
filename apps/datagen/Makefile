all: datagen

datagen: datagen.o
	g++ datagen.o -o datagen

datagen.o: datagen.cpp
	g++ -O3 -c -fmessage-length=0 -MMD -MP -std=c++0x -o datagen.o datagen.cpp

clean: 
	rm -rf *.d *.o datagen
