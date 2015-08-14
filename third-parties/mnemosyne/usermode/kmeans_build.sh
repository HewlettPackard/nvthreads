scons --build-debug --build-stats
rm build/examples/kmeans-icc/*
rm /tmp/segments/*

scons --build-example=kmeans-icc --build-stats --build-debug VERBOSE=1

#-------------icc--------------
rm build/examples/kmeans-icc-intel/*
cp examples/kmeans-icc-intel/* build/examples/kmeans-icc-intel/

/home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icpc -o build/examples/kmeans-icc-intel/kmeans.o -c -Qtm_enabled -fpic -g -m64 -lrt -g build/examples/kmeans-icc-intel/kmeans.cpp

/home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icpc -o build/examples/kmeans-icc-intel/kmeans-icc-intel -Qtm_enabled build/examples/kmeans-icc-intel/kmeans.o

#------------phoenix icc---------
rm build/examples/kmeans-phoenix/*
cp examples/kmeans-phoenix/* build/examples/kmeans-phoenix/

/home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icc -o build/examples/kmeans-phoenix/kmeans-phoenix.o -c -Qtm_enabled -fpic -g -m64 -lrt -g build/examples/kmeans-phoenix/kmeans-pthread.c

/home/hsuchi/intel/Compiler/11.0/606/bin/intel64/icpc -o build/examples/kmeans-phoenix/kmeans-phoenix  -Qtm_enabled build/examples/kmeans-phoenix/kmeans-phoenix.o
