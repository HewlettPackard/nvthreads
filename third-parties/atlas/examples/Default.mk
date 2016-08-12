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

LIBATLAS           += $(ATLAS_LIBDIR)/libatlas.a

PHOENIX_INCDIR     :=  ../include

NTHREADS ?=12

CC = gcc -march=core2 -mtune=core2
CXX = g++ -march=core2 -mtune=core2
CFLAGS += -O3 -g

CONFIGS = atlas
PROGS = $(addprefix $(TEST_NAME)-, $(CONFIGS))

.PHONY: default all clean

default: all
all: $(PROGS)
clean:
	rm -f $(PROGS) obj/* *.out

eval: $(addprefix eval-, $(CONFIGS))


########### atlas builders ############
#$(CPP) -DATLAS $(OBJS) $(FLAGS) $(LIBS) $(LIBATLAS) $(ATLAS_CPPFLAGS) -o $(ATLASEXEC)
LIBATLAS += $(LIBS)
ATLAS_CFLAGS = $(CFLAGS) -DATLAS $(LIBATLAS) $(ATLAS_CPPFLAGS)

ATLAS_OBJS = $(addprefix obj/, $(addsuffix -atlas.o, $(TEST_FILES)))

obj/%-atlas.o: %-pthread.c
	$(CC) $(ATLAS_CFLAGS) -c $< -o $@ -Iinclude/ -I$(PHOENIX_INCDIR) -I$(ATLAS_INCDIR) -I$(ATLAS_INT_INCDIR)

obj/%-atlas.o: %.c
	$(CC) $(ATLAS_CFLAGS) -c $< -o $@ -Iinclude/ -I$(PHOENIX_INCDIR) -I$(ATLAS_INCDIR) -I$(ATLAS_INT_INCDIR)

obj/%-atlas.o: %-pthread.cpp
	$(CC) $(ATLAS_CFLAGS) -c $< -o $@ -Iinclude/ -I$(PHOENIX_INCDIR) -I$(ATLAS_INCDIR) -I$(ATLAS_INT_INCDIR)

obj/%-atlas.o: %.cpp
	$(CXX) $(ATLAS_CFLAGS) -c $< -o $@ -Iinclude/ -I$(PHOENIX_INCDIR) -I$(ATLAS_INCDIR) -I$(ATLAS_INT_INCDIR)


### FIXME, put the 
$(TEST_NAME)-atlas: $(ATLAS_OBJS)
	$(CC) $(ATLAS_CFLAGS) -o $@.out $(ATLAS_OBJS) $(LIBATLAS)

eval-atlas: $(TEST_NAME)-atlas
	time -f "real %e" ./$(TEST_NAME)-atlas.out $(TEST_ARGS)

