OBJS = main.o \
       vector.o \
       matrix.o \
       crsmatrix.o \
       crsmatrixop.o \
       algorithm.o \
       utils.o \
       logger.o

CC = gcc -ggdb -std=gnu99 -O3

PTEXEC = prr-pt
DTEXEC = prr-dt
NVTEXEC = prr-nvt

W_FLAGS = -Wall -Wextra -pedantic

FLAGS = -I . $(W_FLAGS)

LINKOPT = -dynamic

NVTPATH = $(LINKOPT) ../../../src/libnvthread.so
#DTPATH = -rdynamic -libdthread -ldl
DTPATH = $(LINKOPT) ../../../third-parties/dthreads/lib/libdthread.so

LIBS = -lrt -ldl

%.o : %.c
	$(CC) $(FLAGS) -c $<

all: pt dt nvt
	

pt: $(OBJS)
	@echo "Building $(PTEXEC)"
	$(CC) $(OBJS) -pthread $(FLAGS) $(LIBS) -o $(PTEXEC)

dt: $(OBJS)
	@echo "Building $(DTEXEC)"
	$(CC) $(DTPATH) $(OBJS) $(FLAGS) $(LIBS)  -o $(DTEXEC)

nvt: $(OBJS)
	@echo "Building $(NVTEXEC)"
	$(CC) $(NVTPATH) $(OBJS) $(FLAGS) $(LIBS) -o $(NVTEXEC)

clean:
	@echo "Cleaning all!"
	\rm -rf tags *.o *.out *.txt *.~ $(PTEXEC) $(DTEXEC) $(NVTEXEC)
