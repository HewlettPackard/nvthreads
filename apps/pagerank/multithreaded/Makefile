OBJS = main.o \
       vector.o \
       matrix.o \
       crsmatrix.o \
       crsmatrixop.o \
       algorithm.o \
       utils.o \
       logger.o

DEBUG = 

CC = gcc $(DEBUG) -std=gnu99 -O3

PTEXEC = prr-pt
DTEXEC = prr-dt
NVTEXEC = prr-nvt

W_FLAGS = -Wall -Wextra -pedantic

FLAGS = -I . $(W_FLAGS)

LINKOPT = -dynamic

NVTPATH = $(LINKOPT) $(PWD)/../../../src/libnvthread.so
#DTPATH = -rdynamic -libdthread -ldl
DTPATH = $(LINKOPT) $(PWD)/../../../third-parties/dthreads/lib/libdthread.so

LIBS = -lrt -ldl

%.o : %.c
	$(CC) $(FLAGS) -c $<

all: pt dt nvt
	

pt: $(OBJS)
	@echo "Building $(PTEXEC)"
	$(CC) $(OBJS) $(FLAGS) $(LIBS) -lpthread -o $(PTEXEC)

dt: $(OBJS)
	@echo "Building $(DTEXEC)"
	$(CC) $(OBJS) $(FLAGS) $(LIBS) $(DTPATH) -o $(DTEXEC)

nvt: $(OBJS)
	@echo "Building $(NVTEXEC)"
	$(CC) $(OBJS) $(FLAGS) $(LIBS) $(NVTPATH) -o $(NVTEXEC)

clean:
	@echo "Cleaning all!"
	\rm -rf tags *.o *.out *.txt *.~ $(PTEXEC) $(DTEXEC) $(NVTEXEC)
