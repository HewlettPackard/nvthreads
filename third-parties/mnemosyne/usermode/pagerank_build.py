#!/usr/bin/env python

import os
icc='/home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icc'

#-----------mnemosyne----------------------------
BUILD_PATH_MNEMOSYNE='build/examples/pagerank-mnemosyne'
SRC_PATH_MNEMOSYNE='examples/pagerank-mnemosyne'
#scons --clean
cmd = 'scons --build-debug '
print cmd
rv = os.system(cmd)
if rv != 0:
	print 'error, abort'
	exit(-1)

cmd = 'rm '+BUILD_PATH_MNEMOSYNE + '/*'
print cmd
rv = os.system(cmd)

cmd = 'rm /tmp/segments/*'
print cmd
rv = os.system(cmd)

cmd = 'cp '+SRC_PATH_MNEMOSYNE+'/* '+BUILD_PATH_MNEMOSYNE+'/'
print cmd
rv = os.system(cmd)

targets = ['algorithm', 'crsmatrix', 'logger', 'vector', 'main']
args = '-c -Qtm_enabled -fpic -g -m64 -lrt -g -DM_PCM_CPUFREQ=2500 -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=1200 -Ilibrary/mcore/include -Ilibrary/mtm/include -Ilibrary/pmalloc/include -Ilibrary/common'
for t in targets:
	obj = BUILD_PATH_MNEMOSYNE+'/'+t+'.o'
	src = SRC_PATH_MNEMOSYNE+'/'+t+'.c'
	cmd = icc+ ' -o' + obj + ' ' + args + ' ' + src
	print cmd
	rv = os.system(cmd)
	if rv != 0 :
		print 'error, abort'
		exit(-1)

cmd = icc + ' -o '+BUILD_PATH_MNEMOSYNE+'/pagerank-mnemosyne -Qtm_enabled -T /mnt/ssd/terry/workspace/nvthreads/third-parties/mnemosyne/usermode/tool/linker/linker_script_persistent_segment_m64 '+BUILD_PATH_MNEMOSYNE+'/main.o '+ BUILD_PATH_MNEMOSYNE+'/algorithm.o '+BUILD_PATH_MNEMOSYNE+'/logger.o '+BUILD_PATH_MNEMOSYNE+'/crsmatrix.o '+BUILD_PATH_MNEMOSYNE+'/vector.o ' +'build/library/pmalloc/libpmalloc.so build/library/mcore/libmcore.so build/library/mtm/libmtm.so -lrt ' 
print cmd
rv = os.system(cmd)
if rv != 0 :
	print 'error, abort'
	exit(-1)

cmd = 'rm -rf /tmp/*'
os.system(cmd)

#$ICC -o $BUILD_PATH_MNEMOSYNE/algorithm.o -c -Qtm_enabled -fpic -g -m64 -lrt -g -DM_PCM_CPUFREQ=2500 -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=1200 -Ilibrary/mcore/include -Ilibrary/mtm/include -Ilibrary/pmalloc/include -Ilibrary/common $BUILD_PATH_MNEMOSYNE/algorithm.c
#$ICC -o $BUILD_PATH_MNEMOSYNE/crsmatrix.o -c -Qtm_enabled -fpic -g -m64 -lrt -g -DM_PCM_CPUFREQ=2500 -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=1200 -Ilibrary/mcore/include -Ilibrary/mtm/include -Ilibrary/pmalloc/include -Ilibrary/common $BUILD_PATH_MNEMOSYNE/crsmatrix.c
#$ICC -o $BUILD_PATH_MNEMOSYNE/logger.o -c -Qtm_enabled -fpic -g -m64 -lrt -g -DM_PCM_CPUFREQ=2500 -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=1200 -Ilibrary/mcore/include -Ilibrary/mtm/include -Ilibrary/pmalloc/include -Ilibrary/common $BUILD_PATH_MNEMOSYNE/logger.c
#$ICC -o $BUILD_PATH_MNEMOSYNE/main.o -c -Qtm_enabled -fpic -g -m64 -lrt -g -DM_PCM_CPUFREQ=2500 -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=1200 -Ilibrary/mcore/include -Ilibrary/mtm/include -Ilibrary/pmalloc/include -Ilibrary/common $BUILD_PATH_MNEMOSYNE/main.c
#$ICC -o $BUILD_PATH_MNEMOSYNE/vector.o -c -Qtm_enabled -fpic -g -m64 -lrt -g -DM_PCM_CPUFREQ=2500 -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=1200 -Ilibrary/mcore/include -Ilibrary/mtm/include -Ilibrary/pmalloc/include -Ilibrary/common $BUILD_PATH_MNEMOSYNE/vector.c
#$ICC -o $BUILD_PATH_MNEMOSYNE/pagerank-icc -Qtm_enabled -T /mnt/ssd/terry/workspace/mnemosyne/usermode/tool/linker/linker_script_persistent_segment_m64 $BUILD_PATH_MNEMOSYNE/main.o $BUILD_PATH_MNEMOSYNE/algorithm.o $BUILD_PATH_MNEMOSYNE/logger.o $BUILD_PATH_MNEMOSYNE/crsmatrix.o $BUILD_PATH_MNEMOSYNE/vector.o build/library/pmalloc/libpmalloc.so build/library/mcore/libmcore.so build/library/mtm/libmtm.so -lrt
exit(0)

#----------icc----------------------
#BUILD_PATH_INTELSTM=build/examples/pagerank-icc-intel/
#SRC_PATH_INTELSTM=examples/pagerank-icc-intel/
#rm $BUILD_PATH_INTELSTM/*
#cp $SRC_PATH_INTELSTM/* $BUILD_PATH_INTELSTM/
#
#$ICC -o $BUILD_PATH_INTELSTM/algorithm.o -c -Qtm_enabled -fpic -g -m64 -lrt -g $BUILD_PATH_MNEMOSYNE-intel/algorithm.c
#$ICC -o $BUILD_PATH_INTELSTM/logger.o -c -Qtm_enabled -fpic -g -m64 -lrt -g $BUILD_PATH_MNEMOSYNE-intel/logger.c
#$ICC -o $BUILD_PATH_INTELSTM/crsmatrix.o -c -Qtm_enabled -fpic -g -m64 -lrt -g $BUILD_PATH_MNEMOSYNE-intel/crsmatrix.c
#$ICC -o $BUILD_PATH_INTELSTM/vector.o -c -Qtm_enabled -fpic -g -m64 -lrt -g $BUILD_PATH_MNEMOSYNE-intel/vector.c
#$ICC -o $BUILD_PATH_INTELSTM/main.o -c -Qtm_enabled -fpic -g -m64 -lrt -g $BUILD_PATH_MNEMOSYNE-intel/main.c
#$ICC -o $BUILD_PATH_INTELSTM/pagerank-icc-intel -Qtm_enabled $BUILD_PATH_MNEMOSYNE-intel/main.o $BUILD_PATH_MNEMOSYNE-intel/algorithm.o $BUILD_PATH_MNEMOSYNE-intel/logger.o $BUILD_PATH_MNEMOSYNE-intel/crsmatrix.o $BUILD_PATH_MNEMOSYNE-intel/vector.o -lrt
#

