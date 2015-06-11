
/** VERSION:
 *
 @Last Version: Graph Inference engine that implements an In-memory graph, numa-aware, optimized to local reads and batched remote writes.
 *This version implements Gibbs Sampling method.
 *
 *@ Fei, Nandish, Tere, Krishna,Hideaki
 *@ Last update: May 29, 2015
 *
 *Before compiling, install
 *Library Dependencies: boost, numalib,pthread
 *
 * To compile: g++ -pthread gibbsDNS-ArrayPartitioned.cpp -lpthread -lnuma  -lboost_program_options
 * export LD_LIBRARY_PATH=/usr/local/lib to locate boots libraries
 * or use the Makefile provided
 *
 * ./make clean
 * ./make
 */

#include <vector>
#include <iostream>
#include <sys/time.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <assert.h>
#include <math.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <numa.h>
#include <unistd.h> //to avoid errorin dragonhawk
#include "graphReader.h"

using namespace std;

/**********************************************
 * DEBUG FLAGS
 * it should be commented out to enable the debug code.
 **********************************************/
#define DEBUG_TIME_ON /*Enables breakdown time*/
//#define DEBUG /*Enable general stats*/
//#define DEBUG_GRAPHSTATS 0
//#define DEBUG_TOPOLOGY


/**********************************************
 * Socket and threads configuration
 **********************************************/
/*default value for number of sockets in which topology graph will be replicated.*/
#define NUMBER_OF_SOCKETS 4
// number of maximum CPU cores per socket
#define MAX_CORE_PER_SOCKET 10
// default value for number of total samples
#define NUMBER_OF_ITERS 200
// default value for number of threads to use for actual Gibbs sampling.
#define NUMBER_OF_THREADS 32
// default number of threads per socket (Based on the above values; NUMBER_OF_THREADS/NUMBER_OF_SOCKETS)
#define NUMBER_OF_THREADS_PER_SOCKET 8
// the way in which threads are supposed to access the topology array:
// 0=Round-Robin, 1=Partition, 2=Uniform_Thread_Workload (Fei's benchmark version)
#define TOPOLOGY_ARRAY_ACCESS_METHOD 1


/***********************************************
 * CUSTOM TYPES
 * for graph/graph topology and main operations
 ***********************************************/
/*Factor types*/
#define  FACTOR_VALUE_TYPE_INFLATED int64_t
#define  FACTOR_VALUE_TYPE_DEFLATED float
#define  FACTOR_VALUE_INFLATION (1LL << 20)
#define MAX_FACTOR_LENGTH 2

/*Vertex type:  type for the state of the vertex*/
#define VERTEX_VALUE_TYPE char

/*aggregators/couters: type for convergence and counters*/
#define VERTEX_AGG_VALUE_TYPE int
#define FLOAT_AGG_VALUE_TYPE float

/*Topology/Iterators: types for index, offsets, Vertices identifiers*/
#define TOPOLOGY_VALUE_TYPE int64_t
#define ITERATOR_INDEX int64_t

/***********************************************
 * FACTOR CONSTANTS
 * for accessing factor table
 ***********************************************/
/*number of elements for a unary factor*/
#define UNARY_FACTOR_LENGTH 4
/*number of elements for a binary factor*/
#define BINARY_FACTOR_LENGTH 6
/*number of unary factor within a vertex*/
#define NUM_OF_UNARY_FACTORS 1
/*number of binary factor within a vertex*/
#define NUM_OF_BINARY_FACTORS 2
/*number of variables within a domain*/
#define NUM_OF_VALUES_FOR_VARIABLE 2

/****************************************************************
 * GRAPH TOPOLOGY STRUCTURES
 * This is the topology graph that is stored in each socket used. This is the main data structure that will be
 * accessed by all the threads for Gibbs sampling. This is a read-only immutable data structure.
 */

/*topology graph: vertices, edge factors, neighbors*/
struct topology_graph {
	// An entry for each vertex in the graph. Contains links to corresponding index in the topology array.
	struct vertex_info *vertex_array;
	// A big 1-D array that stores the information regarding each factor of each node, along with links to the neighboring
	// vertices of a factor.
	union topology_array_element *topology_array;
	// This is the actual factor table.
	FACTOR_VALUE_TYPE_INFLATED *factor_tables;
};

/*vertex pointers*/
struct vertex_info {
	/* this is the pointer to the first factor pointer of a vertex.*/
	TOPOLOGY_VALUE_TYPE first_factor_offset;
	/* this is the pointer to the global variable location which holds the actual value for this vertex.*/
	VERTEX_VALUE_TYPE *global_value_ptr;
	/* number of index locations that belong to this vertex in the topology_array*/
	int length;
	/* initial label for the vertex*/
	bool label;
};

/*topology array*/
union topology_array_element {
	/*pointer to the value of a neighboring vertex.*/
	VERTEX_VALUE_TYPE* vertex_ptr;
	/* offset index to the correct entry in the factor table for the factor considered.*/
	FACTOR_VALUE_TYPE_INFLATED factor_value;
	/* pointer to the correct entry in the factor table for the factor considered.*/
	FACTOR_VALUE_TYPE_INFLATED *factor_ptr;
};


/***********************************************
 * GLOBAL VARIALBES
 * for parameters that can be changed based on input
 ***********************************************/

/*Convergence parameters*/
/*on/off to generate convergence statistics*/
bool generate_statistics  = true;
/*number of samples to be done*/
int num_of_stat_samples   = 1;
/*number of iterations to execute the sampling*/
int stat_samples_interval = 100;
/*threshold >= if the vetex has converged*/
double threshold          = 0.03;

/*Global values for vertex/factors statistics*/
int numVars				   = 2;
int numEdgeFactors		   = 4;
int topologySizeByVertex   = 5;
ITERATOR_INDEX num_of_nodes;
size_t topologyArraySize;

/*default file names*/
const std::string PARTITIONSIZE=".par";
const std::string LABELS=".labels";

/*input file name*/
string fileName;

/*global timer*/
struct timeval timestamp;

/* default, shuffle every 100 updates*/
uint64_t kRefreshIntervalPercent = 100;

/*Distributed Global arrays for vertex states:*/
/*vertex state*/
VERTEX_VALUE_TYPE 		**global_array_partitioned = NULL;
/*vertex counters*/
VERTEX_AGG_VALUE_TYPE   **global_array_copy_partitioned = NULL;
FLOAT_AGG_VALUE_TYPE	**global_array_conv_partitioned = NULL;
/*vertex convergence*/
VERTEX_AGG_VALUE_TYPE 	**global_array_nonConv_partitioned = NULL;


/*Shared Global arrays for vertex states*/
VERTEX_VALUE_TYPE *global_array = NULL;
VERTEX_VALUE_TYPE *global_array_copy = NULL;

/*list of partition indices by socket*/
ITERATOR_INDEX *sizeGlobalArrayDistribution = NULL;
ITERATOR_INDEX *startThreadIndex = NULL;
ITERATOR_INDEX *endThreadIndex = NULL;
VERTEX_VALUE_TYPE *globalArrayIndex = NULL;

/*Topology Master pointer
 * Array of topology copiesTOPOLOGY_ARRAY_ACCESS_METHOD
 * replicated on each input node*/
struct topology_graph **tgMaster = NULL;

/*Global variables for managing threads*/
/*selected number of sockets to use*/
int num_sockets_to_use;
/*selected numa nodes to use, eg  {1,2}*/
int *numaNodes=NULL;
/* keeps track of the CPU ID to be used to stick the next thread on a socket to.*/
int *cpuid_counter_array = NULL;
/* to be used for partition method. Need to know the max number of threads to use per socket, according to the input params*/
int *threads_per_socket=NULL;
/*number of threads to use from the user*/
int num_of_threads;
/*number of threads that will be used per socket. This depends on the total number of threads and number of sockets used*/
int num_threads_per_socket;
/*total iterations to run*/
int num_total_iters;
/*Iteration counter*/
int ITER = 1;
/*phtread barries for the iterations*/
/*main iteration*/
pthread_barrier_t barrier1;
/*convergence test barrier*/
pthread_barrier_t barrier2;
/*mutex to update cpu counters*/
pthread_mutex_t cpumutex;


// The flag that will be used to determine what way to access the topology array in (round robin or partition).
// This is similar to TOPOLOGY_ARRAY_ACCESS_METHOD, which gives the default value.
// // 0=Round-Robin, 1=Partition, 2=Uniform_Thread_Workload (Fei's benchmark version)
/*TB refactored*/
int topology_array_access;


/*****************************************************************
 * Main methods
 *
 *****************************************************************/

/*headers*/
inline void computeConvergenceAtIteration(int numaNodeId,int &local_samples_seen,
		int local_samples_collected, int &samplesCounter,
		ITERATOR_INDEX start_index,ITERATOR_INDEX endIndex);
ITERATOR_INDEX getTopologySizeAtNodeId(int numaNodeId, ITERATOR_INDEX &startIndex,
		ITERATOR_INDEX &endInddex);



/*custom split method
 * */
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}
/*
 * custom split method
 * */
std::vector<std::string> split_string(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}
/*
 * custom split method
 * */
inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

/*****************************************************************
 * Factor Methods
 * Enable manage edge factors as factor table
 *****************************************************************/

/*
 * factor deflated
 *convert int64 to float
 */

inline FACTOR_VALUE_TYPE_DEFLATED deflate_factor_value(FACTOR_VALUE_TYPE_INFLATED inflated) {
  return static_cast<FACTOR_VALUE_TYPE_DEFLATED>(inflated) / FACTOR_VALUE_INFLATION;
}
/*
 * factor in inflated
 * convert float to int64
 */
inline FACTOR_VALUE_TYPE_INFLATED inflate_factor_value(FACTOR_VALUE_TYPE_DEFLATED deflated) {
   //cout<<"inflating value "<<deflated<<"\n";
   //cout<<"inflated value "<<static_cast<FACTOR_VALUE_TYPE_INFLATED>(deflated * FACTOR_VALUE_INFLATION)<<"\n";
   return static_cast<FACTOR_VALUE_TYPE_INFLATED>(deflated * FACTOR_VALUE_INFLATION);
}

/**
 * There can be multiple ways to store factors. Either read it from a file or hard code it.
 * We can also have different kinds of factors. How we populate the factor table and how we retrieve the
 * index to a particular factor are co-dependent. So if we want different ways to do the same, change both
 * functions accordingly.
 */
inline void populate_factors(FACTOR_VALUE_TYPE_INFLATED *factor_table) {
	//int factor_table_length = (NUM_OF_UNARY_FACTORS*UNARY_FACTOR_LENGTH) + (NUM_OF_BINARY_FACTORS*BINARY_FACTOR_LENGTH);
	//FACTOR_VALUE_TYPE *factor_table = (FACTOR_VALUE_TYPE*)malloc(factor_table_length*sizeof(FACTOR_VALUE_TYPE));
	// For now, tentatively, I am hard-coding the following factor table.
	// For each factor, the following entries will be shown:
	// # of variables, # of entries in the factor, # the actual entries.
	// First start with unary factor;
	int i=0;
	factor_table[i++] = 1.0;
	factor_table[i++] = 2.0;
	factor_table[i++] = inflate_factor_value(-0.69314);
	factor_table[i++] = inflate_factor_value(-0.69314);
	// now go for the first binary factor (FIRST vertex id < SECOND vertex id).
	factor_table[i++] = 2.0;
	factor_table[i++] = 4.0;
	factor_table[i++] = inflate_factor_value(-0.67334);// (0, 0)
	factor_table[i++] = inflate_factor_value(-0.71334);// (0, 1)
	factor_table[i++] = inflate_factor_value(-1.38629);// (1, 0)
	factor_table[i++] = inflate_factor_value(-0.28768);// (1, 1)
	// now go for the second binary factor (SECOND vertex id < FIRST vertex id).
	factor_table[i++] = 2.0;
	factor_table[i++] = 4.0;
	factor_table[i++] = inflate_factor_value(-0.67334);// (0, 0)
	factor_table[i++] = inflate_factor_value(-1.38629);// (0, 1)
	factor_table[i++] = inflate_factor_value(-0.71334);// (1, 0)
	factor_table[i++] = inflate_factor_value(-0.28768);// (1, 1)
	//return factor_table;

//	for(int j=0; j<i;j++)
	       //cout<<"factor at "<<j<<" is "<<factor_table[j]<<"\n";
}

