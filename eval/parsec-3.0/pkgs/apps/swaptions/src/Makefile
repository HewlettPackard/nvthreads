include $(PARSECDIR)/defines.mk
PREFIX := ${PARSECDIR}/pkgs/apps/swaptions/inst/${PARSECPLAT}
DEF =
INCLUDE =

EXEC = swaptions 
EXEC_dthread = swaptions-dthread
EXEC_nvthread = swaptions-nvthread

ifdef version
  ifeq "$(version)" "pthreads" 
    DEF := $(DEF) -DENABLE_THREADS
    CXXFLAGS := $(CXXFLAGS) -pthread
  endif
  ifeq "$(version)" "tbb"
    DEF := $(DEF) -DENABLE_THREADS -DTBB_VERSION
    LIBS := $(LIBS) -ltbb
  endif
endif

OBJS= CumNormalInv.o MaxFunction.o RanUnif.o nr_routines.o icdf.o \
	HJM_SimPath_Forward_Blocking.o HJM.o HJM_Swaption_Blocking.o  \
	HJM_Securities.o

CXXFLAGS_XTHREAD = $(filter-out -pthread, $(CXXFLAGS))
all: $(EXEC) $(EXEC_dthread) $(EXEC_nvthread)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEF) $(OBJS) $(INCLUDE) $(LIBS) -o $(EXEC)
$(EXEC_dthread): 
	$(CXX) $(CXXFLAGS_XTHREAD) $(LDFLAGS) $(DEF) $(OBJS) $(INCLUDE) $(LIBS) -o $(EXEC_dthread) $(DTHREAD_FLAGS)
$(EXEC_nvthread):
	$(CXX) $(CXXFLAGS_XTHREAD) $(LDFLAGS) $(DEF) $(OBJS) $(INCLUDE) $(LIBS) -o $(EXEC_nvthread) $(NVTHREAD_FLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(DEF) -c $*.cpp -o $*.o

.c.o:
	$(CXX) $(CXXFLAGS) $(DEF) -c $*.c -o $*.o

clean:
	rm -f $(OBJS) $(EXEC) $(EXEC_dthread) $(EXEC_nvthread)

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(EXEC) $(PREFIX)/bin/$(EXEC)

