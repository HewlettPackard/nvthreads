LIBS = -lm -lrt -ldl
CFLAGS = -Wall -g -O3 # -T../src/dthreads.ld
DTHREAD_LIBS = $(LIBS) -rdynamic ../src/libdthread.so
PTHREAD_LIBS = $(LIBS) -lpthread

all:
	g++ $(CFLAGS) -o dthread_ordering ordering.c $(DTHREAD_LIBS) 
	g++ $(CFLAGS) -o pthread_ordering ordering.c $(PTHREAD_LIBS) 
	g++ $(CFLAGS) -o dthread_atlasp2 atlasp2.c $(DTHREAD_LIBS) 
	g++ $(CFLAGS) -o pthread_atlasp2 atlasp2.c $(PTHREAD_LIBS) 

