CC      = gcc
CPP     = g++

W       = -W -Wall -Wno-unused-parameter -Wbad-function-cast

OPT = -O0 -g

CFLAGS  = -pthread $(OPT) $(W) $(XCFLAGS)

# MDB and LevelDB library placeholder
MDB_LIBDIR = /home/bhandark/Atlas/examples/mdb
LEVELDB_LIBDIR = /home/bhandark/Atlas/examples/mdb/leveldb-bench/leveldb
LEVELDB_INCDIR = /home/bhandark/Atlas/examples/mdb/leveldb-bench/leveldb/include

DB_LDFLAGS = -L$(MDB_LIBDIR) -L$(LEVELDB_LIBDIR) -lmdb -lleveldb
DB_CPPFLAGS = -I$(LEVELDB_INCDIR)

.PHONY: clean

PROGS = mdb_bench

all: $(PROGS)

mdb_bench: db_bench_mdb.o testutil.o
	$(CPP) $(CFLAGS) $^ -std=gnu89 -fPIC $(DB_LDFLAGS) -o $@
 
%.o: %.cc
	$(CPP) $(CFLAGS) -fPIC $(DB_CPPFLAGS) -c $<


clean:
	-rm -rf $(PROGS) *.o 
