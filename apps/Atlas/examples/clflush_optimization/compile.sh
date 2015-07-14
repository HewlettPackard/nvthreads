CC_FLAGS="-g -O2"

rm -rf *.o
rm -rf *.out
gcc -c $CC_FLAGS store_sync.c

g++ $CC_FLAGS -c large_data_init.cpp
g++ $CC_FLAGS -o a.out store_sync.o large_data_init.o

g++ $CC_FLAGS -c sll_flush.c
g++ $CC_FLAGS -o sll_flush.out store_sync.o sll_flush.o 

g++ $CC_FLAGS -c sll_flush_opt.c
g++ $CC_FLAGS -o sll_flush_opt.out store_sync.o sll_flush_opt.o 

g++ $CC_FLAGS -c queue_flush.c
g++ $CC_FLAGS -lpthread -o queue_flush.out store_sync.o queue_flush.o 


gcc -c $CC_FLAGS -DTHREADS store_sync.c
g++ $CC_FLAGS -DTHREADS -c queue_flush_opt.c
g++ $CC_FLAGS -lpthread -o queue_flush_opt.out store_sync.o queue_flush_opt.o 

