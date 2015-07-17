NVTHREAD_HOME = $(PARSECDIR)/../..
NVTHREAD_LIB = $(NVTHREAD_HOME)/src/libnvthread.so
DTHREAD_LIB = $(NVTHREAD_HOME)/third-parties/dthreads/src/libdthread.so
NVTHREAD_FLAGS = -rdynamic $(NVTHREAD_LIB) -ldl
DTHREAD_FLAGS = -rdynamic $(DTHREAD_LIB) -ldl
