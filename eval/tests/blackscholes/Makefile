include ../defines.mk

#NTHREADS=12
TEST_NAME = blackscholes
TEST_FILES = blackscholes
TEST_ARGS = $(NTHREADS) $(DATASET_HOME)/parsec-3.0-naive/parsec-3.0/pkgs/apps/blackscholes/inputs/in_10M.txt prices.txt

CFLAGS = -DENABLE_THREADS -DPARALLEL -DNCO=2 -DN=960
PTHREAD_LIBS = -lm -lstdc++
DTHREAD_LIBS = -lm -lstdc++
NVTHREAD_LIBS = -lm -lstdc++

SRC_SUFFIX=.cpp

DMP_O_GRAN = 6
DMP_O_QUANTUM = 100000

DMP_B_GRAN = 6
DMP_B_QUANTUM = 100000

include $(HOME)/Default.mk

