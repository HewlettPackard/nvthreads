include ../defines.mk

DATASET_HOME = /home/terry/workspace/nvthreads/src/eval/datasets/parsec-3.0/pkgs/apps/ferret/inputs
TEST_NAME = ferret
TEST_FILES = ferret arena assign bench bitmap cass cass_dist cass_reg cuckoo_hash dataset dist emd env error hash hash_table heap import map matrix raw stat table timer topk tp tpool util edge extract image srm local LSH LSH_query_batch LSH_query perturb recall
TEST_ARGS = $(DATASET_HOME)/corel lsh $(DATASET_HOME)/queries 10 20 1 output.txt

CFLAGS = -Iinclude
LIBS = -Llib -lrt -lm -lgsl -lgslcblas -ljpeg  /usr/local/lib/libMagickCore-6.Q16.so
#LIBS = -Llib -lrt -lm -lgsl -lgslcblas -ljpeg

DMP_O_GRAN = 6
DMP_O_QUANTUM = 100000

DMP_B_GRAN = 6
DMP_B_QUANTUM = 100000

include $(HOME)/Default.mk

