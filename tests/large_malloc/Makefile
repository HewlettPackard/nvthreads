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

all:	over4g

over4g:
	$(CC) $(CFLAGS) -DNVTHREAD $(NVINCLUDE_DIRS) $(NVSRCS) over4g.c -o over4g.o -rdynamic $(NVLIB)
	$(CC) $(CFLAGS) over4g.c -o over4g_pthreads.o -lpthread 

clean:
	rm *.o 
