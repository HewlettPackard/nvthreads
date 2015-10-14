NVTHREAD_HOME=../../
CC = g++
CFLAGS = -g  
PLIB = -lpthread
NVLIB = $(NVTHREAD_HOME)/src/libnvthread.so
DLIB = $(NVTHREAD_HOME)/third-parties/dthreads/src/libdthread.so

#nvthread
INC_DIR = $(NVTHREAD_HOME)/src/include
SRC_DIR = $(NVTHREAD_HOME)/src/source

NVINCLUDE_DIRS = -I$(INC_DIR)
NVSRCS = $(SRC_DIR)/nvrecovery.cpp 

all:	checkpages dtests nvtests forloop recover_int nocriticalsection


checkpages:	
	$(CC) $(CFLAGS) checkpages.c -o checkpages.o
	$(CC) $(CFLAGS) readmemlog.c -o readmemlog.o
ptests:
	$(CC) $(CFLAGS) test1.c -o ptest1.o $(PLIB)
	$(CC) $(CFLAGS) test2.c -o ptest2.o $(PLIB)

dtests:
	$(CC) $(CFLAGS)  test1.c -o dtest1.o -rdynamic $(DLIB)
	$(CC) $(CFLAGS)  test2.c -o dtest2.o -rdynamic $(DLIB)
	$(CC) $(CFLAGS)  test3.c -o dtest3.o -rdynamic $(DLIB)

forloop:
	$(CC) $(CFLAGS) $(NVINCLUDE_DIRS) $(NVSRC) forloop.c -o forloop.o -rdynamic $(NVLIB)

nvtests:
	$(CC) $(CFLAGS) -DNVTHREAD $(NVINCLUDE_DIRS) $(NVSRCS) test1.c -o nvtest1.o -rdynamic $(NVLIB)
	$(CC) $(CFLAGS) -DNVTHREAD $(NVINCLUDE_DIRS) $(NVSRCS) test2.c -o nvtest2.o -rdynamic $(NVLIB)
	$(CC) $(CFLAGS) -DNVTHREAD $(NVINCLUDE_DIRS) $(NVSRCS) test3.c -o nvtest3.o -rdynamic $(NVLIB)

nocriticalsection:		
	$(CC) $(CFLAGS) -DNVTHREAD $(NVINCLUDE_DIRS) $(NVSRCS) nocrtsection.c -o nocrtsection.o -rdynamic $(NVLIB)

clean:
	rm *.o MemLog* varmap* _crashed _running /mnt/tmpfs/*
