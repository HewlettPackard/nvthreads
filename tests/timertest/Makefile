NVPATH=$(PWD)/../../src/libnvthread.so

all: tc tcpp tcppnvt

tc: timertest.c logger.c
	gcc -O3 -std=gnu99 -pthread logger.c timertest.c -o tc

tcpp: tcpp.o lcpp.o
	g++ tcpp.o -pthread lcpp.o -o tcpp

tcppnvt: tcpp.o lcpp.o
	g++ tcpp.o lcpp.o -rdynamic $(NVPATH) -ldl -o tcppnvt

tcpp.o: timertest.c
	g++ -O0 -c -MMD -MP -std=c++0x timertest.c -o tcpp.o

lcpp.o: logger.c
	g++ -O0 -c -MMD -MP -std=c++0x logger.c -o lcpp.o

clean:
	rm -rf tc tcpp tcppnvt *.o *.d