/**
 * This method is for the rudimentary factor tables above. This has to be changed when we introduce more factor
 * tables.
 */
void get_factor_pointer(int vertex_id, int neighbor, int num_of_vars_in_factor,
		FACTOR_VALUE_TYPE_INFLATED *factor_table, FACTOR_VALUE_TYPE_INFLATED* return_ptr) {
	FACTOR_VALUE_TYPE_INFLATED* fact_ptr;
	if(num_of_vars_in_factor == 0) {
		// this is a unary factor. At the moment there is only one unary factor.
		fact_ptr = factor_table;
	}
	else {
		// if vertex 1 < vertex 2
		if(vertex_id < neighbor)
			fact_ptr = &factor_table[4];
		else
			fact_ptr = &factor_table[10];
	}
	return_ptr = fact_ptr;
}

/**
 * This method is currently tuned for binary factors and unary factors ONLY.
 * It has to change for more complex factors.
 */
FACTOR_VALUE_TYPE_INFLATED* get_factor_pointer(int vertex_id, int neighbor, int num_of_vars_in_factor,
		FACTOR_VALUE_TYPE_INFLATED *factor_table) {
	FACTOR_VALUE_TYPE_INFLATED* fact_ptr;
	if(num_of_vars_in_factor == 0) {
		// this is a unary factor. At the moment there is only one unary factor.
		fact_ptr = factor_table;
	}
	else {
		// if vertex 1 < vertex 2
		if(vertex_id < neighbor)
			fact_ptr = &factor_table[4];
		else
			fact_ptr = &factor_table[10];
	}
	return fact_ptr;
}


/**
 * Return the address of the location which is to be written.
 * The location that holds the latest value of "vertex_id"
 */
VERTEX_VALUE_TYPE* get_vertex_global_pointer(int vertex_id) {
	// This is a tentative method for now.
	return global_array+vertex_id;
}

/*
 * return the topology partition based on the vertex id
 * the partition id represents the numa socket id where is placed
 * the vertex info
 * */
int get_partition(TOPOLOGY_VALUE_TYPE vertex_id){

 ITERATOR_INDEX firstInSocket =0;
        ITERATOR_INDEX lastInSocket =-1;
        int i;
       for(i=0;i<num_sockets_to_use; i++){
                firstInSocket = (ITERATOR_INDEX) lastInSocket+1;
                lastInSocket  = (ITERATOR_INDEX)(firstInSocket + sizeGlobalArrayDistribution[i]-1);
            if(vertex_id <= lastInSocket && vertex_id >= firstInSocket)
                   break;
        }

 return i;
}


/*
 * retrieve vertex pointers based on the vertex id and numa node id
 * */
VERTEX_VALUE_TYPE* get_vertex_global_pointerByVariedSizePartition(TOPOLOGY_VALUE_TYPE vertex_id, int numaNode) {

      if(numaNode >=0){
    	return  &(global_array_partitioned[numaNode][vertex_id]);
    }

	ITERATOR_INDEX firstInSocket =0;
	ITERATOR_INDEX lastInSocket =-1;
        int i;  	
       for(i=0;i<num_sockets_to_use; i++){
		firstInSocket = (ITERATOR_INDEX) lastInSocket+1;
		lastInSocket = (ITERATOR_INDEX)(firstInSocket + sizeGlobalArrayDistribution[i]-1);
	    if(vertex_id <= lastInSocket && vertex_id >= firstInSocket)
	           break;
	}

	if(i>=num_sockets_to_use){

              std::cout<<"vertex id "<<vertex_id<<"\n";
              std::cout<<"firstInSocket "<<firstInSocket<<" last in socket "<<lastInSocket<<"\n";
              exit(2);
         }

	int socket = i;
	int offset = vertex_id - firstInSocket;

    return global_array_partitioned[socket] + offset;

}

/**
 * Return the pointer where vertex_id is mapped over the global array partitioned.
 * The location that holds the latest value of "vertex_id"
 * This method encodes logic to use global array partitions uniformed by the number of threads
 * assumes that the last socket is overloaded with
 * remaining work.
 * @PARAM: vertex_id
 * @RETURN: pointer to the vertex id
 * @author: @tere
 */
VERTEX_VALUE_TYPE* get_vertex_global_pointerByPartition2(int vertex_id) {

	int uniformPartitionSize =sizeGlobalArrayDistribution[0]; //assuming the first size distribution is the same than the other, except last one.
	int socket = vertex_id/uniformPartitionSize;
	int offset = vertex_id%uniformPartitionSize;

	//If socketid is the last socket,
	//we need to recalculate the offset because the array size may be different.
	if (socket >=num_sockets_to_use ){
		socket--;
		offset = (vertex_id - (uniformPartitionSize*(num_sockets_to_use-1)));
	}
//	std::cout <<"\n vertexi/d "<< vertex_id <<"  \t - "<<" socket: " << socket <<"  \t - offset: "<< offset;
	return global_array_partitioned[socket] + offset;
}

/**
 * Return the pointer where vertex_id is mapped over the global array partitioned.
 * The location that holds the latest value of "vertex_id"
 * This method encodes logic to use global array partitions uniformed by the number of threads
 * assumes that the last socket is overloaded with
 * remaining work.
 * @PARAM: vertex_id
 * @RETURN: pointer to the vertex id
 * @author: @tere
 */
VERTEX_VALUE_TYPE* get_vertex_global_pointerByPartition(int vertex_id_real) {


	int vertex_id            = globalArrayIndex[vertex_id_real];
	int uniformPartitionSize = sizeGlobalArrayDistribution[0]; //assuming the first size distribution is the same than the other, except last one.
	int socket = vertex_id/uniformPartitionSize;
	int offset = vertex_id%uniformPartitionSize;

	//If socket id is the last socket,
	//we need to recalculate the offset because the array size may be different.
	if (socket >=num_sockets_to_use ){
		socket--;
		offset = (vertex_id - (uniformPartitionSize*(num_sockets_to_use-1)));
	}
//	std::cout <<"\n vertexi/d "<< vertex_id <<"  \t - "<<" socket: " << socket <<"  \t - offset: "<< offset;
	return global_array_partitioned[socket] + offset;
}

/**
 * Display topology array
 * to verify the values are correct.
 * Debugging code only works for the small graph
* @tere
 */
void display_topology_graph_new(int node, size_t topologySize) {

	if (node>=0 && node<= num_sockets_to_use) {
		std::cout <<"\ntopology-array socket:" << node;
		std::cout <<"\ntopology-size: "<< topologySize <<"\n CONTENT \n";
		    for (size_t k =0, i=0; k<topologySize; k++,i++){

		    	std::cout << "\nuniary factor:"<<k;
		    	std::cout << "\nvalF:"<<tgMaster[node]->topology_array[k++].factor_value;
		    	std::cout << "\nvalF:"<<tgMaster[node]->topology_array[k++].factor_value;
		    	std::cout << "\nvertex factors size:"<<tgMaster[node]->vertex_array[i].length;

				for(int j=numVars; j<tgMaster[node]->vertex_array[i].length; j++) {
						if ((j-1)%topologySizeByVertex ==0){
							std::cout << "\nvalV: "<<*(tgMaster[node]->topology_array[k].vertex_ptr)+48;
						}else{
							std::cout << "\nvalF:"<<tgMaster[node]->topology_array[k].factor_value;
						}
						k++;
					}
				k--;
		    }
	}
	else{
		std::cout <<"\n Invalid topology node:"<<node;
		/*error invalid node id*/
	}
}


/**
 * Display topology array
 * to verify the values are correct.
 * Debugging code only works for the small graph
* @tere
 */
void display_topology_graph(int node, size_t topologySize) {

	if (node>=0 && node<= num_sockets_to_use) {
		std::cout <<"\ntopology-array socket:" << node;
		std::cout <<"\ntopology-size: \n CONTENT " << topologySize <<"\n";

		std::cout <<*(tgMaster[node]->topology_array[0].factor_ptr) <<" ";
		std::cout <<*(tgMaster[node]->topology_array[1].factor_ptr) <<" ";
		std::cout <<(*(tgMaster[node]->topology_array[2].vertex_ptr)+0) <<"v ";
		std::cout <<*(tgMaster[node]->topology_array[3].factor_ptr) <<" ";
		std::cout <<(*(tgMaster[node]->topology_array[4].vertex_ptr)+0) <<"v ";

		std::cout <<*(tgMaster[node]->topology_array[5].factor_ptr) <<" ";
		std::cout <<*(tgMaster[node]->topology_array[6].factor_ptr) <<" ";
		std::cout <<(*(tgMaster[node]->topology_array[7].vertex_ptr)+0) <<"v ";
		std::cout <<*(tgMaster[node]->topology_array[8].factor_ptr) <<" ";
		std::cout <<(*(tgMaster[node]->topology_array[9].vertex_ptr)+0) <<"v ";

		std::cout <<*(tgMaster[node]->topology_array[10].factor_ptr) <<" ";
		std::cout <<*(tgMaster[node]->topology_array[11].factor_ptr) <<" ";
		std::cout <<(*(tgMaster[node]->topology_array[12].vertex_ptr)+0) <<"v ";

		std::cout <<*(tgMaster[node]->topology_array[13].factor_ptr) <<" ";
		std::cout <<*(tgMaster[node]->topology_array[14].factor_ptr) <<" ";
		std::cout <<(*(tgMaster[node]->topology_array[15].vertex_ptr)+0) <<"v ";
		std::cout <<*(tgMaster[node]->topology_array[16].factor_ptr) <<" ";
		std::cout <<(*(tgMaster[node]->topology_array[17].vertex_ptr)+0) <<"v ";

		std::cout <<*(tgMaster[node]->topology_array[18].factor_ptr) <<" ";
		std::cout <<*(tgMaster[node]->topology_array[19].factor_ptr) <<" ";
		std::cout <<(*(tgMaster[node]->topology_array[20].vertex_ptr)+0) <<"v ";
	}
	else{
		std::cout <<"\n Invalid topology node:"<<node;
		/*error invalid node id*/
	}
}

/*retrive neighborhood size for a given vertex id*/
inline int get_vertex_neighborhood_size(int index, int *factor_count_arr, int *neighbor_count_arr) {
	// this adjacency list is supposed to contain information regarding;
	// - a factor pointer.
	// - the neighbors involved in each factor.
	return(*(factor_count_arr+index) + *(neighbor_count_arr+index));
}

/**
 * This method assumes that the global array was already initialized. Just re-initializing to 0 now.
 */
inline void reinitialize_global_array(int num_nodes) {
	memset (global_array, 0,num_nodes*sizeof(int));
	memset (global_array_copy, 0,num_nodes*sizeof(int));
}

/**
 * This method assumes that the global array was already initialized. Just re-initializing to 0 now.
 */
inline void reinitialize_global_arrayByPartition(int num_nodes) {

	for (int i=0; i<num_sockets_to_use; i++){
		//memset(global_array_partitioned[i], 0,num_nodes*sizeof(int));
          memset(global_array_partitioned[i], 0,sizeGlobalArrayDistribution[i]*sizeof(VERTEX_AGG_VALUE_TYPE));
          memset(global_array_copy_partitioned[i], 0,sizeGlobalArrayDistribution[i]*sizeof(VERTEX_AGG_VALUE_TYPE));
	}
	//memset (global_array_copy, 0,num_nodes*sizeof(int));
}

