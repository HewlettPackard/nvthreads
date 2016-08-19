----------Global settings----------------
dthreads/eval/Default.mk:
	Description: 
		Removed evaluation for dmp_o and dmp_b; fixed wrong shared library name
	Diff:
		+#CONFIGS = pthread dthread dmp_o dmp_b
		+CONFIGS = pthread dthread 
		-$(TEST_NAME)-dthread: $(DTHREAD_OBJS) $(DTHREADS_HOME)/src/libdthreads.so
		+$(TEST_NAME)-dthread: $(DTHREAD_OBJS) $(DTHREADS_HOME)/src/libdthread.so
dthreads/eval/tests/defines.mk
	Description:
		Fixed datasets path;
	Diff:
		-DATASET_HOME=/nfs/cm/scratch1/tonyliu/grace/branches/charliebenchmark/dthreads_eval_pldi10/datasets
		-<<<<<<< HEAD
		-=======
		-
		->>>>>>> ecea07c2d444b0454de49cadcabab0e17bb64ff6
		+DATASET_HOME=/home/terry/workspace/nvthreads/third-parties/dthreads/eval/datasets
		+#<<<<<<< HEAD
		+#=======
		+#>>>>>>> ecea07c2d444b0454de49cadcabab0e17bb64ff6

----------Phoenix benchmarks-------------
URL: https://github.com/kozyraki/phoenix

-histogram:
	-Input: http://csl.stanford.edu/~christos/data/histogram.tar.gz
	-Makefile:
-kmeans:
	-Input: N/A
	-Makefile: 
-linear_regression:
	-Input: http://csl.stanford.edu/~christos/data/linear_regression.tar.gz
	-Makefile:
-string_match:
	-Input: http://csl.stanford.edu/~christos/data/reverse_index.tar.gz
	-Makefile:
-reverse_index:
	-Input: http://csl.stanford.edu/~christos/data/string_match.tar.gz
	-Makefile:
-word_count:
	-Input: http://csl.stanford.edu/~christos/data/word_count.tar.gz
	-Makefile: 

---------Parsec benchmarks---------------
URL: http://parsec.cs.princeton.edu/
Inputs: http://parsec.cs.princeton.edu/download.htm

-blackscholes:
	-Makefile:
-canneal
	-Makefile:
-dedup
	-Makefile:
-ferret
	required for 64-bit machine: http://www.imagemagick.org/script/advanced-unix-installation.php
	dpkg --add-architecture i386
	sudo apt-get install libjpeg62:i386
	-Makefile:
-streamcluster
	-Makefile:
-swaptions
	-Makefile:

------------Altas----------------------
To benchmark Atlas:
1. compile Atlas
    cd $ATLAS
    mkdir build
    cd $ATLAS/build                       
    cmake -DALL_PERSISTENT=true -DNVM_STATS=true ../
    make -j 24
2. build compiler plug-in
    cd $ATLAS/compiler_plugin
    build_static_3.8
    cd $ATLAS/build/tests
3. build Parsec and Phoenix benchmarks
    ./test_atlas true
4. run Parsec and Phoenix benchmarks
    ./$NVthreads/eval/eval-atlas.py

Issues:
   parsec.canneal: float point exception
   parsec.dedup: llvm doesn't compile due to asm size mismatch error
