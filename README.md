NVthreads: Practical Persistence for Multi-threaded Applications
-------------------------------------------------------------------------------------


### Authors ###

- [Terry Hsu](http://www.cs.purdue.edu/homes/hsu62) <<terryhsu@purdue.edu>>

- Helge Bruegner <<helge.bruegner@tum.de>>

- [Indrajit Roy](http://www.indrajitroy.com) <<indrajitroy@google.com>>

- Kimberly Keeton <<kimberly.keeton@hpe.com>>

- [Patrick Eugster](https://www.cs.purdue.edu/homes/peugster/) <<p@dsp.tu-darmstadt.de>>


### Description ###

NVthreads is a drop-in replacement for the popular pthreads library that adds persistence
to existing multi-threaded C/C++ applications. NVthreads infers consistent states via
synchronization points, uses the process memory to buffer uncommitted changes,
and logs writes to ensure a program’s data is recoverable even after a crash.
NVthreads’ page level mechanisms result in good performance: applications that use
NVthreads can be more than 2× faster than state-of-the-art systems that favor
fine-grained tracking of writes. After a failure, iterative applications that use NVthreads
gain speedups by resuming execution.


### Master Source  ###

https://github.com/HewlettPackard/nvthreads


### Maturity ###

NVthreads is still under development.  Please use NVthreads at your own risk. Do not deploy
this research prototype to your production software before verifying the correctness and
performance of your ported apps. Also, please use the master branch only, other branches
are unstable research prototypes.

### Dependencies ###

Install the following packages:
```
sudo apt-get install gcc-multilib
sudo apt-get install g++-multilib
sudo apt-get install libc6-dev-i386 (if you need 32-bit nvthreads)
```

### Build & test ###

1. Install dummy_nvmfs: https://github.com/HewlettPackard/dummy_nvmfs
   - NOTE: For simple testing of NVthreads, this step may be skipped, so long as the path `/mnt/ramdisk/nvthreads/` exists. Be aware that there will be no delays for each NVM write in this case.
     
2. Clone NVthreads repo:
```
git clone https://github.com/HewlettPackard/nvthreads
```

3. Create nvmfs with 1000ns delays:
   - This step is only necessary if using dummy_nvmfs from step 1.
```
cd $NVthreads/
./mknvmfs1000
```

4. Build NVthreads:
```
cd $NVthreads/src/
make libnvthread.so
```

5. Build the *recovery* test program:
```
cd $NVthreads/tests/recover/
make
```

6. Run the test:
   - Note: To start with a clean NVthreads environment, before beginning a test, delete the `/tmp/nvlib.crash` file, if it exists.
```
./recover_int.o  //will abort
./recover_int.o  //will recover data from previous run
```         


### Source tree structure ###
   
    apps/: The applications cases for NVthreads.
        - datagen/: generates data from kmeans inputs.
        - kmeans/: implementation of the kmeans algorithm.
            - phoenix-recovery: kmeans recovery evaluation
        - pagerank/: implementation of the well-known page rank algorithm.
        - tokyocabinet-1.4.48: Tokyo Cabinet evaluation.

    docs/: Published reserach paper for the NVthreads design rationale.

    dummy_nvmfs: https://github.com/HewlettPackard/dummy_nvmfs

    eval/: Benchmark evaluation
        datasets/: please save input data for benchmarks in this directory
        tests/: this directory contains Phoenix and PARSEC benchmarks

    src/: The core of NVthreads library
        source: source code 
        include: header file

    tests/: Simple test cases for NVthreads library
    
    third-parties/: 
        atlas/: https://github.com/HewlettPackard/Atlas
        dthreads/: https://github.com/emeryberger/dthreads
        mnemosyne/: http://research.cs.wisc.edu/sonar/projects/mnemosyne/


### Citing NVthreads ###

If you use NVthreads, please cite our reearch paper published at EuroSys 2017, included as doc/nvthreads-eurosys.pdf.

@InProceedings{nvthreads,   
author    = {Hsu, Terry Ching-Hsiang and Bruegner, Helge and Roy, Indrajit and Keeton, Kimberly and Eugster, Patrick},   
title     = {{NVthreads: Practical Persistence for Multi-threaded Applications}},   
booktitle = {Proceedings of the 12th ACM European Systems Conference},   
year      = {2017},   
series    = {EuroSys 2017},   
address   = {New York, NY, USA},   
publisher = {ACM},   
doi       = {10.1145/3064176.3064204},   
isbn      = {978-1-4503-4938-3},   
location  = {Belgrade, Republic of Serbia},   
url       = {http://dl.acm.org/citation.cfm?doid=3064176.3064204},   
}


### Acknowledgement ###
 - We thank the authors of Dthreads: Tongping Liu, Charlie Curtsinger, and Emery Berger, who open sourced their software that 
 makes NVthreads possible. Please refer to their original work at [Dthreads: Efficient Deterministic Multithreading](https://github.com/emeryberger/dthreads)
 and [Heap Layers: An Extensible Memory Allocation Infrastructure](https://github.com/emeryberger/Heap-Layers).
 - This work was supported in part by Hewlett Packard Labs, the National Science Foundation under grant TC-1117065,  
TWC-1421910, and the European Research Council under grant FP7-617805.
