CC	= gcc
CPP     = g++
W	= -W -Wall -Wno-unused-parameter -Wbad-function-cast
OPT = -O0 -g
CFLAGS	= -pthread $(OPT) $(W) $(XCFLAGS) #-DMDB_DEBUG
LDLIBS	=
SOLIBS	=

ATLAS_ROOTDIR      = ../../..
ATLAS_SRCDIR       = $(ATLAS_ROOTDIR)/src
ATLAS_INCDIR       = $(ATLAS_ROOTDIR)/include
ATLAS_INT_INCDIR   = $(ATLAS_SRCDIR)/internal_includes
ATLAS_LIBDIR       = $(ATLAS_ROOTDIR)/lib
ATLAS_TOOLDIR      = $(ATLAS_ROOTDIR)/tools

ATOMIC_OPS_ROOTDIR = ../../../../libs
ATOMIC_OPS_INCDIR  = $(ATOMIC_OPS_ROOTDIR)/include
ATOMIC_OPS_LIBDIR  = $(ATOMIC_OPS_ROOTDIR)/lib

LIBATLAS           = $(ATLAS_LIBDIR)/libatlas.a

ATLAS_CFLAGS       =   $(COMMON_CFLAGS) -I$(ATLAS_INCDIR) \
                        -I$(ATLAS_INT_INCDIR) -I$(ATOMIC_OPS_INCDIR)

ATLAS_LDFLAGS      =   -L$(ATLAS_LIBDIR) -latlas \
                        -L$(ATOMIC_OPS_LIBDIR) -latomic_ops \
                        -lrt -lpthread


.PHONY: clean


PROGS	= mdb_stat mtest mtest2 mtest3 mtest4 mtest5 mtest_mt
#all:	libmdb.a libmdb.so $(PROGS)
all:	libmdb.a $(PROGS)

clean:
	rm -rf $(PROGS) *.[ao] *.so *~ testdb

test:	all
	mkdir testdb
	./mtest && ./mdb_stat testdb

libmdb.a:	mdb_nvm.o midl.o
	ar rs $@ mdb_nvm.o midl.o

#libmdb.so:	mdb.o midl.o
#	gcc -shared -o $@ mdb.o midl.o $(SOLIBS)

mdb_stat: mdb_stat.o libmdb.a
mtest:    mtest.o    libmdb.a
mtest2:	mtest2.o libmdb.a
mtest3:	mtest3.o libmdb.a
mtest4:	mtest4.o libmdb.a
mtest5:	mtest5.o libmdb.a
mtest6:	mtest6.o libmdb.a
mtest_mt: mtest_mt.o libmdb.a

mdb_nvm.o: mdb_nvm.c mdb.h midl.h
	$(CC) $(CFLAGS) -fPIC $(CPPFLAGS) $(ATLAS_CFLAGS) -c mdb_nvm.c

midl.o: midl.c midl.h
	$(CC) $(CFLAGS) -fPIC $(CPPFLAGS) -c midl.c

%:	%.o
	$(CPP) $(CFLAGS) $(LDFLAGS)  $^ $(LDLIBS)  $(ATLAS_LDFLAGS)  -o $@

%.o:	%.c mdb.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ATLAS_CFLAGS) -c $<

