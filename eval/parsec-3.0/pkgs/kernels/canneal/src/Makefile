# Makefile for parallel simulated annealer
include $(PARSECDIR)/defines.mk
PREFIX=${PARSECDIR}/pkgs/kernels/canneal/inst/${PARSECPLAT}

TARGET=canneal
LIBS:=$(LIBS) -lm

ifdef version
  ifeq "$(version)" "pthreads"
    CXXFLAGS+=-DENABLE_THREADS -pthread
  endif
endif

CXXFLAGS_XTHREAD = $(filter-out -pthread, $(CXXFLAGS))
all:
	$(CXX) $(CXXFLAGS) annealer_thread.cpp -c -o annealer_thread.o
	$(CXX) $(CXXFLAGS) rng.cpp -c -o rng.o
	$(CXX) $(CXXFLAGS) netlist.cpp -c -o netlist.o
	$(CXX) $(CXXFLAGS) main.cpp -c -o main.o
	$(CXX) $(CXXFLAGS) netlist_elem.cpp -c -o netlist_elem.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) *.o $(LIBS) -o $(TARGET)
	$(CXX) $(CXXFLAGS_XTHREAD) $(LDFLAGS) *.o $(LIBS) -o $(TARGET)-dthread $(DTHREAD_FLAGS)
	$(CXX) $(CXXFLAGS_XTHREAD) $(LDFLAGS) *.o $(LIBS) -o $(TARGET)-nvthread $(NVTHREAD_FLAGS)

clean:
	rm -f *.o $(TARGET) $(TARGET)-dthread $(TARGET)-nvthread

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)

