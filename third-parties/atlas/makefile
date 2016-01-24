CC                 := gcc
CPP                := g++

CP                 := /bin/cp

ATLAS_ROOTDIR      := /mnt/ssd/terry/workspace/nvthreads/third-parties/atlas
ATLAS_SRCDIR       := $(ATLAS_ROOTDIR)/src
ATLAS_INCDIR       := $(ATLAS_ROOTDIR)/include
ATLAS_INT_INCDIR   := $(ATLAS_SRCDIR)/internal_includes
ATLAS_LIBDIR       := $(ATLAS_ROOTDIR)/lib
ATLAS_TOOLDIR      := $(ATLAS_ROOTDIR)/tools
EXAMPLEDIR         := $(ATLAS_ROOTDIR)/examples
TESTDIR            := $(ATLAS_ROOTDIR)/tests
EXAMPLE_HT_DIR     := $(EXAMPLEDIR)/c-hashtable-nvm
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

LIBATLAS           := $(ATLAS_LIBDIR)/libatlas.a

SRCS               :=   $(ATLAS_SRCDIR)/alloc/alloc.cpp \
                        $(ATLAS_SRCDIR)/consistency/consistency.cpp \
                        $(ATLAS_SRCDIR)/consistency/helper_driver.cpp \
                        $(ATLAS_SRCDIR)/consistency/logging.cpp \
                        $(ATLAS_SRCDIR)/util/util.cpp \

RECOVERY_SRCS      :=   $(ATLAS_SRCDIR)/recover/recover.cpp \

ATLAS_INCLUDES     :=   $(ATLAS_INCDIR)/atlas_alloc.h \
                        $(ATLAS_INCDIR)/atlas_api.h \

ATLAS_INT_INCLUDES :=   $(ATLAS_INT_INCDIR)/atlas_alloc_priv.h \
			$(ATLAS_INT_INCDIR)/log_alloc.h \
			$(ATLAS_INT_INCDIR)/helper.h \
                        $(ATLAS_INT_INCDIR)/log_structure.h \
                        $(ATLAS_INT_INCDIR)/region_internal.h \
                        $(ATLAS_INT_INCDIR)/util.h \

OBJS               := ${SRCS:.cpp=.o}

RECOVERY_OBJS      := ${RECOVERY_SRCS:.cpp=.o}
RECOVERY_EXE       := $(ATLAS_TOOLDIR)/recover

TRANSIENT_EXAMPLE_SRCS := \
			$(EXAMPLEDIR)/queue_orig.c \
			$(EXAMPLEDIR)/cow_array_list.c \
			$(EXAMPLEDIR)/sll.c \
			$(EXAMPLEDIR)/sll_ll.c \
			$(EXAMPLEDIR)/stores.c \
			$(EXAMPLEDIR)/alarm_clock.c \

NVM_EXAMPLE_SRCS :=     $(EXAMPLEDIR)/queue_nvm.c \
			$(EXAMPLEDIR)/test_memfns.c \
			$(EXAMPLEDIR)/cow_array_list_nvm.c \
			$(EXAMPLEDIR)/sll_nvm.c \
			$(EXAMPLEDIR)/sll_mt.c \
			$(EXAMPLEDIR)/sll_mt_ll.c \
			$(EXAMPLEDIR)/sll_mt_nvm.c \
			$(EXAMPLEDIR)/page_density.c \
			$(EXAMPLEDIR)/stores_nvm.c \
			$(EXAMPLEDIR)/alarm_clock_nvm.c \
			$(EXAMPLE_HT_DIR)/hashtable.c \
			$(EXAMPLE_HT_DIR)/hashtable_itr.c \
			$(EXAMPLE_HT_DIR)/tester.c \

EXAMPLE_SRCS :=         $(TRANSIENT_EXAMPLE_SRCS) $(NVM_EXAMPLE_SRCS)

TRANSIENT_EXAMPLE_OBJS := ${TRANSIENT_EXAMPLE_SRCS:.c=.o}
NVM_EXAMPLE_OBJS := ${NVM_EXAMPLE_SRCS:.c=.o}
EXAMPLE_OBJS       := ${EXAMPLE_SRCS:.c=.o}

EXAMPLE_EXES       :=   $(EXAMPLEDIR)/queue_nvm \
			$(EXAMPLEDIR)/queue_orig \
			$(EXAMPLEDIR)/test_memfns \
			$(EXAMPLEDIR)/cow_array_list \
			$(EXAMPLEDIR)/cow_array_list_nvm \
			$(EXAMPLEDIR)/sll \
			$(EXAMPLEDIR)/sll_ll \
			$(EXAMPLEDIR)/sll_nvm \
			$(EXAMPLEDIR)/sll_mt \
			$(EXAMPLEDIR)/sll_mt_ll \
			$(EXAMPLEDIR)/sll_mt_nvm \
			$(EXAMPLEDIR)/page_density-atlas \
			$(EXAMPLEDIR)/stores \
			$(EXAMPLEDIR)/stores_nvm \
			$(EXAMPLEDIR)/alarm_clock \
			$(EXAMPLEDIR)/alarm_clock_nvm \
			$(EXAMPLE_HT_DIR)/tester \

LD_FLAGS           :=   -L$(ATLAS_LIBDIR) -latlas \
			-L$(ATOMIC_OPS_LIBDIR) -latomic_ops \
			-lrt -lpthread \

AR                 := ar
RANLIB             := ranlib
RM                 := rm -f

.PHONY: default
default: atlas

.PHONY: atlas
atlas: $(LIBATLAS)

.PHONY: clean
clean:
	$(RM) $(LIBATLAS) $(OBJS) $(RECOVERY_OBJS) $(RECOVERY_EXE) \
	$(EXAMPLE_OBJS) $(EXAMPLE_EXES)

.PHONY: clean-nvm-tgts
clean-nvm-tgts:
	$(RM) $(NVM_EXAMPLE_OBJS) $(NVM_EXAMPLE_EXES)

.PHONY: clean_memory
clean_memory:
	$(RM) -rf /dev/shm/regions /dev/shm/__nvm_system_region_table

