# Makefile for dedup kernel
include $(PARSECDIR)/defines.mk
PREFIX=${PARSECDIR}/pkgs/kernels/dedup/inst/${PARSECPLAT}

TARGET=dedup

CFLAGS += -Wall -fno-strict-aliasing -D_XOPEN_SOURCE=600

ostype=$(findstring solaris, ${PARSECPLAT})

ifeq "$(ostype)" "solaris"
    CFLAGS += -std=gnu99
endif

LIBS += -lm

DEDUP_OBJ = hashtable.o util.o dedup.o rabin.o encoder.o decoder.o mbuffer.o sha.o

# Uncomment the following to enable gzip compression
CFLAGS += -DENABLE_GZIP_COMPRESSION
LIBS += -lz

# Uncomment the following to enable bzip2 compression
#CFLAGS += -DENABLE_BZIP2_COMPRESSION
#LIBS += -lbz2

ifdef version
  ifeq "$(version)" "pthreads"
    CFLAGS += -DENABLE_PTHREADS -pthread
    DEDUP_OBJ += queue.o binheap.o tree.o
  endif
endif

CFLAGS_XTHREAD = $(filter-out -pthread, $(CFLAGS))
all: $(TARGET)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(DEDUP_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(DEDUP_OBJ) $(LIBS)
	$(CC) $(CFLAGS_XTHREAD) $(LDFLAGS) -o $(TARGET)-dthread $(DEDUP_OBJ) $(LIBS) $(DTHREAD_FLAGS)
	$(CC) $(CFLAGS_XTHREAD) $(LDFLAGS) -o $(TARGET)-nvthread $(DEDUP_OBJ) $(LIBS) $(NVTHREAD_FLAGS)

clean:
	rm -f *~ *.o $(TARGET) $(TARGET)-dthread $(TARGET)-nvthread

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)

