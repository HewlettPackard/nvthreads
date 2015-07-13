CC = g++
CFLAGS = -g -std=gnu++11 
PLIB = -lpthread
NVLIB = /home/terry/workspace/nvthreads/libnvthread.so
DLIB = /home/terry/workspace/nvthreads/third-parties/dthreads/src/libdthread.so

#nvthread
LOG_INC_DIR = /home/terry/workspace/nvthreads/include
NV_INC_DIR = /home/terry/workspace/nvthreads/api/include
NV_SRC_DIR = /home/terry/workspace/nvthreads/api/src

NVINCLUDE_DIRS = -I$(NV_INC_DIR) -I$(LOG_INC_DIR)
NVSRCS = $(NV_SRC_DIR)/nvrecovery.cpp 

all:	checkpages dtests nvtests forloop recover_int


checkpages:	
	$(CC) $(CFLAGS) checkpages.c -o checkpages.o
	$(CC) $(CFLAGS) readmemlog.c -o readmemlog.o
ptests:
	$(CC) $(CFLAGS) test1.c -o ptest1.o $(PLIB)
	$(CC) $(CFLAGS) test2.c -o ptest2.o $(PLIB)

dtests:
	$(CC) $(CFLAGS)  test1.c -o dtest1.o -rdynamic $(NVLIB)
	$(CC) $(CFLAGS)  test2.c -o dtest2.o -rdynamic $(NVLIB)

forloop:
	$(CC) $(CFLAGS) $(NVINCLUDE_DIRS) $(NVSRC) forloop.c -o forloop.o -rdynamic $(NVLIB)

nvtests:
	$(CC) $(CFLAGS) -DNVTHREAD $(NVINCLUDE_DIRS) $(NVSRCS) test1.c -o nvtest1.o -rdynamic $(NVLIB)
	$(CC) $(CFLAGS) -DNVTHREAD $(NVINCLUDE_DIRS) $(NVSRCS) test2.c -o nvtest2.o -rdynamic $(NVLIB)

recover_int:
	$(CC) $(CFLAGS) -DNVTHREAD $(NVINCLUDE_DIRS) $(NVSRCS) recover_int.c -o recover_int.o -rdynamic $(NVLIB)

clean:
	rm *.o MemLog* varmap* _crashed _running /mnt/tmpfs/*
