SOURCES=db/builder.cc db/c.cc db/dbformat.cc db/db_impl.cc db/db_iter.cc db/filename.cc db/log_reader.cc db/log_writer.cc db/memtable.cc db/repair.cc db/table_cache.cc db/version_edit.cc db/version_set.cc db/write_batch.cc table/block_builder.cc table/block.cc table/filter_block.cc table/format.cc table/iterator.cc table/merger.cc table/table_builder.cc table/table.cc table/two_level_iterator.cc util/arena.cc util/bloom.cc util/cache.cc util/coding.cc util/comparator.cc util/crc32c.cc util/env.cc util/env_posix.cc util/filter_policy.cc util/hash.cc util/histogram.cc util/logging.cc util/options.cc util/status.cc  port/port_posix.cc
MEMENV_SOURCES=helpers/memenv/memenv.cc
CC=/home/dhc/multicore/llvm/bin/clang
CXX=/home/dhc/multicore/llvm/bin/clang++
PLATFORM=OS_LINUX
PLATFORM_LDFLAGS=-pthread
W= -W -Wall -Wno-unused-parameter -Wbad-function-cast
PLATFORM_CCFLAGS= -fno-builtin-memcmp $(W) -pthread -DOS_LINUX -DLEVELDB_PLATFORM_POSIX
PLATFORM_CXXFLAGS= -fno-builtin-memcmp $(W) -pthread -DOS_LINUX -DLEVELDB_PLATFORM_POSIX
PLATFORM_SHARED_CFLAGS=-fPIC
PLATFORM_SHARED_EXT=so
PLATFORM_SHARED_LDFLAGS=-shared -Wl,-soname -Wl,
PLATFORM_SHARED_VERSIONED=true


ATLAS_ROOTDIR      = /home/dhruva/multicore/Atlas
ATLAS_SRCDIR       = $(ATLAS_ROOTDIR)/src
ATLAS_INCDIR       = $(ATLAS_ROOTDIR)/include
ATLAS_INT_INCDIR   = $(ATLAS_SRCDIR)/internal_includes
ATLAS_LIBDIR       = $(ATLAS_ROOTDIR)/lib
ATLAS_TOOLDIR      = $(ATLAS_ROOTDIR)/tools

ATOMIC_OPS_ROOTDIR = /home/dhruva/multicore/libs
ATOMIC_OPS_INCDIR  = $(ATOMIC_OPS_ROOTDIR)/include
ATOMIC_OPS_LIBDIR  = $(ATOMIC_OPS_ROOTDIR)/lib

LIBATLAS           = $(ATLAS_LIBDIR)/libatlas.a

ATLAS_CFLAGS       =   $(COMMON_CFLAGS) -I$(ATLAS_INCDIR) \
                        -I$(ATLAS_INT_INCDIR) -I$(ATOMIC_OPS_INCDIR)

ATLAS_CXXFLAGS       =   $(COMMON_CFLAGS) -I$(ATLAS_INCDIR) \
                        -I$(ATLAS_INT_INCDIR) -I$(ATOMIC_OPS_INCDIR)

ATLAS_LDFLAGS      =   -L$(ATLAS_LIBDIR) -latlas \
                        -L$(ATOMIC_OPS_LIBDIR) -latomic_ops \
                        -lrt -lpthread

