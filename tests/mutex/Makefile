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

all:	condvar nested_locks dependence


condvar:	
	$(CC) $(CFLAGS) $(NVINCLUDE_DIRS) $(NVSRCS) condvar.c -o condvar.o -rdynamic $(NVLIB) -ldl
nested_locks:
	$(CC) $(CFLAGS) $(NVINCLUDE_DIRS) $(NVSRCS) nested_locks.c -o nested_locks.o -rdynamic $(NVLIB) -ldl
dependence:
	$(CC) $(CFLAGS) $(NVINCLUDE_DIRS) $(NVSRCS) dependence.c -o dependence.o -rdynamic $(NVLIB) -ldl

clean:
	rm *.o /mnt/tmpfs/*