/*****************************************************************
 * Inference methods
 * multiply factors, get samples, update states
 *****************************************************************/

/**
 * IMPROTANT ASSUMPTION:
 * this method is tailor made for the assumption that the first factor is a unary factor and all other factors
 * are only edge factors. This also assumes that the variable takes only binary values. This function is called
 * when the variable being sampled takes 0 as the value.
 */
inline FACTOR_VALUE_TYPE_INFLATED multiply_factors_zero(struct topology_graph *tg, TOPOLOGY_VALUE_TYPE start_index,
		TOPOLOGY_VALUE_TYPE end_index) {
	// first get the unary factor.
	FACTOR_VALUE_TYPE_INFLATED factor_product = 0.0;
	factor_product += *((tg->topology_array[start_index].factor_ptr)+2);
	start_index++;
	// now fetch for the binary factor.
	while(start_index < end_index) {
		FACTOR_VALUE_TYPE_INFLATED* fact_ind = tg->topology_array[start_index].factor_ptr;
		int jump_index = *(fact_ind);
		int neighbor_val = *(tg->topology_array[start_index+1].vertex_ptr);
		//int neighbor_val = 0;

		factor_product += *(fact_ind + neighbor_val + 2);
		start_index += jump_index;

	}
	return factor_product;
}
/**
 * IMPROTANT ASSUMPTION:
 * this method is tailor made for the assumption that the first factor is a unary factor and all other factors
 * are only edge factors. This also assumes that the variable takes only binary values. This function is called
 * when the variable being sampled takes 1 as the value.
 */
inline FACTOR_VALUE_TYPE_INFLATED multiply_factors_one(struct topology_graph *tg, TOPOLOGY_VALUE_TYPE start_index, TOPOLOGY_VALUE_TYPE end_index) {
	// first get the unary factor.
	FACTOR_VALUE_TYPE_INFLATED factor_product = 0.0;
	factor_product += *((tg->topology_array[start_index].factor_ptr)+3);
	start_index++;
	// now fetch for the binary factor.
	while(start_index < end_index) {
		FACTOR_VALUE_TYPE_INFLATED* fact_ind = tg->topology_array[start_index].factor_ptr;
		int jump_index = *(fact_ind);
		int neighbor_val = *(tg->topology_array[start_index+1].vertex_ptr);
		//int neighbor_val = 1;
		factor_product += *(fact_ind + neighbor_val + 4);
		start_index += jump_index;
	}
	return factor_product;
}

/*
 * update global array partitioned on the socketNode
 * @PARAMS socketNode as id, var as vertex id, value as new value
 * @RETURN: noted
 */
inline void update_local_array(int socketNode, ITERATOR_INDEX var, VERTEX_VALUE_TYPE value){

       *(global_array_partitioned[socketNode] + var) = value;
       //  std::cout <<"\nindex:" <<var<<" - value:"<<value << " next value:"<<global_array_partitioned[i][var];

}

/*
 *  given the copy_begin and copy_end, push the vertex updates to all remote sockets except socketNode
 *  we need thread_begin and thread_end in the case where copy_end < copy_begin
 *  */
inline void refresh_remote_array(int threadIndex, int socketNode, ITERATOR_INDEX copy_begin,
		ITERATOR_INDEX copy_end, ITERATOR_INDEX thread_begin, ITERATOR_INDEX thread_end){

  bool fragmented = (copy_end < copy_begin);

  ITERATOR_INDEX copy_begin1, copy_length, copy_length1;

  if(fragmented){
    copy_begin1  = copy_begin;
    copy_length1 = thread_end - copy_begin;

    copy_begin   = thread_begin;
  }

  if(copy_end == copy_begin){
         copy_begin = thread_begin;
         copy_end = thread_end;
  }

  copy_length = copy_end - copy_begin;

  /*push data to the remote global array replicas*/
  for(int i=0; i<num_sockets_to_use; i++){
         if(i == socketNode)
            continue;

         std::memcpy(
          reinterpret_cast<char*>(global_array_partitioned[i]) + copy_begin,
          reinterpret_cast<char*>(global_array_partitioned[socketNode]) + copy_begin,
          copy_length);
         if(fragmented)
           std::memcpy(
          reinterpret_cast<char*>(global_array_partitioned[i]) + copy_begin1,
          reinterpret_cast<char*>(global_array_partitioned[socketNode]) + copy_begin1,
          copy_length1);

         //*(global_array_partitioned[i] + copy_begin) = *(global_array_partitioned[socketNode]+copy_begin);
    }


}

/**
 * Compute the product of the factors, get the conditional probability
 * and then get the sample of the new vertex state.
 * IMPROTANT ASSUMPTION:
 * This method assumes the first factor is always a unary factor.
 * This also assumes we are only looking at binary values for each variable.
 * For example: vertex 0 have 2 unary factors (0,1) then 4 edge factors, then the vertex id of the neighborn
 * The function has some unrolled the loops to help on performance, but limited usability
 */
inline VERTEX_VALUE_TYPE get_sample(ITERATOR_INDEX var, struct topology_graph *tg,
		double random_num, VERTEX_VALUE_TYPE*ga) {

	TOPOLOGY_VALUE_TYPE topology_offset = tg->vertex_array[var].first_factor_offset;
	int length = tg->vertex_array[var].length;
	TOPOLOGY_VALUE_TYPE        end_index = topology_offset+length;
	FACTOR_VALUE_TYPE_INFLATED prob_values[NUM_OF_VALUES_FOR_VARIABLE];
	FACTOR_VALUE_TYPE_INFLATED maxVal = 0;
	VERTEX_VALUE_TYPE neighbor_val    = 0;
	float normalizing_factor          = 0.0;
	float probability_values[NUM_OF_VALUES_FOR_VARIABLE];


	//std::cout <<"\n\n unary factors:";
	prob_values[0] = ((tg->topology_array[topology_offset].factor_value)); //+2-f0
	topology_offset++;


 	prob_values[1] = ((tg->topology_array[topology_offset].factor_value)); //+3-f1
 	topology_offset++;

 	ITERATOR_INDEX jump;
 	//computing factor product
	while(topology_offset < end_index) {

		//+4
		neighbor_val = ga[tg->topology_array[topology_offset+4].factor_value];
		jump = topology_offset;

	    if (neighbor_val != 0){
	     	   jump++;
	    }

	    prob_values[0]  += tg->topology_array[jump].factor_value;//*( fact_ind+ neighbor_val+2);
	    prob_values[1]  += tg->topology_array[jump+2].factor_value;//*( fact_ind+ neighbor_val+4);
	    topology_offset += topologySizeByVertex;
	 }

    maxVal =max(prob_values[0], prob_values[1]);

    //computing sampling
	// NOTE: This change works only for binary values now!
	probability_values[0] = exp(deflate_factor_value(prob_values[0] - maxVal));
	probability_values[1] = exp(deflate_factor_value(prob_values[1] - maxVal));

	normalizing_factor = probability_values[0] + probability_values[1];

	int new_value = 0;
	float p = 0.0;
	for(; new_value<NUM_OF_VALUES_FOR_VARIABLE-1; new_value++) {
		p += probability_values[new_value]/normalizing_factor;
		if(random_num <= p)
			break;
	}
	return new_value;
}

/*
 * update shared global array
 * */
inline void update_global_array(ITERATOR_INDEX var, VERTEX_VALUE_TYPE value, struct topology_graph *tg) {
	*(tg->vertex_array[var].global_value_ptr) = value;
}


/*
 * update shared global array
 * */
inline void update_global_array(ITERATOR_INDEX var, VERTEX_VALUE_TYPE value){

      for(int i=0; i<num_sockets_to_use; i++){
           *(global_array_partitioned[i] + var) = value;
         //  std::cout <<"\nindex:" <<var<<" - value:"<<value << " next value:"<<global_array_partitioned[i][var];
      }
}

/*
 * compute Convergence statistics
 * every number of sample specific from the configuration file
 * */
inline void computeConvergenceAtIteration(int numaNodeId,int &local_samples_seen,
		int local_samples_collected, int &samplesCounter,
		ITERATOR_INDEX start_index,ITERATOR_INDEX endIndex, int threadIndex) {

	int   differing_nodes =0;
	float prob;
	float diff;

	if(local_samples_seen >= stat_samples_interval && samplesCounter < num_of_stat_samples) {

			for(int j=start_index; j<endIndex; j++) {
				prob = global_array_copy_partitioned[numaNodeId][j]/(local_samples_collected*(1.0));
				diff = global_array_conv_partitioned[numaNodeId][j] - prob;
				if(diff < 0){
					diff = diff*(-1);
				}
				global_array_conv_partitioned[numaNodeId][j] = prob;
				if(diff > threshold) {
					differing_nodes++;
				}
			}

			global_array_nonConv_partitioned[threadIndex][samplesCounter] =differing_nodes;
			local_samples_seen = 0;
			samplesCounter++;
	}
}

/*
 * return the topology pointer partitioned on specific node
 * @PARAMS: node as numa node id
 * @RETURN: pointer to the topology of the node id
 * */
struct topology_graph* get_topology_graph(int node) {

	if (node>=0 && node<= num_sockets_to_use) {
		return tgMaster[node];
	}
	else{
		std::cout <<"\n Invalid topology node:"<<node;
		/*error invalid node id*/
		return NULL;
	}
}

/*****************************************************************
 * Worker methods
 * Find worker tasks, assign workers to core, main inference method
 *****************************************************************/

/*
 * return cpu assigned given the node id
 * enable manage balance workers by node id
 * to keep equal number of assigned threads over the socket
 * */
inline int get_modular_threadID(int node_id_index) {
	int cpu_id = -1;
	// The CPU id to use is:
	// NodeID*MAX_CORE_PER_SOCKET + cpuID (offset)

	// numaNodes[node_id_index]*MAX_CORE_PER_SOCKET -> the first core ID in socket numaNodes[node_id_index]
	// cpuid_counter_array[node_id_index] -> The offset of the CPU id in the socket.
	int first_core_of_socket = numaNodes[node_id_index]*MAX_CORE_PER_SOCKET;
	pthread_mutex_lock(&cpumutex);
	cpu_id = first_core_of_socket + cpuid_counter_array[node_id_index];
	if(cpuid_counter_array[node_id_index] == MAX_CORE_PER_SOCKET-1)
		cpuid_counter_array[node_id_index] = 0;
	else
		cpuid_counter_array[node_id_index] = cpuid_counter_array[node_id_index]+1;
	pthread_mutex_unlock(&cpumutex);
	return cpu_id;
}

/**
 * IMPROTANT: Note that the thread_num_in_socket is set to the a number between 0 and MAX_CORE_PER_SOCKET-1.
 * If we run with number of threads that would lead to more than MAX_CORE_PER_SOCKET threads per socket, then
 * this method will not return the correct value for thread_num_in_socket (because of the if-else). This wrong value
 * might also lead to wrong Gibbs sampling since some variables might never get sampled, in case of PARTITION.
 * ASSUMPTION: We will not be running more than the MAX_CORE_PER_SOCKET-1 threads per socket.
 */
inline int get_modular_threadID(int* thread_num_in_socket, int node_id_index) {
	int cpu_id = -1;
	// The CPU id to use is:
	// NodeID*MAX_CORE_PER_SOCKET + cpuID (offset)

	// numaNodes[node_id_index]*MAX_CORE_PER_SOCKET -> the first core ID in socket numaNodes[node_id_index]
	// cpuid_counter_array[node_id_index] -> The offset of the CPU id in the socket.
	int first_core_of_socket = numaNodes[node_id_index]*MAX_CORE_PER_SOCKET;
	pthread_mutex_lock(&cpumutex);
	cpu_id = first_core_of_socket + cpuid_counter_array[node_id_index];
	*thread_num_in_socket = cpuid_counter_array[node_id_index];
	if(cpuid_counter_array[node_id_index] == MAX_CORE_PER_SOCKET-1)
		cpuid_counter_array[node_id_index] = 0;
	else
		cpuid_counter_array[node_id_index] = cpuid_counter_array[node_id_index]+1;
	pthread_mutex_unlock(&cpumutex);
	return cpu_id;
}

