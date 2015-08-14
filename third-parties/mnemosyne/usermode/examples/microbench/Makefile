ICC=/home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icc
NVLIB = /mnt/ssd/terry/workspace/nvthreads/src/libnvthread.so
all: pt nvt icc mnemosyne
	mv *.out $(PWD)/../../build/examples/microbench/
pt:
	gcc pmalloc-mt.c -o pmalloc-mt-pt.out -lpthread
nvt:
	gcc pmalloc-mt.c -o pmalloc-mt-nvt.out -rdynamic $(NVLIB) -ldl
icc:
	$(ICC) pmalloc-mt.c -DINTEL_ICC -Qtm_enabled -o pmalloc-mt-icc.out
mnemosyne:
	cd ../../ && scons --build-example=microbench --build-debug
clean:
	rm *.out
