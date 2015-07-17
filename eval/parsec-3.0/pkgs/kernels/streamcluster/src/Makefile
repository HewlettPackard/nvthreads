# Makefile for streamcluster
include $(PARSECDIR)/defines.mk
PREFIX=${PARSECDIR}/pkgs/kernels/streamcluster/inst/${PARSECPLAT}

TARGET=streamcluster
OBJS=streamcluster.o

ifdef version
  ifeq "$(version)" "pthreads"
    CXXFLAGS :=	$(CXXFLAGS) -DENABLE_THREADS -pthread
    OBJS += parsec_barrier.o
  endif
  ifeq "$(version)" "tbb"
    CXXFLAGS := $(CXXFLAGS) -DTBB_VERSION
    LIBS := $(LIBS) -ltbb
  endif
endif

CXXFLAGS_XTHREAD = $(filter-out -pthread, $($CXXFLAGS))
all: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)
	$(CXX) $(CXXFLAGS_XTHREAD) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)-dthread $(NVTHREAD_FLAGS)
	$(CXX) $(CXXFLAGS_XTHREAD) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)-nvthread  $(DTHREAD_FLAGS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o $(TARGET) $(TARGET)-dthread $(TARGET)-nvthread

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)

