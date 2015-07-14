NVTHREAD_HOME=../../
CC = g++
CFLAGS = -g -std=gnu++11 
PLIB = -lpthread
NVLIB = $(NVTHREAD_HOME)/src/libnvthread.so
DLIB = $(NVTHREAD_HOME)/third-parties/dthreads/src/libdthread.so

#nvthread
INC_DIR = $(NVTHREAD_HOME)/src/include
SRC_DIR = $(NVTHREAD_HOME)/src/source

NVINCLUDE_DIRS = -I$(INC_DIR)
NVSRCS = $(SRC_DIR)/nvrecovery.cpp 

all:	atlas_tests

atlas_tests:
	$(CC) $(CFLAGS)  -DNVTHREAD $(NVINCLUDE_DIRS) $(NVSRCS) atlas_fig4.c -o atlas_fig4_nvthread.o -rdynamic $(NVLIB)
	$(CC) $(CFLAGS)  atlas_fig4.c -o atlas_fig4_pthread.o $(PLIB)


clean:
	rm *.o MemLog* varmap* _crashed _running /mnt/tmpfs/*