/*
 * Set the thread to specific core in order to
 * avoid context swithed between cores
 * */
inline int stick_this_thread_to_core(int core_id) {
  int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
  if (core_id < 0 || core_id >= num_cores)
    return EINVAL;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  pthread_t current_thread = pthread_self();
  return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

/*
 * retrieves the socket id
 * given a node id,
 * enable manages different ids for socket/nodes
 * */
int get_socketid_index(int node_id_index) {
	if(node_id_index < num_sockets_to_use)
		return numaNodes[node_id_index];
	else {
		std::cout << " ERROR!!!!! invalid node id= "<<node_id_index << std::endl;
		exit(2);
	}
}

/*
 * Determine number of vertex to be assinged to specific thread/worker.
 * Each socket has a contigorous partition, whose size may vary.
 * All threads in the same socket uniformly partition the work
 */
void find_work_index_for_thread(int node_id_index, int thread_id_in_socket, int* socketPartitionSizes,
		ITERATOR_INDEX* thread_iter_start_index, ITERATOR_INDEX* thread_iter_end_index) {

	ITERATOR_INDEX segment_size = sizeGlobalArrayDistribution[node_id_index];

	ITERATOR_INDEX segment_start_index=0;

	for(int i=0; i<node_id_index; i++)
		segment_start_index += socketPartitionSizes[i];

	ITERATOR_INDEX segment_end_index = segment_start_index + segment_size;
	int max_threads_per_socket_to_use = threads_per_socket[node_id_index];

	ITERATOR_INDEX thread_work_size = segment_size/max_threads_per_socket_to_use;

	*thread_iter_start_index = segment_start_index + thread_work_size*thread_id_in_socket;
	*thread_iter_end_index = *thread_iter_start_index + thread_work_size;

	if(thread_id_in_socket == max_threads_per_socket_to_use-1) {
		// the last thread in socket may have to do some extra work.
		*thread_iter_end_index = segment_end_index;
	}
	if(thread_id_in_socket == max_threads_per_socket_to_use-1 && node_id_index == num_sockets_to_use-1) {
		// the last thread in the last socket might have to work on more samples. so the ending value is the num_nodes.
		*thread_iter_end_index = num_of_nodes;
	}
}

/*
 * Determine number of vertex to be assinged to specific thread/worker.
 * With partition method, the assumption is that the dataset is split into contiguous equal segments.
 * For instance, all threads in socket 0, will work in the first segment and all threads in the
 * last socket will be working on the last segment.
 * The other behavior of this code is that, in some cases, the number of threads in some socket might be lesser
 * (normally less by one thread) than other sockets. The threads in such sockets will end up doing more work here
 * since the "segment" size does not change per socket and there will be lesser number of threads working on the
 * same amount of load, compared to another socket which might have more threads to work on the segment load.
 */
void find_work_index_for_thread(int node_id_index, int thread_id_in_socket,
		ITERATOR_INDEX* thread_iter_start_index, ITERATOR_INDEX* thread_iter_end_index) {

	ITERATOR_INDEX segment_size = num_of_nodes/num_sockets_to_use;
	ITERATOR_INDEX segment_start_index = node_id_index*segment_size;
	ITERATOR_INDEX segment_end_index = segment_start_index + segment_size;
	int max_threads_per_socket_to_use = threads_per_socket[node_id_index];

	ITERATOR_INDEX thread_work_size = segment_size/max_threads_per_socket_to_use;
	*thread_iter_start_index = segment_start_index + thread_work_size*thread_id_in_socket;
	*thread_iter_end_index = *thread_iter_start_index + thread_work_size;

	if(thread_id_in_socket == max_threads_per_socket_to_use-1) {
		// the last thread in socket may have to do some extra work.
		*thread_iter_end_index = segment_end_index;
	}
	if(thread_id_in_socket == max_threads_per_socket_to_use-1 && node_id_index == num_sockets_to_use-1) {
		// the last thread in the last socket might have to work on more samples. so the ending value is the num_nodes.
		*thread_iter_end_index = num_of_nodes;
	}
}

/*
 * Determine number of vertex to be assinged to specific thread/worker.
 * This function distributes work among all the threads uniformly. This does NOT consider the number of sockets used
 * like the other overloaded method. So, irrespective of the number of threads in each socket, the amount of work
 * done by each thread will remain the same. This is based on Fei's benchmarking code.
 */
void find_work_index_for_thread(int thread_index,
		ITERATOR_INDEX* thread_iter_start_index, ITERATOR_INDEX* thread_iter_end_index) {

	ITERATOR_INDEX thread_work_size = num_of_nodes/num_of_threads;
	*thread_iter_start_index = thread_work_size*thread_index;
	*thread_iter_end_index = *thread_iter_start_index + thread_work_size;

	if(thread_index == num_of_threads-1) {
		// the last thread in the last socket might have to work on more samples. so the ending value is the num_nodes.
		*thread_iter_end_index = num_of_nodes;
	}
}

/*
 * Determine number of vertex to be assinged to specific thread/worker.
 * */
void find_work_index_for_threadByPartition(int threadIndex,
		ITERATOR_INDEX* threadIterStartIndex,
		ITERATOR_INDEX* threadIterEndIndex,
		int nodePartitionId, int numberOfThreadsOnSocket ) {


	ITERATOR_INDEX threadWorkSize =  sizeGlobalArrayDistribution[nodePartitionId]/numberOfThreadsOnSocket;
	*threadIterStartIndex = startThreadIndex[nodePartitionId]+ threadWorkSize*(threadIndex%numberOfThreadsOnSocket);
	*threadIterEndIndex   = *threadIterStartIndex + threadWorkSize;
	if((threadIndex+1)%numberOfThreadsOnSocket==0){
		*threadIterEndIndex= endThreadIndex[nodePartitionId];
	}
}


/**
 * Main thread function: each worker will execute this method
 * to run the inference model
 * This function partitions the total worker tasks equally among all the threads.
 * This method executes updates to remote partitions in batched
 */
void *launch_tparallel_partition_uniform_thread(void* arg) {
	int thread_index = *(int*)arg;
	int trheadBySocket = num_of_threads/num_sockets_to_use;
	int node_id_index = thread_index /trheadBySocket;
	int node_id = get_socketid_index(node_id_index);



	struct bitmask* mask = numa_allocate_nodemask();
	numa_bitmask_setbit(mask, node_id);
	numa_bind(mask);
	numa_set_membind(mask);

	int thread_number_in_socket = 0;
	int cpu_id = get_modular_threadID(&thread_number_in_socket, node_id_index);
	stick_this_thread_to_core(cpu_id);

	ITERATOR_INDEX thread_iter_start_index;
	ITERATOR_INDEX thread_iter_end_index;
	VERTEX_VALUE_TYPE sample_value=0;
	VERTEX_VALUE_TYPE *ga= global_array_partitioned[node_id];

	int local_samples_collected = 0;
	int local_samples_seen = 0;
	int samplerCounter     = 0;
	double x;

	//add memory for convergence stats assigned to the thread
	if (generate_statistics){
		global_array_nonConv_partitioned[thread_index]=(VERTEX_AGG_VALUE_TYPE*) numa_alloc_onnode(sizeof(VERTEX_AGG_VALUE_TYPE)*num_of_stat_samples,node_id_index);
		memset(global_array_nonConv_partitioned[thread_index], 0, num_of_stat_samples * sizeof(VERTEX_AGG_VALUE_TYPE));
	}

	find_work_index_for_threadByPartition(thread_index, &thread_iter_start_index, &thread_iter_end_index,
			node_id_index,num_of_threads/num_sockets_to_use);

	struct topology_graph *tg = get_topology_graph(node_id_index);
	struct drand48_data randBuffer;
	srand48_r((ITER+1)*thread_index,&randBuffer);

	/*bached updates params*/
	uint64_t numUpdates	= 0;
	uint64_t refresh_next = kRefreshIntervalPercent; // interval (batch size)
	uint64_t refresh_trigger_at = refresh_next; // no delays between threads;
	uint64_t latestUpdate = thread_iter_start_index;
	uint64_t copy_end;

	for(int i=0; i<num_total_iters; i++) {
		for(ITERATOR_INDEX j=thread_iter_start_index; j<thread_iter_end_index; j++) {

           drand48_r(&randBuffer,&x);

     		sample_value = get_sample(j, tg, x,ga);

			/*deprecated update
			 *const VERTEX_VALUE_TYPE old_val = *(tg->vertex_array[j].global_value_ptr);
			 * if(old_val != sample_value){
			 *	update_global_array(j, sample_value);
			}*/

           update_local_array(node_id_index,j,sample_value);
           numUpdates++;
           if(numUpdates==refresh_trigger_at){
                 if(j==thread_iter_start_index && latestUpdate>thread_iter_start_index){
                	 copy_end = thread_iter_end_index;
                 }else{
                     copy_end = j+1;
                 }

                 refresh_remote_array(thread_index,node_id_index,latestUpdate,copy_end,thread_iter_start_index,thread_iter_end_index);
                 refresh_trigger_at += refresh_next;
                 latestUpdate = copy_end;
            }


			if (sample_value== 0){
				global_array_copy_partitioned[node_id_index][j] = global_array_copy_partitioned[node_id_index][j] + 1;
		   }


		}

		if(generate_statistics) {
			local_samples_collected++;
			local_samples_seen++;
			computeConvergenceAtIteration(node_id_index,local_samples_seen,local_samples_collected,samplerCounter,
					thread_iter_start_index,thread_iter_end_index,thread_index);
		    }
			pthread_barrier_wait(&barrier1);
	}
	return(NULL);
}

/**
 * Main thread function: each worker will execute this method
 * to run the inference model
 * This function partitions the total task equally among all the threads.
 * The updates are push instanstaneously to the remote partitions
 */
void *launch_tparallel_partition_uniform_threadNoBatched(void* arg) {
	int thread_index = *(int*)arg;
	int trheadBySocket = num_of_threads/num_sockets_to_use;
	int node_id_index = thread_index /trheadBySocket;
	int node_id = get_socketid_index(node_id_index);


	struct bitmask* mask = numa_allocate_nodemask();
	numa_bitmask_setbit(mask, node_id);
	numa_bind(mask);
	numa_set_membind(mask);

	int thread_number_in_socket = 0;
	int cpu_id = get_modular_threadID(&thread_number_in_socket, node_id_index);
	stick_this_thread_to_core(cpu_id);

	ITERATOR_INDEX thread_iter_start_index;
	ITERATOR_INDEX thread_iter_end_index;
	VERTEX_VALUE_TYPE sample_value=0;
	VERTEX_VALUE_TYPE *ga= global_array_partitioned[node_id];

	int local_samples_collected = 0;
	int local_samples_seen = 0;
	int samplerCounter     = 0;

	//add memory for convert
	if (generate_statistics){
		global_array_nonConv_partitioned[thread_index]=(VERTEX_AGG_VALUE_TYPE*) numa_alloc_onnode(sizeof(VERTEX_AGG_VALUE_TYPE)*num_of_stat_samples,node_id_index);
		memset(global_array_nonConv_partitioned[thread_index], 0, num_of_stat_samples * sizeof(VERTEX_AGG_VALUE_TYPE));
	}

	find_work_index_for_threadByPartition(thread_index, &thread_iter_start_index, &thread_iter_end_index,
			node_id_index,num_of_threads/num_sockets_to_use);

	struct topology_graph *tg = get_topology_graph(node_id_index);
	struct drand48_data randBuffer;
	srand48_r((ITER+1)*thread_index,&randBuffer);

	double x;
	for(int i=0; i<num_total_iters; i++) {
		for(ITERATOR_INDEX j=thread_iter_start_index; j<thread_iter_end_index; j++) {
			drand48_r(&randBuffer,&x);

			//if (tg->vertex_array[j].prior){
			//	continue;
			//}
			const VERTEX_VALUE_TYPE old_val = *(tg->vertex_array[j].global_value_ptr);
			sample_value = get_sample(j, tg, x,ga);


			if(old_val != sample_value){
				update_global_array(j, sample_value);
			}

			if (sample_value== 0){
				global_array_copy_partitioned[node_id_index][j] = global_array_copy_partitioned[node_id_index][j] + 1;
		   }
		}

			if(generate_statistics) {
				local_samples_collected++;
				local_samples_seen++;
				computeConvergenceAtIteration(node_id_index,local_samples_seen,local_samples_collected,samplerCounter,
						thread_iter_start_index,thread_iter_end_index,thread_index);
		    }
			pthread_barrier_wait(&barrier1);
	}
	return(NULL);
}


/*
 * Main thread launcher based on user configuration
 * */
void launch_sequential_parallel() {
	// initializing barriers
	pthread_t tid[num_of_threads];

	/*start time counters*/
	gettimeofday(&timestamp, NULL);

	if(pthread_barrier_init(&barrier1, NULL, num_of_threads)) {
		cout <<"Could not create a barrier 1\n";
	}


	for (int i = 0; i < num_of_threads; i++) {
		if (pthread_create(&tid[i], NULL, launch_tparallel_partition_uniform_thread, new int(i))) {
			cout << "Cannot create thread " << i << endl;
		}
	}

	for (int j = 0; j < num_of_threads; j++) {
		pthread_join(tid[j], NULL);
	}
	pthread_barrier_destroy(&barrier1);
}
/*
 * Launch inference for a singl thread
 * */
inline void launch_single_thread() {
	struct topology_graph *tg = get_topology_graph(0);
	struct drand48_data randBuffer;
	srand48_r(1,&randBuffer);
	double x;
	for(int i=0; i<num_total_iters; i++) {
		for(int j=0; j<num_of_nodes; j++) {
			drand48_r(&randBuffer,&x);
			VERTEX_VALUE_TYPE sample_value = get_sample(j, tg, x, global_array);
			update_global_array(j, sample_value, tg);
		}
		for (int j = 0; j < num_of_nodes; j++) {
			if (global_array[j] == 0) {
				global_array_copy[j] = global_array_copy[j] + 1;
			}
		}
	}
}



/*************************************************************************************************************************
 * Methods to create graph topology in memory:
 *
 * read_inputTopology file
 * buildTopology
 * CopyTopology
 *
 **************************************************************************************************************************/

/**
 * Read the topology as neighbors in a edge list
 * Create temporal structures for the file values that will be used to build the
 * topology_array structure.
 * temp_topology create the contiguous array of the neiborns for each node starting for the node=0 to the number of nodes.
 * @PARAMS: number of nodes in the graph, total number of neighbors, vertex_array and graphFileMap
 * @RETURN: nothing
 */
void read_inputTopology(size_t num_nodes, size_t topologySize, int numaNodeId,
		struct vertex_info *vertex_array,GraphFileMemoryMap &graphFileMapPtr, FACTOR_VALUE_TYPE_INFLATED * unaryFactors) {

	TOPOLOGY_VALUE_TYPE  offset =  0;
	graphFileMapPtr.topology_offset_global    =  (TOPOLOGY_VALUE_TYPE *)numa_alloc_onnode(num_nodes *    sizeof(TOPOLOGY_VALUE_TYPE),numaNodeId);
	graphFileMapPtr.temp_topology_global      =  (TOPOLOGY_VALUE_TYPE *)numa_alloc_onnode(topologySize * sizeof(TOPOLOGY_VALUE_TYPE), numaNodeId);


	memset (graphFileMapPtr.topology_offset_global,   0,sizeof(TOPOLOGY_VALUE_TYPE)*num_nodes);
	memset (graphFileMapPtr.temp_topology_global, 0, sizeof(TOPOLOGY_VALUE_TYPE)*topologySize );

	for(size_t i=0; i<num_nodes; i++) {
		graphFileMapPtr.topology_offset_global[i]    = offset;
		//vertex_array[i].fist_factor_offset=offset;
		//adding unary factors
		graphFileMapPtr.temp_topology_global[offset] =  (unaryFactors[i*numVars]);//inflate_factor_value(-0.69314);
		graphFileMapPtr.temp_topology_global[offset+1] =(unaryFactors[(i*numVars)+1]);// inflate_factor_value(-0.69314);
		offset =  offset + vertex_array[i].length;
	}

	graphFileMapPtr.lastVertexLength = vertex_array[num_nodes-1].length;
	getNeigborns(graphFileMapPtr,graphFileMapPtr.temp_topology_global, graphFileMapPtr.topology_offset_global,num_nodes, numaNodeId);
	std::cout <<"\n Total edge factor computed:" <<graphFileMapPtr.num_of_edges;
}

/**
 * Build the topology_array in  memory,
 * it uses temp_topology and temp_topology_offsets to replace nodes and factors id for pointers to global structure and factor tables.
 * @PARAM: topology_array to fill,vertex_array to update
 * @RETURN: noting
 */
void buildTopologyArray(int numa_node, size_t num_nodes, union topology_array_element *topology_array,
		FACTOR_VALUE_TYPE_INFLATED *factor_tables, struct vertex_info *vertex_array,
		ITERATOR_INDEX startTopologyIndex,
		ITERATOR_INDEX endTopologyIndex,ITERATOR_INDEX topologySize ){


	TOPOLOGY_VALUE_TYPE  topology_index = 0;
	TOPOLOGY_VALUE_TYPE *temp_topology   = NULL;
	TOPOLOGY_VALUE_TYPE *topology_offset = NULL;

	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);



	/*pointers to the new structure*/
	temp_topology   = graphFileMap.temp_topology_global;
	topology_offset = graphFileMap.topology_offset_global;

	#ifdef DEBUG_GRAPHSTATS
		displayArrays(graphFileMap.topology_offset_global,num_nodes);
		displayArrays(topology_offset,num_nodes);
		displayArrays(graphFileMap.temp_topology_global,graphFileMap.topologyArraySize);
		displayArrays(temp_topology,graphFileMap.topologyArraySize);
		checkSumTopology(graphFileMap.topology_offset_global,num_nodes);
		checkSumTopology(topology_offset,num_nodes);
		checkSumTopology(graphFileMap. temp_topology_global,graphFileMap.topologyArraySize);
		checkSumTopology(temp_topology,graphFileMap.topologyArraySize);
	 #endif
	//std::cout <<"\n Building/Copy the topology array for numa node: "<<numa_node<<"\n";
    //std::cout <<"\nCOPY topology size:"<<topologySize;
    memcpy (topology_array, temp_topology+topology_offset[startTopologyIndex],topologySize*sizeof(union topology_array_element));


    for(ITERATOR_INDEX i=startTopologyIndex; i<endTopologyIndex; i++) {
		vertex_array[i].first_factor_offset = topology_index;
		topology_index+=vertex_array[i].length;
    }


    #ifdef DEBUG_TIME_ON
		double loadTime = 0;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		loadTime = (tv.tv_sec - start_tv.tv_sec) +
		 			(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
		std::cout <<"\n Build Topology TIME: "<<loadTime;
	#endif
}

/*
 * original method to copy and buid topology assuming that the vertex id are pointers, instead of offsets
 * */
void reBuildTopologyArray(int numa_node, size_t num_nodes, union topology_array_element *topology_array,
		FACTOR_VALUE_TYPE_INFLATED *factor_tables, struct vertex_info *vertex_array,
		ITERATOR_INDEX startTopologyIndex,
		ITERATOR_INDEX endTopologyIndex,ITERATOR_INDEX topologySize ){



	TOPOLOGY_VALUE_TYPE  topology_index = 0;
	TOPOLOGY_VALUE_TYPE *temp_topology   = NULL;
	TOPOLOGY_VALUE_TYPE *topology_offset = NULL;
	TOPOLOGY_VALUE_TYPE offset           = 0;

	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	/*pointers to the new structure*/
	temp_topology   = graphFileMap.temp_topology_global;
	topology_offset = graphFileMap.topology_offset_global;

	#ifdef DEBUG_GRAPHSTATS
		displayArrays(graphFileMap.topology_offset_global,num_nodes);
		displayArrays(topology_offset,num_nodes);
		displayArrays(graphFileMap.temp_topology_global,graphFileMap.topologyArraySize);
		displayArrays(temp_topology,graphFileMap.topologyArraySize);
		checkSumTopology(graphFileMap.topology_offset_global,num_nodes);
		checkSumTopology(topology_offset,num_nodes);
		checkSumTopology(graphFileMap. temp_topology_global,graphFileMap.topologyArraySize);
		checkSumTopology(temp_topology,graphFileMap.topologyArraySize);
	 #endif


	cout<<"\nBuilding for numa node: "<<numa_node<<"\n";
  //  std::cout <<"\nCOPY topology size:"<<topologySize;
    memcpy (topology_array, temp_topology+topology_offset[startTopologyIndex],topologySize*sizeof(union topology_array_element));


    for(ITERATOR_INDEX i=startTopologyIndex; i<endTopologyIndex; i++) {
    	offset = topology_offset[(int64_t)i];
		vertex_array[i].first_factor_offset = topology_index;
		topology_index+=vertex_array[i].length;
		//unary factors
		//std::cout << "\nuniary factor:"<<topology_index;
		topology_array[topology_index].factor_value=*(temp_topology+offset+0);
		//std::cout << "\nvalF["<< topology_index<<"]:"<<topology_array[topology_index].factor_value;
		topology_index++;
		topology_array[topology_index].factor_value=*(temp_topology+offset+1);
		//std::cout << "\nvalF["<< topology_index<<"]:"<<topology_array[topology_index].factor_value;
		topology_index++;
		//
		//std::cout << "\nvertex factor:";
		for(int j=numVars; j<vertex_array[i].length; j++) {
					if((j-1)%(topologySizeByVertex) ==0){
							//topology_array[topology_index].vertex_ptr =  get_vertex_global_pointerByVariedSizePartition(*(temp_topology+offset+j),numa_node);
						topology_array[topology_index].factor_value=*(temp_topology+offset+j);
							//std::cout << "\nvalV["<< topology_index<<"]:"<<*(topology_array[topology_index].vertex_ptr)+48;
					}else{
						topology_array[topology_index].factor_value=*(temp_topology+offset+j);
						//std::cout << "\nvalF["<< topology_index<<"]:"<<topology_array[topology_index].factor_value;
					}
					topology_index++;
		}
    }

    std::cout << "\nchecking again";

#ifdef DEBUG_GRAPHSTATS
    for (size_t k =0, i=0; k<topology_index; k++,i++){

    	std::cout << "\nuniary factor:"<<k;
    	std::cout << "\nvalF:"<<topology_array[k++].factor_value;
    	std::cout << "\nvalF:"<<topology_array[k++].factor_value;
    	std::cout << "\nvertex factors size:"<<vertex_array[i].length;

    	for(int j=numVars; j<vertex_array[i].length; j++) {
			if ((j-1)%() ==0){
				std::cout << "\nvalV: "<<*(topology_array[j].vertex_ptr)+48;
			}else{
				std::cout << "\nvalF:"<<topology_array[j].factor_value;
			}
			k++;
        }
    	k--;

    }
        int a;
        std::cout <<"\n enter zeroto continue: [0]";
        std::cin>>a;
#endif

    #ifdef DEBUG_TIME_ON
		double loadTime = 0;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		loadTime = (tv.tv_sec - start_tv.tv_sec) +
		 			(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
		std::cout <<"\n Build Topology TIME: "<<loadTime;
	#endif
}
/**
 * Replicate graph topology to sockets.
 * assuming the node IDs start with a 0 (so the index itself can work out as node IDs too.
 * @PARAMS: input filename, topology for the sokcet, total of vertex
 * @RETURN: nothing
 */
void copyGraphTopology(int thread_index,struct topology_graph *tg, int num_nodes,
		GraphFileMemoryMap &localGraphFile) {

	int    numaNode = numaNodes[thread_index];

	tg->factor_tables  = NULL;
	tg->topology_array = NULL;
	tg->vertex_array   = NULL;

	//std::cout <<"\n=======>START Copy data for socket:" <<numaNode;

	// copy factors
	int factor_table_length = (NUM_OF_UNARY_FACTORS*UNARY_FACTOR_LENGTH) + (NUM_OF_BINARY_FACTORS*BINARY_FACTOR_LENGTH);
	tg->factor_tables       = (FACTOR_VALUE_TYPE_INFLATED*) numa_alloc_onnode(factor_table_length*sizeof(FACTOR_VALUE_TYPE_INFLATED), numaNode);
	memcpy(tg->factor_tables, tgMaster[0]->factor_tables, factor_table_length*sizeof(FACTOR_VALUE_TYPE_INFLATED));

	//copy vertex array
	tg->vertex_array = (struct vertex_info *) numa_alloc_onnode(localGraphFile.num_of_nodes*sizeof(struct vertex_info), numaNode);
	memcpy(tg->vertex_array,tgMaster[0]->vertex_array, localGraphFile.num_of_nodes*sizeof(struct vertex_info));

        //@Fei changed to pointing to local
	for(int i=0; i<num_nodes; i++) {
          tg->vertex_array[i].global_value_ptr = get_vertex_global_pointerByVariedSizePartition(i, numaNode);
        }

        //allocate topology array memory.
	//std::cout<< "\nTotal topology_array size:"<<localGraphFile.topologyArraySize;

	ITERATOR_INDEX startTopologyIndex=0;
	ITERATOR_INDEX endTopologyIndex=0;

	ITERATOR_INDEX localTopologySize= getTopologySizeAtNodeId(numaNode,startTopologyIndex,endTopologyIndex);
	tg->topology_array = (union topology_array_element*)numa_alloc_onnode(localTopologySize*sizeof(union topology_array_element),numaNode);
	buildTopologyArray(numaNode, num_nodes, tg->topology_array, tg->factor_tables, tg->vertex_array,
				startTopologyIndex,endTopologyIndex,localTopologySize);

}



/* Initialize global array based on the input list of vertex
 * make interleave allocation of the stucture over the selected sockets to run
 * in the input filename/
 * Allocate the memory for array using interleave policy
 * @PARAMS: input file name
 * @RETURNS: the number of vertex in the input file.
 *
 * */

int initialize_global_array(GraphFileMemoryMap & graphFileMapPtr) {
	int    vertices = 0;
	struct bitmask* mask = numa_parse_nodestring("0"); /*Default node*/


	vertices = getNumberOfVertices(graphFileMapPtr);



	std::cout <<"\n Number of Vertices: "<<vertices;
	if (vertices == 0 ){
			return 0; /*invalid number of vertex*/
	}

	/*Setting for interleave for the set of nodes seleted*/
	numa_bitmask_clearall(mask);
	for (int i=0; i< num_sockets_to_use; i++){
		numa_bitmask_setbit(mask, numaNodes[i]);
	}

	numa_set_interleave_mask(mask);
	global_array 	  = (VERTEX_VALUE_TYPE*)numa_alloc_interleaved_subset(vertices * sizeof(VERTEX_VALUE_TYPE), mask);
	global_array_copy = (VERTEX_VALUE_TYPE*)numa_alloc_interleaved_subset(vertices * sizeof(VERTEX_VALUE_TYPE), mask);

	memset(global_array, 0, vertices * sizeof(VERTEX_VALUE_TYPE));
	memset(global_array_copy, 0, vertices * sizeof(VERTEX_VALUE_TYPE));

	return vertices;
}

/*
 * Create and init global array by partitions using balance worksize by thread.;
 * split the vertices over the  partitions and allocat 2-d array:
 * nxm , n -> number of sockets, m, partition size.
 * partition size assumes that the last socket will keep the rest
 * of vertices when the number of vertices can not be totally balanced
 * @PARAM: graph file pointer
 * @RETURN 0-> error, 1 -> success
 * @Tere
 */
int initialize_global_arrayByPartition(std::string &graphfileName,GraphFileMemoryMap & graphFileMapPtr) {

	int vertices                = 0;
	vertices = getNumberOfVertices(graphFileMapPtr);
	num_of_nodes= vertices;

	//TODO: to read from alchemy
	numVars=2;
	numEdgeFactors=pow(numVars,2);
	topologySizeByVertex=numEdgeFactors+1;

	std::cout <<"\n Number of Vertices: "<<vertices;
	std::cout <<"\n Number of vars: "<<numVars;
	std::cout <<"\n Number of vars: "<<numEdgeFactors;
	if (vertices == 0 ){
			return 0; /*invalid number of vertices*/
	}


	if (vertices<num_sockets_to_use){
		std::cout <<"Global array partition can not be done. The number of sockets is more than the data elements.";
		return 0;
	}

	//master pointers (bi-dimensional array for global array partitions
	global_array_partitioned       = (VERTEX_VALUE_TYPE**)numa_alloc_onnode(num_sockets_to_use * sizeof(VERTEX_VALUE_TYPE), 0);
	global_array_copy_partitioned    = (VERTEX_AGG_VALUE_TYPE**)numa_alloc_onnode(num_sockets_to_use * sizeof(VERTEX_AGG_VALUE_TYPE), 0);
	global_array_conv_partitioned    = (FLOAT_AGG_VALUE_TYPE**)numa_alloc_onnode(num_sockets_to_use * sizeof(FLOAT_AGG_VALUE_TYPE), 0);
	global_array_nonConv_partitioned =(VERTEX_AGG_VALUE_TYPE**)numa_alloc_onnode(num_of_threads * sizeof(VERTEX_AGG_VALUE_TYPE), 0);


	/*array to store global state distributions*/
    sizeGlobalArrayDistribution = (ITERATOR_INDEX*)malloc(num_sockets_to_use*sizeof(ITERATOR_INDEX));

    /*array to read global states sizes*/
	readPartitionSizes(graphfileName+PARTITIONSIZE,num_sockets_to_use,sizeGlobalArrayDistribution);
	for(int i=0; i<num_sockets_to_use; i++)
	             std::cout <<"\npartition of socket "<<i<<" is "<< sizeGlobalArrayDistribution[i];

	#ifdef DEBUG_GRAPHSTATS
	std::cout <<"\n**Partition Stats                ";
	std::cout <<"\n \tSocket  to use:                "<<	num_sockets_to_use;
	std::cout <<"\n \tThreads to use:                "<<	num_of_threads;
	std::cout <<"\n \tThreads by Socket:             "<<numberOfThreadsBySocket;
	std::cout <<"\n \tThreads by last Socket:        "<<lastSocketThreads;
	std::cout <<"\n \tuniform array size by thread:  "<<vertices/num_of_threads;
	std::cout <<"\n \tuniform array size by socket:  "<<partitionSizeBySocket;
	#endif



	for (int i=0; i<num_sockets_to_use; i++){

		//std::cout <<"\n \t Socket i: "<<i <<" work size:"<<sizeGlobalArrayDistribution[i];

       global_array_partitioned[i]  = (VERTEX_VALUE_TYPE*)numa_alloc_onnode(num_of_nodes * sizeof(VERTEX_VALUE_TYPE), i);
       memset(global_array_partitioned[i], 0, num_of_nodes * sizeof(VERTEX_VALUE_TYPE));


		global_array_copy_partitioned[i]  = (VERTEX_AGG_VALUE_TYPE*)numa_alloc_onnode(num_of_nodes * sizeof(VERTEX_AGG_VALUE_TYPE), i);
		memset(global_array_copy_partitioned[i], 0, num_of_nodes * sizeof(VERTEX_AGG_VALUE_TYPE));

		global_array_conv_partitioned[i]  = (FLOAT_AGG_VALUE_TYPE*) numa_alloc_onnode(num_of_nodes * sizeof(FLOAT_AGG_VALUE_TYPE), i);

		for (int k=0; k <num_of_nodes; k++){
					global_array_conv_partitioned[i][k]=3.0;
		}
	}


	/*initialize worker indices to determine vertex id to start and end*/
	startThreadIndex = (ITERATOR_INDEX*)malloc(num_sockets_to_use*sizeof(ITERATOR_INDEX));
	endThreadIndex   = (ITERATOR_INDEX*)malloc(num_sockets_to_use*sizeof(ITERATOR_INDEX));

	startThreadIndex[0]= 0;
	endThreadIndex[0]  = sizeGlobalArrayDistribution[0];
	for (int m=1; m<num_sockets_to_use; m++){
		startThreadIndex[m]    = startThreadIndex[m-1] + sizeGlobalArrayDistribution[m];
		endThreadIndex[m]      = endThreadIndex[m-1] + sizeGlobalArrayDistribution[m];

	//std::cout <<"\ns:"<<startThreadIndex[m] <<"\t e: "<<endThreadIndex[m];
	}

	return vertices;
}

/*
 * retrieves the size of the topology given specific vertex id to start and end
 *
 * */
ITERATOR_INDEX getTopologySizeAtNodeId(int numaNodeId, ITERATOR_INDEX &startIndex, ITERATOR_INDEX &endInddex){

	ITERATOR_INDEX totalsize=0;

	for (int m=0; m<num_sockets_to_use; m++){
		endInddex += sizeGlobalArrayDistribution[m];
		if (m == numaNodeId)
			break;
		startIndex += sizeGlobalArrayDistribution[m];
	}


#ifdef DEBUG_TOPOLOGY
	std::cout <<"\n===Topology Partition Stats====";
	std::cout <<"\n local partition for:"<<numaNodeId;
	std::cout <<"\n end index:"<<endInddex;
	std::cout <<"\n start index:"<<startIndex;
	std::cout <<"\n total topology size:" <<totalsize<<"\n";
#endif

	if (numaNodeId+1 == num_sockets_to_use){
			totalsize = graphFileMap.topology_offset_global[endInddex-1]-graphFileMap.topology_offset_global[startIndex];
			totalsize+= graphFileMap.lastVertexLength;
	}
	else{
		totalsize = graphFileMap.topology_offset_global[endInddex]-graphFileMap.topology_offset_global[startIndex];
	}


	return (totalsize);
}

/**
 * Parse input file in order to load nodes, neighbors and factors.
 * some limiting assumptions made here!
 * assuming the node IDs start with a 0 (so the index itself can work out as node IDs too.
 * @PARAMS: input filename, topology for the sokcet, total of vertex
 * @RETURN: nothing
 */
void read_inputfile(const std::string input_filename, int thread_index,struct topology_graph *tg,
		int num_nodes, GraphFileMemoryMap &localGraphFile) {


	int *factor_count_arr = NULL;
	FACTOR_VALUE_TYPE_INFLATED *unaryfactors=NULL;
	int  numaNode         = numaNodes[thread_index];
	size_t vertexNESize;
	size_t topologyArraySize;

	tg->factor_tables  = NULL;
	tg->topology_array = NULL;
	tg->vertex_array   = NULL;

	std::cout <<"\n Allocating data structures for topology socket:" <<numaNode<<"\n";

	// create an array to count the number of neighbors of each node.
	factor_count_arr = (int *)malloc(num_nodes*sizeof(int));
	memset(factor_count_arr, 0,num_nodes*sizeof(int));


	unaryfactors =(FACTOR_VALUE_TYPE_INFLATED*)malloc(num_nodes*numVars*sizeof(FACTOR_VALUE_TYPE_INFLATED));
	memset(unaryfactors,0,num_nodes*numVars*sizeof(FACTOR_VALUE_TYPE_INFLATED));

	//std::cout <<"\n reading unary factors..";
	getUnaryFactorCounter(localGraphFile, factor_count_arr, num_nodes, unaryfactors);
	std::cout <<"\n Reading edge factors..";
	getEdgeNeigbornsCounter(localGraphFile,factor_count_arr, num_nodes);

	// populate info about the factors
	//std::cout <<"\n Initializing factor table";
	int factor_table_length = (NUM_OF_UNARY_FACTORS*UNARY_FACTOR_LENGTH) + (NUM_OF_BINARY_FACTORS*BINARY_FACTOR_LENGTH);
	tg->factor_tables       = (FACTOR_VALUE_TYPE_INFLATED*)numa_alloc_onnode(factor_table_length*sizeof(FACTOR_VALUE_TYPE_INFLATED), numaNode);
	populate_factors(tg->factor_tables );




	// now create the topology related data structures.
	// allocate memory for individual node.
	// create an array to count the number of neighbors of each node.
	//std::cout <<"\n initializing vertex array";
	topologyArraySize=0;
	tg->vertex_array = (struct vertex_info *)numa_alloc_onnode(num_nodes*sizeof(struct vertex_info), numaNode);

	//allocating only for the local:
	//for(int i=0; i<num_nodes; i++) {

	for(int i=0; i<num_nodes; i++) {
		vertexNESize = factor_count_arr[i]; //-1 to remove unary factor as neigborn
		topologyArraySize += vertexNESize;
		tg->vertex_array[i].length = vertexNESize;


		//@Tere change to get vertex pointer by partition
		tg->vertex_array[i].global_value_ptr = get_vertex_global_pointerByVariedSizePartition(i, numaNode);
	}



	localGraphFile.topologyArraySize = topologyArraySize;
	std::cout<< "\n Total topology_array size:"<< topologyArraySize << std::endl;


	std::cout <<"\n@@start reading topology....";
	read_inputTopology( localGraphFile.num_of_nodes, localGraphFile.topologyArraySize,
			numaNode, tg->vertex_array,localGraphFile,unaryfactors);
	std::cout <<"\n@@end reading topology....";

	ITERATOR_INDEX startTopologyIndex=0;
	ITERATOR_INDEX endTopologyIndex=0;
	ITERATOR_INDEX localTopologySize= getTopologySizeAtNodeId(numaNode,startTopologyIndex,endTopologyIndex);

	tg->topology_array = (union topology_array_element*)numa_alloc_onnode(localTopologySize*sizeof(union topology_array_element),numaNode);
	buildTopologyArray(numaNode, num_nodes, tg->topology_array, tg->factor_tables, tg->vertex_array,
			startTopologyIndex,endTopologyIndex, localTopologySize);
	//display_topology_graph_new(numaNode,localGraphFile.topologyArraySize);

	// free out memory that is not required anymore.
	if(factor_count_arr!=NULL ){
		free(factor_count_arr);
		factor_count_arr = NULL;
	}

	if (unaryfactors!=NULL){
		free(unaryfactors);
		unaryfactors=NULL;
	}
}


/**
 * Read alchemy file in order to
 * build the graph data structures in-memory:
 * Graph Topology fore a given thread.
 * The thread is the creating a copy for
 * specific NumaNode.
 * @PARAMS: Thread Index
 * @RETURN: Null
 */
void *populate_socket_topology_info(void* arg) {
	int threadIndex = *(int*)arg;
	struct bitmask* mask = numa_allocate_nodemask();

	numa_bitmask_setbit(mask, numaNodes[threadIndex]);
	numa_bind(mask);
	numa_set_membind(mask);
	numa_set_strict(numaNodes[threadIndex]);
	numa_run_on_node(numaNodes[threadIndex]);


	//numa allocation of the topology for local socket
	tgMaster[threadIndex] = (struct topology_graph*)numa_alloc_onnode(sizeof(struct topology_graph), numaNodes[threadIndex]);

	//fill the topology data structures.
	if (threadIndex==0)
		read_inputfile(fileName, threadIndex, tgMaster[threadIndex], graphFileMap.num_of_nodes,graphFileMap);
	else
		copyGraphTopology(threadIndex, tgMaster[threadIndex], graphFileMap.num_of_nodes,graphFileMap);
	return(NULL);
}


/*
 * Loads the graph from an alchemy file
 * and create in-memory data numa aware data structures
 *
 * @PARAMS:inpt file name
 * @1= SUCESS, 0=ERROR
 * */


bool graphLoader(std::string fileName){

	struct timeval tv;
	struct timeval start_tv;
	double graph_loadtime = 0;

	pthread_t socket_threads[num_sockets_to_use];

	//Init Global Array of Data copies
	tgMaster = (struct topology_graph**) numa_alloc_onnode(num_sockets_to_use * sizeof(struct topology_graph*), 0);
	//open and map input file.


	std::cout <<"\n\n===== Starting graph Loading ===================\n";
	gettimeofday(&start_tv, NULL);

	//map file
	gettimeofday(&start_tv, NULL);
	loadGraphFile((char*)fileName.c_str(), graphFileMap);
	graph_loadtime = (tv.tv_sec - start_tv.tv_sec) +
	(tv.tv_usec - start_tv.tv_usec) / 1000000.0;

	std::cout <<"\n **TIME Mapping the file: "<<graph_loadtime;


	//Initialize global array only ONCE.
	//Code to create and init global Array Partitions @Tere

	graphFileMap.num_of_nodes =  initialize_global_arrayByPartition(fileName,graphFileMap);

	//read priors
	readPriors(fileName,global_array_partitioned,  num_sockets_to_use);

	gettimeofday(&tv, NULL);


	num_of_nodes = 	graphFileMap.num_of_nodes;
	if(	graphFileMap.num_of_nodes == 0 ){
		std::cout <<"\nError, number of VERTEX = 0 or file does not exists, we can not continue.";
		return false;
	}
	/*load graph topology from file*/

	gettimeofday(&start_tv, NULL);

	populate_socket_topology_info( new int(0));

	unmap_file(graphFileMap.fileDescriptor,graphFileMap.byteSize);
	gettimeofday(&tv, NULL);
	graph_loadtime = (tv.tv_sec - start_tv.tv_sec) +
	 			(tv.tv_usec - start_tv.tv_usec) / 1000000.0;


	//Replicate the topology graph in the respective sockets.
	std::cout << "\nSTART loading, sockets to use: " <<num_sockets_to_use<< std::endl;
	for (int i = 1; i < num_sockets_to_use; i++) {
		if (pthread_create(&socket_threads[i], NULL, populate_socket_topology_info, new int(i))) {
				std::cout << "Cannot create thread " << i << std::endl;
		}
	}
	for (int i = 1; i < num_sockets_to_use; i++) {
		pthread_join(socket_threads[i], NULL);
	}
	std::cout <<"\n **TIME parse and build first topology: " <<graph_loadtime;
	std::cout << "\nEND loading on sockets: " <<num_sockets_to_use<< std::endl;


	/*unload file and release file memory*/
	numa_free(graphFileMap.temp_topology_global,  graphFileMap.topologyArraySize*sizeof(TOPOLOGY_VALUE_TYPE));
	numa_free(graphFileMap.topology_offset_global, graphFileMap.num_of_nodes*sizeof(TOPOLOGY_VALUE_TYPE));

	return true;
}

/**
 * Release the topology in cascade
 * @PARAMS: Nothing
 * @RETURN: Nothing
 */

void releaseTopology(){

	int i=0;
	int factor_table_length = (NUM_OF_UNARY_FACTORS*UNARY_FACTOR_LENGTH) + (NUM_OF_BINARY_FACTORS*BINARY_FACTOR_LENGTH);

	if (tgMaster!=NULL){

		for (i =0; i<num_sockets_to_use; i++){
			if (tgMaster[i]!=NULL){

				if (tgMaster[i]->factor_tables !=NULL){
					numa_free(tgMaster[i]->factor_tables, factor_table_length*sizeof(FACTOR_VALUE_TYPE_INFLATED));
				}

				if (tgMaster[i]->vertex_array != NULL) {
					numa_free(tgMaster[i]->vertex_array, num_of_nodes*sizeof(vertex_info));
				}

				if (tgMaster[i]->topology_array != NULL) {
					numa_free(tgMaster[i]->topology_array, topologyArraySize*sizeof(topology_array_element));
				}
				/*if (tgMaster[i]->topology_array != NULL)
						numa_free(tgMaster[i]->topology_array, num_of_edges*sizeof(union topology_array_element*));*/

				numa_free(tgMaster[i], sizeof(topology_graph));
			}
		}

		numa_free(tgMaster,NUMBER_OF_SOCKETS);
	}
}
/**
 * Release global vertex arrays
 * @PARAM:Nothing
 * @RETURN:Noting
 */
void releaseGlobalVertexArray(){

	if(global_array!=NULL){
		numa_free(global_array, num_of_nodes * sizeof(VERTEX_VALUE_TYPE));
	}

	if (global_array_partitioned != NULL){

		for (int i=0; i<num_sockets_to_use; i++){
			if (global_array_partitioned[i]!=NULL)
				numa_free(global_array_partitioned[i],sizeGlobalArrayDistribution[i]*sizeof(VERTEX_VALUE_TYPE));
		}
		numa_free(global_array_partitioned,num_sockets_to_use*sizeof(VERTEX_VALUE_TYPE));
	}


	if(global_array_copy!=NULL){
		numa_free(global_array_copy, num_of_nodes * sizeof(VERTEX_VALUE_TYPE));
	}

	if (global_array_copy_partitioned != NULL){
			for (int i=0; i<num_sockets_to_use; i++){
				if (global_array_copy_partitioned[i]!=NULL)
					numa_free(global_array_copy_partitioned[i],sizeGlobalArrayDistribution[i]*sizeof(VERTEX_AGG_VALUE_TYPE));
			}
			numa_free(global_array_copy_partitioned,num_sockets_to_use*sizeof(VERTEX_AGG_VALUE_TYPE));
		}

	if (global_array_conv_partitioned != NULL){
				//for (int i=0; i<num_sockets_to_use; i++){
				//	if (global_array_conv_partitioned[i]!=NULL)
						//TOCHECK
						//numa_free(global_array_conv_partitioned[i],sizeGlobalArrayDistribution[i]*sizeof(FLOAT_AGG_VALUE_TYPE));
				//}
				//TOCHECK
				numa_free(global_array_conv_partitioned,num_of_threads*sizeof(VERTEX_AGG_VALUE_TYPE));
			}

	if (sizeGlobalArrayDistribution!=NULL){
		free(sizeGlobalArrayDistribution);
		sizeGlobalArrayDistribution=NULL;
	}

	if (startThreadIndex!=NULL){
		numa_free(startThreadIndex, num_sockets_to_use*sizeof(ITERATOR_INDEX));
		startThreadIndex = NULL;
	}

	if (endThreadIndex!=NULL){
		numa_free(endThreadIndex,num_sockets_to_use*sizeof(ITERATOR_INDEX));
		endThreadIndex = NULL;
	}

	if (numaNodes!=NULL){
			free(numaNodes);
			numaNodes = NULL;
	}
	if (cpuid_counter_array!=NULL){
		free(cpuid_counter_array);
		cpuid_counter_array = NULL;
	}


}



/*Method to get the memory usage for the current execution
 * @Params: by reference
 * 1. vm_usage: virtual memory usage
 * 2. RSS memory (resident memory)
 * */
void process_mem_usage(float& vm_usage, float& resident_set)
{
    vm_usage     = 0.0;
    resident_set = 0.0;

    // the two fields we want
    unsigned long vsize;
    long rss;
    {
        std::string ignore;
        std::ifstream ifs("/proc/self/stat", std::ios_base::in);
        ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
                >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
                >> ignore >> ignore >> vsize >> rss;
    }

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages

    /*convertig to gigabytes*/
    vm_usage = (vsize / 1024.0)/1024/1024;
    resident_set = (rss * page_size_kb)/1024.0/1024;
}



/*****************************************************************
 * Engine Methods
 * Methods to read configurations, run the model, generate output
 * Main launch_gibbs_sampler
 *****************************************************************/

/*
 * Load parameters configuration for the inference:
 * number of threads,number of iterations,number of sockets to use, number of samples, convergence threshold
 *
 *
 * **/
void loadConfigurationParameters(std::string line,std::string &output_file, int line_number){

	bool use_default = false;
	std::vector<std::string> params = split(line, ',');
	std::cout<<"\n User Configurations: "<<line;

	if(params.size() == 4) {
			generate_statistics = false;
	}else if(params.size() == 6) {
		generate_statistics = true;
		stat_samples_interval = atoi(params[4].c_str());
		threshold = atof(params[5].c_str());
	}else {
		cout << " Line number " << line_number << " is invalid: " << line << endl;
		cout << " Using default values for the various parameters!!! " << endl;
		use_default = true;
	}

	if(use_default) {
				num_of_threads = NUMBER_OF_THREADS;
				num_total_iters = NUMBER_OF_ITERS;
				num_threads_per_socket = NUMBER_OF_THREADS_PER_SOCKET;
				topology_array_access = TOPOLOGY_ARRAY_ACCESS_METHOD;
				output_file = "gibbs-samples";
				generate_statistics = false;
				threshold = 0.03;
				stat_samples_interval = 100;
				/*initializing numa nodes*/
				num_sockets_to_use=1;
				numaNodes = (int*)malloc(sizeof(int));
				numaNodes[0] =0;
				cpuid_counter_array = (int*)malloc(sizeof(int));
				cpuid_counter_array[0] = 0;
	}else {
				num_of_threads  = atoi(params[0].c_str());
				num_total_iters = atoi(params[1].c_str());
				output_file     = params[2];

				/*numasockets input*/
				/**
				 * NJ changed code. this in temporary until Tere uses the correct format.
				 */
				num_sockets_to_use=  atoi(params[3].c_str());
				std::cout <<"\n User selected sockets:"<<num_sockets_to_use;
				numaNodes =  (int*)malloc(num_sockets_to_use*sizeof(int));
				cpuid_counter_array = (int*)malloc(num_sockets_to_use*sizeof(int));
				for (int i = 0 ; i <num_sockets_to_use; i++) {
					numaNodes[i] = i;
					cpuid_counter_array[i] = 0;
				}

				/**
				 * NJ added code to consider the method in which the topology array is accessed. Round-robin or partition:
				 * Tere must change it in future to be able to take this as a parameter. Hard coding the value from
				 * #define TOPOLOGY_ARRAY_ACCESS_METHOD for now!
				 */
				topology_array_access = TOPOLOGY_ARRAY_ACCESS_METHOD;
				if(topology_array_access == 1) {
					threads_per_socket = (int*)malloc(num_sockets_to_use*sizeof(int));
					int node_id_index;
					for(int thread_index=0; thread_index<num_of_threads; thread_index++) {
						node_id_index = thread_index % num_sockets_to_use;
						threads_per_socket[node_id_index] = 0;
					}
				}

			}

			if(generate_statistics) {
				num_of_stat_samples   = num_total_iters/stat_samples_interval;
			}
}

/*******************************************************************************
 * Output Methods:
 * Write vertex states, timers, convergence statistcs
 * ******************************************************************************/
/**
 * Write the inference result to file
 *
 */
void  writeGlobalArray(std::string outFileName){

	std::ofstream outResults;
	outResults.open(outFileName.c_str());

	if (outResults.good()){
		for (int i=0; i<num_sockets_to_use; i++){
			for (int k =0; k <sizeGlobalArrayDistribution[i]; k++){
				outResults <<	global_array_partitioned[i][k];
			}
			outResults <<std::endl;
	 }
	}
	outResults.close();
}
/*
 * write convergence results to a fiel
 * */
void writeConvergenceStats(std::string fileName){

	size_t checkSum=0;
	ofstream outStats;
	outStats.open((fileName+".conv").c_str());
	ITERATOR_INDEX totalConvergeStat = 0;

	if (outStats.good()){

		for (int i =0; i <num_of_stat_samples; i++){
			totalConvergeStat=0;
			for (int m=0; m <num_of_threads; m++){
				totalConvergeStat +=global_array_nonConv_partitioned[m][i];
			}
			//outStats<<snapshot_sample_size[i]<<","<<snapshot_timestamp[i]<<","<<convergenceStat[i] <<"\n";
			outStats<<totalConvergeStat <<"\n";
			checkSum += totalConvergeStat;
		}
		//outStats  <<"\n nonConv checksum="<<checkSum<<"\n";
		//std::cout <<"\n nonConv checksum="<<checkSum<<"\n";
	}
	outStats.close();
}
/**
 * Write the state counter distribution, numbe of zeros for each vertex
 * */
void writeInferencesStates(std::string fileName){

	ofstream out_file;
	out_file.open((fileName + ".tsv").c_str());

	int start_index=0;
	int endIndex=0;

	if (out_file.good()){

		size_t checkSum=0;
		for (int m=0; m<num_sockets_to_use; m++){
			endIndex +=sizeGlobalArrayDistribution[m];
			VERTEX_AGG_VALUE_TYPE * g_copy=global_array_copy_partitioned[m];

			for (int j=start_index; j<endIndex; j++){
				out_file <<g_copy[j] << "\n";
				checkSum += g_copy[j];
			}
			start_index+=sizeGlobalArrayDistribution[m];
		}
		//std::cout <<"\nZero Counts checksum:"<<checkSum;
		out_file.close();
	}

}
/*
 * Create file statistics: time and memory
 *
 * */
void writeTimerAndMemoryStatistics(std::string &fileName, timeval &start_tv,timeval &tv, long  graph_loadtime){
	float vm_usage=0;
	float resident_set=0;
	process_mem_usage(vm_usage, resident_set);

	ofstream timer_file;

	timer_file.open((fileName + ".timer").c_str());

	if (timer_file.good()){
	double elapsed = (tv.tv_sec - start_tv.tv_sec) +
			  (tv.tv_usec - start_tv.tv_usec) / 1000000.0;
			timer_file << "Graph Load Time = " << graph_loadtime << endl;
			timer_file << "Gibbs Run Time = " << elapsed << endl;
			timer_file << "VMEM usage = " << vm_usage << "GB"<< endl;
			timer_file << "RSS MEM usage = " << resident_set <<" GB"<< endl;

			std::cout <<"\n\n===== Summary of Execution =====================\n";
			std::cout  << "Graph Load Time = " << graph_loadtime << endl;
			std::cout  << "Gibbs Run Time = " << elapsed << endl;
			std::cout  << "VMEM usage = " << vm_usage << endl;
			std::cout  << "RSS MEM usage = " << resident_set << endl;
	}
	timer_file.close();
}


/*
 * Main to read configuration file and all the flow of the inference
 * Fist read configuration,  execute the graph loader to create
 * graph in-memory, execute the inference and then generate output statistics
 * This method support multiple configurations for the same data load.
 * @PARAMS: configuration file name, graph file input
 * @RETURN: nothing
 * */
void launch_gibbs_sampler(std::string parameters_filename, std::string graphFile) {

		struct timeval tv;
		struct timeval start_tv;
		std::ifstream fin(parameters_filename.c_str());
		std::string line;
		std::string output_file;
		size_t line_number = 1;
		double graph_loadtime = 0;
		bool loadedGraph =false;

		fileName = graphFile;

		/*read header and ignore: header is only use for user info*/
		if (fin.good()){
			getline(fin, line, line_number++);
		}

		while(fin.good() && getline(fin, line, line_number++)) {

			if(trim(line).length() == 0) {
				continue; /*ignore empty line*/
			}

			/*to load inference configuration*/
			loadConfigurationParameters(line,output_file, line_number);


			/*to load the graph input*/

			if (!loadedGraph){

				gettimeofday(&start_tv, NULL);
				loadedGraph = graphLoader(graphFile);
				gettimeofday(&tv, NULL);

				if (loadedGraph== false){
					std::cout <<" \nERROR: Graph loader and builder has failed. Check your input settings";
					return;
				}

				graph_loadtime = (tv.tv_sec - start_tv.tv_sec) +
						(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
				std::cout <<"\n=>Total Loading time:" <<graph_loadtime;
			}

			/* now start Gibbs Sampling.*/

			std::cout <<  "\n\n===== Begin Inference!!! =====================\n"
					  << " # of threads = " << num_of_threads << std::endl
					  << " Threshold = " << threshold << std::endl;

			gettimeofday(&start_tv, NULL);
			launch_sequential_parallel();
			gettimeofday(&tv, NULL);

			std::cout <<"\nEND COMPUTATION\n";
			std::cout <<  "\n\n===== Generating output =====================\n";
			/* execution statistics*/
			writeTimerAndMemoryStatistics(output_file, start_tv,tv, graph_loadtime);
			/*states*/
			writeInferencesStates(output_file);
			/*write out the convergence file*/
			if(generate_statistics) {
				writeConvergenceStats(output_file);
			}

		//	reinitialize_global_array(num_of_nodes);
		}

	fin.close();
	releaseTopology();
    releaseGlobalVertexArray();
}


/**
 * Gibbs Sampling Main Driver
 * @PARMAS: argv[1]= configuration file name
 * 			argv[2]= graph alchemy file name
 * 			argv[3]= update rafreshment rate (number of updates)
 * @RETURN : Exit
 */
int main(int argc, char*argv[]) {
	std::cout << "BEGIN!" << std::endl;


	if(numa_available() < 0) {
		std::cout << "\n This system does not support NUMA" << std::endl;
		std::cout << "\n Please install numalib:  yum install numactl-devel" << std::endl;
		return 1;
	}
	else {
		std::cout << "\n Ok! We have NUMA!!" << std::endl;
	}
	if(argc < 3) {
		std::cout <<"\nError, incorrect input parameters\n"
		 << "\n Usage: <configurationFileName> <inputGraphFile>"
		 << "\n Please input a parameter comma separated file containing: " << endl
		 << " # of threads, # of samples/iterations, output filename, sample interval, convergence_threshold" << std::endl
		 << " Each line in the parameter file must have either the first 3 parameters, or all 5 parameters " << endl;
		return 1;
	}


     if (argc >= 4) {
          std::string a3(argv[3]);
          if (a3.find(std::string("-refresh=")) != std::string::npos) {
                 kRefreshIntervalPercent = atoi((a3.substr(std::string("-refresh=").size())).c_str());
                 if (kRefreshIntervalPercent == 0) {
                     std::cerr << "Invalid kRefreshIntervalPercent" << std::endl;
                     return 1;
                 }

          }
      }
     std::cout <<"\n===== Inference Parameters =====================\n"
    		   <<"\n input filename:         " << argv[2]
     	 	   <<"\n configuration filename: " << argv[1]
     	 	   <<"\n Update frequency:       " << kRefreshIntervalPercent;

	 launch_gibbs_sampler(argv[1],argv[2]);

	 std::cout << "\nDONE!" << std::endl;
	 return EXIT_SUCCESS;
}
