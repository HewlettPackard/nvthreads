OBJS = main.o \
       vector.o \
       crsmatrix.o \
       algorithm.o \
       logger.o

DEBUG = 

#CC = gcc $(DEBUG) -std=gnu99 -O3
CC = icpc -std=gnu99 

PTEXEC = prr-pt
DTEXEC = prr-dt
NVTEXEC = prr-nvt

#W_FLAGS = -Wall -Wextra -pedantic


LINKOPT = -dynamic

MNEMOSYNE_ROOT=$(PWD)/../../
MNEMOSYNE_FLAGS= -Qtm_enabled -fpic -g -m64 -DM_PCM_CPUFREQ=2500 -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=1200
MNEMOSYNE_INC = -I$(MNEMOSYNE_ROOT)library/mcore/include -I$(MNEMOSYNE_ROOT)library/mtm/include -I$(MNEMOSYNE_ROOT)library/pmalloc/include
MNEMOSYNE_LIBS = $(MNEMOSYNE_ROOT)build/library/pmalloc/libpmalloc.so $(MNEMOSYNE_ROOT)build/library/mcore/libmcore.so $(MNEMOSYNE_ROOT)build/library/mtm/libmtm.so

LIBS = -lrt -ldl

FLAGS = -I . $(MNEMOSYNE_FLAGS)

%.o : %.c
	$(CC) $(FLAGS) $(MNEMOSYNE_INC) -c $<  $(MNEMOSYNE_LIBS)

all: pt
	

pt: $(OBJS)
	@echo "Building $(PTEXEC)"
	$(CC) -g $(OBJS) $(FLAGS) $(LIBS) $(MNEMOSYNE_INC) -lpthread -o $(PTEXEC) $(MNEMOSYNE_LIBS)

clean:
	@echo "Cleaning all!"
	\rm -rf tags *.o *.out *.txt *.~ $(PTEXEC) $(DTEXEC) $(NVTEXEC)
