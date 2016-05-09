NVTHREAD_HOME=../../
CC = g++
CFLAGS = -g  
PLIB = -lpthread
NVLIB = $(NVTHREAD_HOME)/src/libnvthread.so -ldl
DLIB = $(NVTHREAD_HOME)/third-parties/dthreads/src/libdthread.so -ldl

#nvthread
INC_DIR = $(NVTHREAD_HOME)/src/include
SRC_DIR = $(NVTHREAD_HOME)/src/source

NVINCLUDE_DIRS = -I$(INC_DIR)

all:	condvar nested_locks barrier locks

locks:
	$(CC) $(CFLAGS) $(NVINCLUDE_DIRS) locks.c -o locks.o -rdynamic $(NVLIB) -ldl
condvar:	
	$(CC) $(CFLAGS) $(NVINCLUDE_DIRS) condvar.c -o condvar.o -rdynamic $(NVLIB) -ldl
nested_locks:
	$(CC) $(CFLAGS) $(NVINCLUDE_DIRS) nested_locks.c -o nested_locks.o -rdynamic $(NVLIB) -ldl
barrier:
	$(CC) $(CFLAGS) $(NVINCLUDE_DIRS) barrier.c -o barrier.o -rdynamic $(NVLIB) -ldl	
clean:
	rm *.o /mnt/tmpfs/*