.PHONY: clean_pmm_memory
clean_pmm_memory:
	$(RM) -r /dev/pmmfs/*

.PHONY: quick-test
quick-test:
	/usr/bin/ruby $(ATLAS_TOOLDIR)/run_tests.rb quick

.PHONY: test
test:
	/usr/bin/ruby $(ATLAS_TOOLDIR)/run_tests.rb full

.PHONY: all-persistent
all-persistent: ATLAS_CFLAGS += -D_ALL_PERSISTENT -DNVM_STATS
all-persistent: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT
all-persistent: APP_CFLAGS += -D_ALL_PERSISTENT
all-persistent: APP_CPPFLAGS += -D_ALL_PERSISTENT
all-persistent: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm page_density-atlas sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-flc
all-persistent-flc: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_FLUSH_LOCAL_COMMIT
all-persistent-flc: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_FLUSH_LOCAL_COMMIT
all-persistent-flc: APP_CFLAGS += -D_ALL_PERSISTENT -D_FLUSH_LOCAL_COMMIT
all-persistent-flc: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_FLUSH_LOCAL_COMMIT
all-persistent-flc: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-flc-stats
all-persistent-flc-stats: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_FLUSH_LOCAL_COMMIT -DNVM_STATS
all-persistent-flc-stats: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_FLUSH_LOCAL_COMMIT -DNVM_STATS
all-persistent-flc-stats: APP_CFLAGS += -D_ALL_PERSISTENT -D_FLUSH_LOCAL_COMMIT -DNVM_STATS
all-persistent-flc-stats: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_FLUSH_LOCAL_COMMIT -DNVM_STATS
all-persistent-flc-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-fgc
all-persistent-fgc: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_FLUSH_GLOBAL_COMMIT
all-persistent-fgc: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_FLUSH_GLOBAL_COMMIT
all-persistent-fgc: APP_CFLAGS += -D_ALL_PERSISTENT -D_FLUSH_GLOBAL_COMMIT
all-persistent-fgc: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_FLUSH_GLOBAL_COMMIT
all-persistent-fgc: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-fgc-stats
all-persistent-fgc-stats: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_FLUSH_GLOBAL_COMMIT -DNVM_STATS
all-persistent-fgc-stats: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_FLUSH_GLOBAL_COMMIT -DNVM_STATS
all-persistent-fgc-stats: APP_CFLAGS += -D_ALL_PERSISTENT -D_FLUSH_GLOBAL_COMMIT -DNVM_STATS
all-persistent-fgc-stats: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_FLUSH_GLOBAL_COMMIT -DNVM_STATS
all-persistent-fgc-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-stats
all-persistent-stats: ATLAS_CFLAGS += -D_ALL_PERSISTENT -DNVM_STATS
all-persistent-stats: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -DNVM_STATS
all-persistent-stats: APP_CFLAGS += -D_ALL_PERSISTENT -DNVM_STATS
all-persistent-stats: APP_CPPFLAGS += -D_ALL_PERSISTENT -DNVM_STATS
all-persistent-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-table-flush
use-table-flush: ATLAS_CFLAGS += -D_USE_TABLE_FLUSH
use-table-flush: ATLAS_CPPFLAGS += -D_USE_TABLE_FLUSH
use-table-flush: APP_CFLAGS += -D_USE_TABLE_FLUSH
use-table-flush: APP_CPPFLAGS += -D_USE_TABLE_FLUSH
use-table-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-table-flush-trace
use-table-flush-trace: ATLAS_CFLAGS += -D_USE_TABLE_FLUSH -D_NVM_TRACE
use-table-flush-trace: ATLAS_CPPFLAGS += -D_USE_TABLE_FLUSH -D_NVM_TRACE
use-table-flush-trace: APP_CFLAGS += -D_USE_TABLE_FLUSH -D_NVM_TRACE
use-table-flush-trace: APP_CPPFLAGS += -D_USE_TABLE_FLUSH -D_NVM_TRACE
use-table-flush-trace: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-table-flush-disable-flushes
use-table-flush-disable-flushes: ATLAS_CFLAGS += -D_USE_TABLE_FLUSH -DDISABLE_FLUSHES
use-table-flush-disable-flushes: ATLAS_CPPFLAGS += -D_USE_TABLE_FLUSH -DDISABLE_FLUSHES
use-table-flush-disable-flushes: APP_CFLAGS += -D_USE_TABLE_FLUSH -DDISABLE_FLUSHES
use-table-flush-disable-flushes: APP_CPPFLAGS += -D_USE_TABLE_FLUSH -DDISABLE_FLUSHES
use-table-flush-disable-flushes: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-table-flush-disable-log-flush
use-table-flush-disable-log-flush: ATLAS_CFLAGS += -D_USE_TABLE_FLUSH -D_DISABLE_LOG_FLUSH
use-table-flush-disable-log-flush: ATLAS_CPPFLAGS += -D_USE_TABLE_FLUSH -D_DISABLE_LOG_FLUSH
use-table-flush-disable-log-flush: APP_CFLAGS += -D_USE_TABLE_FLUSH -D_DISABLE_LOG_FLUSH
use-table-flush-disable-log-flush: APP_CPPFLAGS += -D_USE_TABLE_FLUSH -D_DISABLE_LOG_FLUSH
use-table-flush-disable-log-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-table-flush-stats
use-table-flush-stats: ATLAS_CFLAGS += -D_USE_TABLE_FLUSH -DNVM_STATS
use-table-flush-stats: ATLAS_CPPFLAGS += -D_USE_TABLE_FLUSH -DNVM_STATS
use-table-flush-stats: APP_CFLAGS += -D_USE_TABLE_FLUSH -DNVM_STATS
use-table-flush-stats: APP_CPPFLAGS += -D_USE_TABLE_FLUSH -DNVM_STATS
use-table-flush-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-use-table-flush
all-persistent-use-table-flush: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH
all-persistent-use-table-flush: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH
all-persistent-use-table-flush: APP_CFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH
all-persistent-use-table-flush: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH
all-persistent-use-table-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-use-table-flush-profile-ht
all-persistent-use-table-flush-profile-ht: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -D_PROFILE_HT
all-persistent-use-table-flush-profile-ht: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -D_PROFILE_HT
all-persistent-use-table-flush-profile-ht: APP_CFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -D_PROFILE_HT
all-persistent-use-table-flush-profile-ht: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -D_PROFILE_HT
all-persistent-use-table-flush-profile-ht: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-use-table-flush-stats
all-persistent-use-table-flush-stats: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -DNVM_STATS
all-persistent-use-table-flush-stats: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -DNVM_STATS
all-persistent-use-table-flush-stats: APP_CFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -DNVM_STATS
all-persistent-use-table-flush-stats: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -DNVM_STATS
all-persistent-use-table-flush-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-movnt
use-movnt: ATLAS_CFLAGS += -D_USE_MOVNT
use-movnt: ATLAS_CPPFLAGS += -D_USE_MOVNT
use-movnt: APP_CFLAGS += -D_USE_MOVNT
use-movnt: APP_CPPFLAGS += -D_USE_MOVNT
use-movnt: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-use-movnt
all-persistent-use-movnt: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_USE_MOVNT
all-persistent-use-movnt: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_MOVNT
all-persistent-use-movnt: APP_CFLAGS += -D_ALL_PERSISTENT -D_USE_MOVNT
all-persistent-use-movnt: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_MOVNT
all-persistent-use-movnt: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-movnt-use-table-flush
use-movnt-use-table-flush: ATLAS_CFLAGS += -D_USE_MOVNT -D_USE_TABLE_FLUSH 
use-movnt-use-table-flush: ATLAS_CPPFLAGS += -D_USE_MOVNT -D_USE_TABLE_FLUSH 
use-movnt-use-table-flush: APP_CFLAGS += -D_USE_MOVNT -D_USE_TABLE_FLUSH 
use-movnt-use-table-flush: APP_CPPFLAGS += -D_USE_MOVNT -D_USE_TABLE_FLUSH 
use-movnt-use-table-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-use-movnt-use-table-flush
all-persistent-use-movnt-use-table-flush: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_USE_MOVNT -D_USE_TABLE_FLUSH 
all-persistent-use-movnt-use-table-flush: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_MOVNT -D_USE_TABLE_FLUSH 
all-persistent-use-movnt-use-table-flush: APP_CFLAGS += -D_ALL_PERSISTENT -D_USE_MOVNT -D_USE_TABLE_FLUSH 
all-persistent-use-movnt-use-table-flush: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_MOVNT -D_USE_TABLE_FLUSH 
all-persistent-use-movnt-use-table-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-movnt-no-sfence
use-movnt-no-sfence: ATLAS_CFLAGS += -D_USE_MOVNT -D_NO_SFENCE
use-movnt-no-sfence: ATLAS_CPPFLAGS += -D_USE_MOVNT -D_NO_SFENCE
use-movnt-no-sfence: APP_CFLAGS += -D_USE_MOVNT -D_NO_SFENCE
use-movnt-no-sfence: APP_CPPFLAGS += -D_USE_MOVNT -D_NO_SFENCE
use-movnt-no-sfence: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-movnt-no-sfence-no-flushes
use-movnt-no-sfence-no-flushes: ATLAS_CFLAGS += -D_USE_MOVNT -D_NO_SFENCE -DDISABLE_FLUSHES
use-movnt-no-sfence-no-flushes: ATLAS_CPPFLAGS += -D_USE_MOVNT -D_NO_SFENCE -DDISABLE_FLUSHES
use-movnt-no-sfence-no-flushes: APP_CFLAGS += -D_USE_MOVNT -D_NO_SFENCE -DDISABLE_FLUSHES
use-movnt-no-sfence-no-flushes: APP_CPPFLAGS += -D_USE_MOVNT -D_NO_SFENCE -DDISABLE_FLUSHES
use-movnt-no-sfence-no-flushes: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: disable-helper
disable-helper: ATLAS_CFLAGS += -D_DISABLE_HELPER
disable-helper: ATLAS_CPPFLAGS += -D_DISABLE_HELPER
disable-helper: APP_CFLAGS += -D_DISABLE_HELPER
disable-helper: APP_CPPFLAGS += -D_DISABLE_HELPER
disable-helper: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-use-table-flush-disable-helper
all-persistent-use-table-flush-disable-helper: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -D_DISABLE_HELPER
all-persistent-use-table-flush-disable-helper: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -D_DISABLE_HELPER
all-persistent-use-table-flush-disable-helper: APP_CFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -D_DISABLE_HELPER
all-persistent-use-table-flush-disable-helper: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_USE_TABLE_FLUSH -D_DISABLE_HELPER
all-persistent-use-table-flush-disable-helper: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: log-flush-opt
log-flush-opt: ATLAS_CFLAGS += -D_LOG_FLUSH_OPT
log-flush-opt: ATLAS_CPPFLAGS += -D_LOG_FLUSH_OPT
log-flush-opt: APP_CFLAGS += -D_LOG_FLUSH_OPT
log-flush-opt: APP_CPPFLAGS += -D_LOG_FLUSH_OPT
log-flush-opt: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: always-log
always-log: ATLAS_CFLAGS += -D_ALWAYS_LOG
always-log: ATLAS_CPPFLAGS += -D_ALWAYS_LOG
always-log: APP_CFLAGS += -D_ALWAYS_LOG
always-log: APP_CPPFLAGS += -D_ALWAYS_LOG
always-log: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-table-flush-always-log
use-table-flush-always-log: ATLAS_CFLAGS += -D_ALWAYS_LOG -D_USE_TABLE_FLUSH 
use-table-flush-always-log: ATLAS_CPPFLAGS += -D_ALWAYS_LOG -D_USE_TABLE_FLUSH 
use-table-flush-always-log: APP_CFLAGS += -D_ALWAYS_LOG -D_USE_TABLE_FLUSH 
use-table-flush-always-log: APP_CPPFLAGS += -D_ALWAYS_LOG -D_USE_TABLE_FLUSH 
use-table-flush-always-log: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-table-flush-always-log-always-map
use-table-flush-always-log-always-map: ATLAS_CFLAGS += -D_ALWAYS_LOG -D_USE_TABLE_FLUSH -D_ALWAYS_MAP
use-table-flush-always-log-always-map: ATLAS_CPPFLAGS += -D_ALWAYS_LOG -D_USE_TABLE_FLUSH -D_ALWAYS_MAP
use-table-flush-always-log-always-map: APP_CFLAGS += -D_ALWAYS_LOG -D_USE_TABLE_FLUSH -D_ALWAYS_MAP
use-table-flush-always-log-always-map: APP_CPPFLAGS += -D_ALWAYS_LOG -D_USE_TABLE_FLUSH -D_ALWAYS_MAP
use-table-flush-always-log-always-map: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: always-log-trace
always-log-trace: ATLAS_CFLAGS += -D_ALWAYS_LOG -D_NVM_TRACE
always-log-trace: ATLAS_CPPFLAGS += -D_ALWAYS_LOG -D_NVM_TRACE
always-log-trace: APP_CFLAGS += -D_ALWAYS_LOG -D_NVM_TRACE
always-log-trace: APP_CPPFLAGS += -D_ALWAYS_LOG -D_NVM_TRACE
always-log-trace: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-always-log
all-persistent-always-log: ATLAS_CFLAGS += -D_ALWAYS_LOG -D_ALL_PERSISTENT 
all-persistent-always-log: ATLAS_CPPFLAGS += -D_ALWAYS_LOG -D_ALL_PERSISTENT
all-persistent-always-log: APP_CFLAGS += -D_ALWAYS_LOG -D_ALL_PERSISTENT 
all-persistent-always-log: APP_CPPFLAGS += -D_ALWAYS_LOG -D_ALL_PERSISTENT
all-persistent-always-log: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-always-log-disable-flush
all-persistent-always-log-disable-flush: ATLAS_CFLAGS += -D_ALWAYS_LOG -D_ALL_PERSISTENT -DDISABLE_FLUSHES
all-persistent-always-log-disable-flush: ATLAS_CPPFLAGS += -D_ALWAYS_LOG -D_ALL_PERSISTENT -DDISABLE_FLUSHES
all-persistent-always-log-disable-flush: APP_CFLAGS += -D_ALWAYS_LOG -D_ALL_PERSISTENT -DDISABLE_FLUSHES
all-persistent-always-log-disable-flush: APP_CPPFLAGS += -D_ALWAYS_LOG -D_ALL_PERSISTENT -DDISABLE_FLUSHES
all-persistent-always-log-disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: opt-uniq-loc
opt-uniq-loc: ATLAS_CFLAGS += -D_OPT_UNIQ_LOC
opt-uniq-loc: ATLAS_CPPFLAGS += -D_OPT_UNIQ_LOC
opt-uniq-loc: APP_CFLAGS += -D_OPT_UNIQ_LOC
opt-uniq-loc: APP_CPPFLAGS += -D_OPT_UNIQ_LOC
opt-uniq-loc: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: disable-log-flush
disable-log-flush: ATLAS_CFLAGS += -D_DISABLE_LOG_FLUSH
disable-log-flush: ATLAS_CPPFLAGS += -D_DISABLE_LOG_FLUSH
disable-log-flush: APP_CFLAGS += -D_DISABLE_LOG_FLUSH
disable-log-flush: APP_CPPFLAGS += -D_DISABLE_LOG_FLUSH
disable-log-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: disable-data-flush
disable-data-flush: ATLAS_CFLAGS += -D_DISABLE_DATA_FLUSH
disable-data-flush: ATLAS_CPPFLAGS += -D_DISABLE_DATA_FLUSH
disable-data-flush: APP_CFLAGS += -D_DISABLE_DATA_FLUSH
disable-data-flush: APP_CPPFLAGS += -D_DISABLE_DATA_FLUSH
disable-data-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: disable-data-flush-stats
disable-data-flush-stats: ATLAS_CFLAGS += -D_DISABLE_DATA_FLUSH -DNVM_STATS
disable-data-flush-stats: ATLAS_CPPFLAGS += -D_DISABLE_DATA_FLUSH -DNVM_STATS
disable-data-flush-stats: APP_CFLAGS += -D_DISABLE_DATA_FLUSH -DNVM_STATS
disable-data-flush-stats: APP_CPPFLAGS += -D_DISABLE_DATA_FLUSH -DNVM_STATS
disable-data-flush-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: always-log-disable-flush
always-log-disable-flush: ATLAS_CFLAGS += -D_ALWAYS_LOG -DDISABLE_FLUSHES
always-log-disable-flush: ATLAS_CPPFLAGS += -D_ALWAYS_LOG -DDISABLE_FLUSHES
always-log-disable-flush: APP_CFLAGS += -D_ALWAYS_LOG -DDISABLE_FLUSHES
always-log-disable-flush: APP_CPPFLAGS += -D_ALWAYS_LOG -DDISABLE_FLUSHES
always-log-disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: log-with-nvm-alloc
log-with-nvm-alloc: ATLAS_CFLAGS += -D_LOG_WITH_NVM_ALLOC
log-with-nvm-alloc: ATLAS_CPPFLAGS += -D_LOG_WITH_NVM_ALLOC
log-with-nvm-alloc: APP_CFLAGS += -D_LOG_WITH_NVM_ALLOC
log-with-nvm-alloc: APP_CPPFLAGS += -D_LOG_WITH_NVM_ALLOC
log-with-nvm-alloc: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: log-with-nvm-alloc-stats
log-with-nvm-alloc-stats: ATLAS_CFLAGS += -D_LOG_WITH_NVM_ALLOC -DNVM_STATS
log-with-nvm-alloc-stats: ATLAS_CPPFLAGS += -D_LOG_WITH_NVM_ALLOC -DNVM_STATS
log-with-nvm-alloc-stats: APP_CFLAGS += -D_LOG_WITH_NVM_ALLOC -DNVM_STATS
log-with-nvm-alloc-stats: APP_CPPFLAGS += -D_LOG_WITH_NVM_ALLOC -DNVM_STATS
log-with-nvm-alloc-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: log-with-malloc
log-with-malloc: ATLAS_CFLAGS += -D_LOG_WITH_MALLOC
log-with-malloc: ATLAS_CPPFLAGS += -D_LOG_WITH_MALLOC
log-with-malloc: APP_CFLAGS += -D_LOG_WITH_MALLOC
log-with-malloc: APP_CPPFLAGS += -D_LOG_WITH_MALLOC
log-with-malloc: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: log-with-malloc-only
log-with-malloc-only: ATLAS_CFLAGS += -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES
log-with-malloc-only: ATLAS_CPPFLAGS += -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES
log-with-malloc-only: APP_CFLAGS += -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES
log-with-malloc-only: APP_CPPFLAGS += -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES
log-with-malloc-only: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-log-with-malloc-only
all-persistent-log-with-malloc-only: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES
all-persistent-log-with-malloc-only: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES
all-persistent-log-with-malloc-only: APP_CFLAGS += -D_ALL_PERSISTENT -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES
all-persistent-log-with-malloc-only: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES
all-persistent-log-with-malloc-only: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: all-persistent-log-with-malloc-only-stats
all-persistent-log-with-malloc-only-stats: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES -DNVM_STATS
all-persistent-log-with-malloc-only-stats: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES -DNVM_STATS
all-persistent-log-with-malloc-only-stats: APP_CFLAGS += -D_ALL_PERSISTENT -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES -DNVM_STATS
all-persistent-log-with-malloc-only-stats: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_LOG_WITH_MALLOC -DDISABLE_FLUSHES -DNVM_STATS
all-persistent-log-with-malloc-only-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: log-with-malloc-stats
log-with-malloc-stats: ATLAS_CFLAGS += -D_LOG_WITH_MALLOC -DNVM_STATS
log-with-malloc-stats: ATLAS_CPPFLAGS += -D_LOG_WITH_MALLOC -DNVM_STATS
log-with-malloc-stats: APP_CFLAGS += -D_LOG_WITH_MALLOC -DNVM_STATS
log-with-malloc-stats: APP_CPPFLAGS += -D_LOG_WITH_MALLOC -DNVM_STATS
log-with-malloc-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-malloc
use-malloc: ATLAS_CFLAGS += -D_USE_MALLOC
use-malloc: ATLAS_CPPFLAGS += -D_USE_MALLOC
use-malloc: APP_CFLAGS += -D_USE_MALLOC
use-malloc: APP_CPPFLAGS += -D_USE_MALLOC
use-malloc: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: use-malloc-stats
use-malloc-stats: ATLAS_CFLAGS += -D_USE_MALLOC -DNVM_STATS
use-malloc-stats: ATLAS_CPPFLAGS += -D_USE_MALLOC -DNVM_STATS
use-malloc-stats: APP_CFLAGS += -D_USE_MALLOC -DNVM_STATS
use-malloc-stats: APP_CPPFLAGS += -D_USE_MALLOC -DNVM_STATS
use-malloc-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: always-log-stats
always-log-stats: ATLAS_CFLAGS += -D_ALWAYS_LOG -DNVM_STATS
always-log-stats: ATLAS_CPPFLAGS += -D_ALWAYS_LOG -DNVM_STATS
always-log-stats: APP_CFLAGS += -D_ALWAYS_LOG -DNVM_STATS
always-log-stats: APP_CPPFLAGS += -D_ALWAYS_LOG -DNVM_STATS
always-log-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: force-fail
force-fail: ATLAS_CFLAGS += -D_FORCE_FAIL
force-fail: ATLAS_CPPFLAGS += -D_FORCE_FAIL
force-fail: APP_CFLAGS += -D_FORCE_FAIL
force-fail: APP_CPPFLAGS += -D_FORCE_FAIL
force-fail: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: force-fail-trace
force-fail-trace: ATLAS_CFLAGS += -D_FORCE_FAIL -D_NVM_TRACE
force-fail-trace: ATLAS_CPPFLAGS += -D_FORCE_FAIL -D_NVM_TRACE
force-fail-trace: APP_CFLAGS += -D_FORCE_FAIL -D_NVM_TRACE
force-fail-trace: APP_CPPFLAGS += -D_FORCE_FAIL -D_NVM_TRACE
force-fail-trace: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: force-fail-verbose-trace
force-fail-verbose-trace: ATLAS_CFLAGS += -D_FORCE_FAIL -D_NVM_TRACE -D_NVM_VERBOSE_TRACE
force-fail-verbose-trace: ATLAS_CPPFLAGS += -D_FORCE_FAIL -D_NVM_TRACE -D_NVM_VERBOSE_TRACE
force-fail-verbose-trace: APP_CFLAGS += -D_FORCE_FAIL -D_NVM_TRACE -D_NVM_VERBOSE_TRACE
force-fail-verbose-trace: APP_CPPFLAGS += -D_FORCE_FAIL -D_NVM_TRACE -D_NVM_VERBOSE_TRACE
force-fail-verbose-trace: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: flc
flc: ATLAS_CFLAGS += -D_FLUSH_LOCAL_COMMIT
flc: ATLAS_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT
flc: APP_CFLAGS += -D_FLUSH_LOCAL_COMMIT
flc: APP_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT
flc: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm 

.PHONY: flc-always-log
flc-always-log: ATLAS_CFLAGS += -D_FLUSH_LOCAL_COMMIT -D_ALWAYS_LOG
flc-always-log: ATLAS_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -D_ALWAYS_LOG
flc-always-log: APP_CFLAGS += -D_FLUSH_LOCAL_COMMIT -D_ALWAYS_LOG
flc-always-log: APP_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -D_ALWAYS_LOG
flc-always-log: default recover queue_nvm queue_orig test_memfns \
                cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
                sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: flc-always-log-stats
flc-always-log-stats: ATLAS_CFLAGS += -D_FLUSH_LOCAL_COMMIT -D_ALWAYS_LOG -DNVM_STATS
flc-always-log-stats: ATLAS_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -D_ALWAYS_LOG -DNVM_STATS
flc-always-log-stats: APP_CFLAGS += -D_FLUSH_LOCAL_COMMIT -D_ALWAYS_LOG -DNVM_STATS
flc-always-log-stats: APP_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -D_ALWAYS_LOG -DNVM_STATS
flc-always-log-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: fgc
fgc: ATLAS_CFLAGS += -D_FLUSH_GLOBAL_COMMIT
fgc: ATLAS_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT
fgc: APP_CFLAGS += -D_FLUSH_GLOBAL_COMMIT
fgc: APP_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT
fgc: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt  sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: flc-stats
flc-stats: ATLAS_CFLAGS += -D_FLUSH_LOCAL_COMMIT -DNVM_STATS
flc-stats: ATLAS_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -DNVM_STATS
flc-stats: APP_CFLAGS += -D_FLUSH_LOCAL_COMMIT -DNVM_STATS
flc-stats: APP_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -DNVM_STATS
flc-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: fgc-stats
fgc-stats: ATLAS_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -DNVM_STATS
fgc-stats: ATLAS_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -DNVM_STATS
fgc-stats: APP_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -DNVM_STATS
fgc-stats: APP_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -DNVM_STATS
fgc-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: stats
stats: ATLAS_CFLAGS += -DNVM_STATS
stats: ATLAS_CPPFLAGS += -DNVM_STATS
stats: APP_CFLAGS += -DNVM_STATS
stats: APP_CPPFLAGS += -DNVM_STATS
stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: trace
trace: ATLAS_CFLAGS += -D_NVM_TRACE
trace: ATLAS_CPPFLAGS += -D_NVM_TRACE
trace: APP_CFLAGS += -D_NVM_TRACE
trace: APP_CPPFLAGS += -D_NVM_TRACE
trace: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: verbose-trace
verbose-trace: ATLAS_CFLAGS += -D_NVM_TRACE -D_NVM_VERBOSE_TRACE
verbose-trace: ATLAS_CPPFLAGS += -D_NVM_TRACE -D_NVM_VERBOSE_TRACE
verbose-trace: APP_CFLAGS += -D_NVM_TRACE -D_NVM_VERBOSE_TRACE
verbose-trace: APP_CPPFLAGS += -D_NVM_TRACE -D_NVM_VERBOSE_TRACE
verbose-trace: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: disable-flush
disable-flush: ATLAS_CFLAGS += -DDISABLE_FLUSHES
disable-flush: ATLAS_CPPFLAGS += -DDISABLE_FLUSHES
disable-flush: APP_CFLAGS += -DDISABLE_FLUSHES
disable-flush: APP_CPPFLAGS += -DDISABLE_FLUSHES
disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: disable-flush-stats
disable-flush-stats: ATLAS_CFLAGS += -DDISABLE_FLUSHES -DNVM_STATS
disable-flush-stats: ATLAS_CPPFLAGS += -DDISABLE_FLUSHES -DNVM_STATS
disable-flush-stats: APP_CFLAGS += -DDISABLE_FLUSHES -DNVM_STATS
disable-flush-stats: APP_CPPFLAGS += -DDISABLE_FLUSHES -DNVM_STATS
disable-flush-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: all-persistent-disable-flush
all-persistent-disable-flush: ATLAS_CFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES
all-persistent-disable-flush: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES
all-persistent-disable-flush: APP_CFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES
all-persistent-disable-flush: APP_CPPFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES
all-persistent-disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: all-persistent-disable-flush-stats
all-persistent-disable-flush-stats: ATLAS_CFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES -DNVM_STATS
all-persistent-disable-flush-stats: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES -DNVM_STATS
all-persistent-disable-flush-stats: APP_CFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES -DNVM_STATS
all-persistent-disable-flush-stats: APP_CPPFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES -DNVM_STATS
all-persistent-disable-flush-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: all-persistent-srrf-disable-flush
all-persistent-srrf-disable-flush: ATLAS_CFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES -D_SRRF
all-persistent-srrf-disable-flush: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES -D_SRRF
all-persistent-srrf-disable-flush: APP_CFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES -D_SRRF
all-persistent-srrf-disable-flush: APP_CPPFLAGS += -D_ALL_PERSISTENT -DDISABLE_FLUSHES -D_SRRF
all-persistent-srrf-disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: srrf-disable-flush
srrf-disable-flush: ATLAS_CFLAGS += -DDISABLE_FLUSHES -D_SRRF
srrf-disable-flush: ATLAS_CPPFLAGS += -DDISABLE_FLUSHES -D_SRRF
srrf-disable-flush: APP_CFLAGS +=  -DDISABLE_FLUSHES -D_SRRF
srrf-disable-flush: APP_CPPFLAGS += -DDISABLE_FLUSHES -D_SRRF
srrf-disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: no-nest-disable-flush
no-nest-disable-flush: ATLAS_CFLAGS += -D_NO_NEST -DDISABLE_FLUSHES
no-nest-disable-flush: ATLAS_CPPFLAGS += -D_NO_NEST -DDISABLE_FLUSHES
no-nest-disable-flush: APP_CFLAGS += -D_NO_NEST -DDISABLE_FLUSHES
no-nest-disable-flush: APP_CPPFLAGS += -D_NO_NEST -DDISABLE_FLUSHES
no-nest-disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: all-persistent-no-nest-disable-flush
all-persistent-no-nest-disable-flush: ATLAS_CFLAGS += -D_ALL_PERSISTENT -D_NO_NEST -DDISABLE_FLUSHES
all-persistent-no-nest-disable-flush: ATLAS_CPPFLAGS += -D_ALL_PERSISTENT -D_NO_NEST -DDISABLE_FLUSHES
all-persistent-no-nest-disable-flush: APP_CFLAGS += -D_ALL_PERSISTENT -D_NO_NEST -DDISABLE_FLUSHES
all-persistent-no-nest-disable-flush: APP_CPPFLAGS += -D_ALL_PERSISTENT -D_NO_NEST -DDISABLE_FLUSHES
all-persistent-no-nest-disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: flc-disable-flush
flc-disable-flush: ATLAS_CFLAGS += -D_FLUSH_LOCAL_COMMIT -DDISABLE_FLUSHES
flc-disable-flush: ATLAS_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -DDISABLE_FLUSHES
flc-disable-flush: APP_CFLAGS += -D_FLUSH_LOCAL_COMMIT -DDISABLE_FLUSHES
flc-disable-flush: APP_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -DDISABLE_FLUSHES
flc-disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: fgc-disable-flush
fgc-disable-flush: ATLAS_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -DDISABLE_FLUSHES
fgc-disable-flush: ATLAS_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -DDISABLE_FLUSHES
fgc-disable-flush: APP_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -DDISABLE_FLUSHES
fgc-disable-flush: APP_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -DDISABLE_FLUSHES
fgc-disable-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: fgc-disable-log-flush
fgc-disable-log-flush: ATLAS_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_LOG_FLUSH
fgc-disable-log-flush: ATLAS_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_LOG_FLUSH
fgc-disable-log-flush: APP_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_LOG_FLUSH
fgc-disable-log-flush: APP_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_LOG_FLUSH
fgc-disable-log-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: fgc-disable-data-flush
fgc-disable-data-flush: ATLAS_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_DATA_FLUSH
fgc-disable-data-flush: ATLAS_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_DATA_FLUSH
fgc-disable-data-flush: APP_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_DATA_FLUSH
fgc-disable-data-flush: APP_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_DATA_FLUSH
fgc-disable-data-flush: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: fgc-disable-log-flush-stats
fgc-disable-log-flush-stats: ATLAS_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_LOG_FLUSH -DNVM_STATS
fgc-disable-log-flush-stats: ATLAS_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_LOG_FLUSH -DNVM_STATS
fgc-disable-log-flush-stats: APP_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_LOG_FLUSH -DNVM_STATS
fgc-disable-log-flush-stats: APP_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_LOG_FLUSH -DNVM_STATS
fgc-disable-log-flush-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: fgc-disable-data-flush-stats
fgc-disable-data-flush-stats: ATLAS_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_DATA_FLUSH -DNVM_STATS
fgc-disable-data-flush-stats: ATLAS_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_DATA_FLUSH -DNVM_STATS
fgc-disable-data-flush-stats: APP_CFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_DATA_FLUSH -DNVM_STATS
fgc-disable-data-flush-stats: APP_CPPFLAGS += -D_FLUSH_GLOBAL_COMMIT -D_DISABLE_DATA_FLUSH -DNVM_STATS
fgc-disable-data-flush-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm \
     sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: flc-disable-flush-stats
flc-disable-flush-stats: ATLAS_CFLAGS += -D_FLUSH_LOCAL_COMMIT -DDISABLE_FLUSHES -DNVM_STATS
flc-disable-flush-stats: ATLAS_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -DDISABLE_FLUSHES -DNVM_STATS
flc-disable-flush-stats: APP_CFLAGS += -D_FLUSH_LOCAL_COMMIT -DDISABLE_FLUSHES -DNVM_STATS
flc-disable-flush-stats: APP_CPPFLAGS += -D_FLUSH_LOCAL_COMMIT -DDISABLE_FLUSHES -DNVM_STATS
flc-disable-flush-stats: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm

.PHONY: pmm
pmm: ATLAS_CFLAGS += -DPMM_OS
pmm: ATLAS_CPPFLAGS += -DPMM_OS
pmm: default recover queue_nvm queue_orig test_memfns \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm sll_mt sll_mt_ll sll_mt_nvm tester

.PHONY: all
all: default recover queue_nvm queue_orig test_memfns page_density-atlas \
     cow_array_list cow_array_list_nvm sll sll_ll sll_nvm sll_mt sll_mt_ll sll_mt_nvm tester stores stores_nvm alarm_clock alarm_clock_nvm

$(EXAMPLEDIR)/queue_orig.o : $(EXAMPLEDIR)/queue_orig.c
	$(CC) $(COMMON_CPPFLAGS) -pthread -c $< -o $@

$(EXAMPLEDIR)/cow_array_list.o : $(EXAMPLEDIR)/cow_array_list.c
	$(CC) $(COMMON_CPPFLAGS) -pthread -c $< -o $@

$(EXAMPLEDIR)/sll.o : $(EXAMPLEDIR)/sll.c
	$(CPP) $(COMMON_CPPFLAGS) -c $< -o $@

$(EXAMPLEDIR)/sll_ll.o : $(EXAMPLEDIR)/sll_ll.c $(ATLAS_INCLUDES) \
			 $(ATLAS_INT_INCLUDES)
	$(CPP) $(APP_CPPFLAGS) -c $< -o $@

$(EXAMPLEDIR)/stores.o : $(EXAMPLEDIR)/stores.c $(ATLAS_INCLUDES) \
			 $(ATLAS_INT_INCLUDES)
	$(CPP) $(APP_CPPFLAGS) -c $< -o $@

$(EXAMPLEDIR)/sll_nvm.o : $(EXAMPLEDIR)/sll_nvm.c $(ATLAS_INCLUDES) \
			 $(ATLAS_INT_INCLUDES)
	$(CPP) $(APP_CPPFLAGS) -c $< -o $@
	echo $(CPP) $(APP_CPPFLAGS) -c $< -o $@

$(EXAMPLEDIR)/page_density-atlas.o : $(EXAMPLEDIR)/page_density.c $(ATLAS_INCLUDES) \
			 $(ATLAS_INT_INCLUDES)
	$(CPP) $(APP_CPPFLAGS) -c $< -o $@
	echo $(CPP) $(APP_CPPFLAGS) -c $< -o $@


$(EXAMPLEDIR)/stores_nvm.o : $(EXAMPLEDIR)/stores_nvm.c $(ATLAS_INCLUDES) \
			 $(ATLAS_INT_INCLUDES)
	$(CPP) $(APP_CPPFLAGS) -c $< -o $@

$(EXAMPLEDIR)/sll_mt.o : $(EXAMPLEDIR)/sll_mt.c
	$(CPP) $(COMMON_CPPFLAGS) -pthread -c $< -o $@

$(EXAMPLEDIR)/sll_mt_ll.o : $(EXAMPLEDIR)/sll_mt_ll.c $(ATLAS_INCLUDES) \
			 $(ATLAS_INT_INCLUDES)
	$(CPP) $(APP_CPPFLAGS) -pthread -c $< -o $@


$(EXAMPLEDIR)/sll_mt_nvm.o : $(EXAMPLEDIR)/sll_mt_nvm.c $(ATLAS_INCLUDES) \
			 $(ATLAS_INT_INCLUDES)
	$(CPP) $(APP_CPPFLAGS) -pthread -c $< -o $@

$(EXAMPLEDIR)/alarm_clock.o : $(EXAMPLEDIR)/alarm_clock.c
	$(CC) $(COMMON_CPPFLAGS) -pthread -c $< -o $@

%.o: %.cpp $(ATLAS_INCLUDES) $(ATLAS_INT_INCLUDES)
	$(CPP) $(ATLAS_CPPFLAGS) -c $< -o $@

%.o: %.c $(ATLAS_INCLUDES) $(ATLAS_INT_INCLUDES) $(LIBATLAS)
	$(CC) $(APP_CFLAGS) -c $< -o $@

$(LIBATLAS): $(OBJS)
	$(AR) cru $@ $^
	$(RANLIB) $@

recover: $(RECOVERY_EXE)
$(RECOVERY_EXE): $(RECOVERY_OBJS) $(LIBATLAS)
	$(CPP) $^ $(LD_FLAGS) -o $@

queue_nvm: $(EXAMPLEDIR)/queue_nvm
$(EXAMPLEDIR)/queue_nvm: $(EXAMPLEDIR)/queue_nvm.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/queue_nvm $(TESTDIR)

cow_array_list_nvm: $(EXAMPLEDIR)/cow_array_list_nvm
$(EXAMPLEDIR)/cow_array_list_nvm: $(EXAMPLEDIR)/cow_array_list_nvm.o \
				  $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/cow_array_list_nvm $(TESTDIR)

test_memfns: $(EXAMPLEDIR)/test_memfns 
$(EXAMPLEDIR)/test_memfns: $(EXAMPLEDIR)/test_memfns.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/test_memfns $(TESTDIR)

queue_orig: $(EXAMPLEDIR)/queue_orig
$(EXAMPLEDIR)/queue_orig: $(EXAMPLEDIR)/queue_orig.o
	$(CC) $< -lpthread -o $@
	$(CP) $(EXAMPLEDIR)/queue_orig $(TESTDIR)

cow_array_list: $(EXAMPLEDIR)/cow_array_list
$(EXAMPLEDIR)/cow_array_list: $(EXAMPLEDIR)/cow_array_list.o
	$(CC) $< -lpthread -o $@
	$(CP) $(EXAMPLEDIR)/cow_array_list $(TESTDIR)

sll: $(EXAMPLEDIR)/sll
$(EXAMPLEDIR)/sll: $(EXAMPLEDIR)/sll.o
	$(CPP) $< -o $@
	$(CP) $(EXAMPLEDIR)/sll $(TESTDIR)

sll_ll: $(EXAMPLEDIR)/sll_ll
$(EXAMPLEDIR)/sll_ll: $(EXAMPLEDIR)/sll_ll.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/sll_ll $(TESTDIR)

stores: $(EXAMPLEDIR)/stores
$(EXAMPLEDIR)/stores: $(EXAMPLEDIR)/stores.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/stores $(TESTDIR)

sll_nvm: $(EXAMPLEDIR)/sll_nvm
$(EXAMPLEDIR)/sll_nvm: $(EXAMPLEDIR)/sll_nvm.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/sll_nvm $(TESTDIR)

stores_nvm: $(EXAMPLEDIR)/stores_nvm
$(EXAMPLEDIR)/stores_nvm: $(EXAMPLEDIR)/stores_nvm.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/stores_nvm $(TESTDIR)

sll_mt: $(EXAMPLEDIR)/sll_mt
$(EXAMPLEDIR)/sll_mt: $(EXAMPLEDIR)/sll_mt.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/sll_mt $(TESTDIR)

page_density-atlas: $(EXAMPLEDIR)/page_density-atlas
$(EXAMPLEDIR)/page_density-atlas: $(EXAMPLEDIR)/page_density-atlas.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/page_density-atlas $(TESTDIR)

sll_mt_ll: $(EXAMPLEDIR)/sll_mt_ll
$(EXAMPLEDIR)/sll_mt_ll: $(EXAMPLEDIR)/sll_mt_ll.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/sll_mt_ll $(TESTDIR)

sll_mt_nvm: $(EXAMPLEDIR)/sll_mt_nvm
$(EXAMPLEDIR)/sll_mt_nvm: $(EXAMPLEDIR)/sll_mt_nvm.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/sll_mt_nvm $(TESTDIR)

tester: $(EXAMPLE_HT_DIR)/tester
$(EXAMPLE_HT_DIR)/tester: $(EXAMPLE_HT_DIR)/tester.o \
			  $(EXAMPLE_HT_DIR)/hashtable.o \
			  $(EXAMPLE_HT_DIR)/hashtable_itr.o \
			  $(LIBATLAS)
	$(CPP) $^ $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLE_HT_DIR)/tester $(TESTDIR)

alarm_clock: $(EXAMPLEDIR)/alarm_clock
$(EXAMPLEDIR)/alarm_clock: $(EXAMPLEDIR)/alarm_clock.o
	$(CC) $< -lpthread -o $@
	$(CP) $(EXAMPLEDIR)/alarm_clock $(TESTDIR)

alarm_clock_nvm: $(EXAMPLEDIR)/alarm_clock_nvm
$(EXAMPLEDIR)/alarm_clock_nvm: $(EXAMPLEDIR)/alarm_clock_nvm.o $(LIBATLAS)
	$(CPP) $< $(LD_FLAGS) -o $@
	$(CP) $(EXAMPLEDIR)/alarm_clock_nvm $(TESTDIR)
