ICC=/home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icc
NVLIB = /mnt/ssd/terry/workspace/nvthreads/src/libnvthread.so
DLIB = /mnt/ssd/terry/workspace/nvthreads/third-parties/dthreads/libdthread.so
SRC = page_density.c
OBJ = page_density
all: icc mnemosyne 
	mv *.out $(PWD)/../../build/examples/microbench/
pt:
	gcc -g $(SRC) -o $(OBJ)-pt.out -lpthread
nvt:
	gcc -g $(SRC) -o $(OBJ)-nvt.out -rdynamic $(NVLIB) -ldl
dt:
	gcc -g $(SRC) -o $(OBJ)-dt.out -rdynamic $(DLIB) -ldl
icc:
	$(ICC) $(SRC) -DINTEL_ICC -Qtm_enabled -o $(OBJ)-icc.out
mnemosyne:
	cd ../../ && scons --build-example=microbench --build-debug
clean:
	rm -rf ../../build/examples/microbench/*
	rm -rf /tmp/*
