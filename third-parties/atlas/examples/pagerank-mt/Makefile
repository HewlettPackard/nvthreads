
ATLAS_ROOTDIR      := /mnt/ssd/terry/workspace/nvthreads/third-parties/atlas
ATLAS_SRCDIR       := $(ATLAS_ROOTDIR)/src
ATLAS_INCDIR       := $(ATLAS_ROOTDIR)/include
ATLAS_INT_INCDIR   := $(ATLAS_SRCDIR)/internal_includes
ATLAS_LIBDIR       := $(ATLAS_ROOTDIR)/lib
ATOMIC_OPS_ROOTDIR := /mnt/ssd/terry/workspace/nvthreads/third-parties/atlas/libs/libatomic_ops-7.4.0-build
ATOMIC_OPS_INCDIR  := $(ATOMIC_OPS_ROOTDIR)/include
ATOMIC_OPS_LIBDIR  := $(ATOMIC_OPS_ROOTDIR)/lib

COMMON_CFLAGS      := -g -O3 -Wall -Winline $(CMDLINE_ARG)
COMMON_CPPFLAGS    := $(COMMON_CFLAGS)

ATLAS_CFLAGS       :=   $(COMMON_CFLAGS) -I$(ATLAS_INCDIR) \
						-I$(ATLAS_INT_INCDIR) -I$(ATOMIC_OPS_INCDIR) \
						-pthread -DDEBUG -DALLOC_DUMP -DNVM_STATS\

ATLAS_CPPFLAGS     := $(ATLAS_CFLAGS)

APP_CFLAGS         :=   $(COMMON_CFLAGS) -I$(ATLAS_INCDIR) \
						-I$(ATLAS_INT_INCDIR) -I$(ATOMIC_OPS_INCDIR) \
						-pthread -DNVM_STATS\

APP_CPPFLAGS       := $(APP_CFLAGS)

LIBATLAS           := $(ATLAS_LIBDIR)/libatlas.a


OBJS = main.o \
	   vector.o \
	   crsmatrix.o \
	   algorithm.o \
	   logger.o

DEBUG = 

#GCC = /home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icpc
GCC = gcc
CC = $(GCC) $(DEBUG) -std=gnu99 -O3
CPP = g++

PTEXEC = prr-pt
DTEXEC = prr-dt
NVTEXEC = prr-nvt
ATLASEXEC = prr-atlas

W_FLAGS = -Wall -Wextra -pedantic

FLAGS = -I . $(W_FLAGS)

NVTPATH = $(PWD)/../../../../
NVTLIB = -rdynamic $(NVTPATH)/src/libnvthread.so

DTPATH = $(PWD)/../../../dthreads
DTLIB = -rdynamic $(DTPATH)/libdthread.so

LIBS = -lrt -ldl

%.o : %.c
#	$(CC) -DATLAS $(FLAGS)  $(LIBATLAS) $(ATLAS_CPPFLAGS)  -c $<
	$(CC) $(FLAGS) -c $<

#all: pt dt nvt
#all: atlas

pt: $(OBJS)
	@echo "Building $(PTEXEC)"
	$(CC) $(OBJS) $(FLAGS) $(LIBS) -lpthread -o $(PTEXEC)

dt: $(OBJS)
	@echo "Building $(DTEXEC)"
	$(CC) $(OBJS) $(FLAGS) $(LIBS) $(DTLIB) -o $(DTEXEC)

nvt: $(OBJS)
	@echo "Building $(NVTEXEC)"
	$(CC) $(OBJS) $(FLAGS) $(LIBS) $(NVTLIB) -o $(NVTEXEC)

atlas: $(OBJS)
	@echo "Building $(ATLASEXEC)"
	$(CPP) -DATLAS $(OBJS) $(FLAGS) $(LIBS) $(LIBATLAS) $(ATLAS_CPPFLAGS) -o $(ATLASEXEC)

clean:
	@echo "Cleaning all!"
	\rm -rf tags *.o *.out *.txt *.~ $(PTEXEC) $(DTEXEC) $(NVTEXEC)
