
OBJS = main.o \
       vector.o \
       crsmatrix.o \
       algorithm.o \
       logger.o

DEBUG = 

GCC = /home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icpc
CC = $(GCC) $(DEBUG) -std=gnu99 -O3

PTEXEC = prr-pt
DTEXEC = prr-dt
NVTEXEC = prr-nvt

W_FLAGS = -Wall -Wextra -pedantic

FLAGS = -I . $(W_FLAGS)

NVTPATH = /mnt/ssd/terry/workspace/nvthreads
NVTLIB = -rdynamic $(NVTPATH)/src/libnvthread.so

DTPATH = /mnt/ssd/terry/workspace/nvthreads/third-parties/dthreads
DTLIB = -rdynamic $(DTPATH)/libdthread.so

LIBS = -lrt -ldl

%.o : %.c
	$(CC) $(FLAGS) -c $<

all: pt dt nvt
	

pt: $(OBJS)
	@echo "Building $(PTEXEC)"
	$(CC) $(OBJS) $(FLAGS) $(LIBS) -lpthread -o $(PTEXEC)

dt: $(OBJS)
	@echo "Building $(DTEXEC)"
	$(CC) $(OBJS) $(FLAGS) $(LIBS) $(DTLIB) -o $(DTEXEC)

nvt: $(OBJS)
	@echo "Building $(NVTEXEC)"
	$(CC) $(OBJS) $(FLAGS) $(LIBS) $(NVTLIB) -o $(NVTEXEC)

clean:
	@echo "Cleaning all!"
	\rm -rf tags *.o *.out *.txt *.~ $(PTEXEC) $(DTEXEC) $(NVTEXEC)
