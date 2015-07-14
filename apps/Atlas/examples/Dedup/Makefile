# Makefile for dedup kernel


#PREFIX=/home/dhc/multicore/tests/bench/parsec-1.0/pkgs/kernels/dedup/inst/${PARSECPLAT}

CC = /home/dhc/multicore/llvm/llvm/Debug+Asserts/bin/clang
LD = /home/dhc/multicore/llvm/llvm/Debug+Asserts/bin/clang++

TARGET=dedup

BIGFILE_CFLAGS=$(shell getconf LFS_CFLAGS)

#CFLAGS += -O2 -DNVM_STATS -Wall -std=c89 -fno-strict-aliasing -D_XOPEN_SOURCE=500 -I/home/dhc/multicore/Atlas/include -I/home/dhc/multicore/libs/include
CFLAGS += -O2 -Wall -std=c89 -fno-strict-aliasing -D_XOPEN_SOURCE=500 -I../../include -I/home/dhc/multicore/libs/include

LDFLAGS += -L../../lib

LIBS += -lz -lm -lssl -lcrypto -latlas -lpthread

DEDUP_OBJ =  hashtable.o queue.o util.o binheap.o tree.o decoder.o dedup.o rabin.o encoder.o

#ifdef version
#  ifeq "$(version)" "parallel"
    CFLAGS += -DPARALLEL -pthread
#  endif
#endif


all: $(TARGET)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(DEDUP_OBJ)
	$(LD) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(DEDUP_OBJ) $(LIBS)

clean:
	rm -f *~ *.o $(TARGET)

#install:
#	mkdir -p $(PREFIX)/bin
#	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)

