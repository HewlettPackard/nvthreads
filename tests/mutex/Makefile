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

all:	condvar


condvar:	
	$(CC) $(CFLAGS) condvar.c -o condvar.o

clean:
	rm *.o /mnt/tmpfs/*
