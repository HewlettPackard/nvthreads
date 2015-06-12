#include<iostream>
#include<fstream>
#include<cstring>
#include<string>
#include<stdlib.h>
#include<stdio.h>
// for mmap:
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>

#ifndef GRAPHREADER_H_
#define GRAPHREADER_H_
/**
 * Graph Reader maps an alchemy file to
 * access the different sections of the file: variables, factors, edge factors
 * The Graph Reader keeps an structure of pointers and statistics for the building
 * the graph topology in memory.
 *
 * gomariat, Dec 2014
 * *@ Fei, Nandish, Tere, Krishna,Hideaki
 *
 *
 * Example of ALCHEMY file: Graph of 2 vertices, 1 edge from 0-1
 * The format includes variable list, unary factors for each vertex and factors for each edge.

	variables:
	0
	1
	factors:
	0 // 0.5 0.7
	1 // 0.5 0.8
	0 / 1 // 0.8 0.5 0.9 0.7

 * This reader parse text in mapped in memory.
 *
 */

/************************************
 * DEBUG FLAGS
 * it should be commment out to enable the debug code.
 */
#define DEBUG_BREAKDOWN_IME_ON /*Enables breakdown time*/

const int MAXLINESIZE       = 100;
const int MAX_FACTOR_LENGTH = 2;

#define TOPOLOGY_VALUE_TYPE int64_t
#define ITERATOR_INDEX int64_t
#define VERTEX_VALUE_TYPE char
/*factor types*/
#define  FACTOR_VALUE_TYPE_INFLATED int64_t
#define  FACTOR_VALUE_TYPE_DEFLATED float
#define  FACTOR_VALUE_INFLATION (1LL << 20)


extern inline FACTOR_VALUE_TYPE_DEFLATED deflate_factor_value(FACTOR_VALUE_TYPE_INFLATED inflated);
extern inline FACTOR_VALUE_TYPE_INFLATED inflate_factor_value(FACTOR_VALUE_TYPE_DEFLATED deflated);


extern int numVars;
extern int numEdgeFactors;
extern int topologySizeByVertex;
extern const std::string LABELS;
/**********************************************************************************************
 *Global Variables
 **********************************************************************************************/
struct GraphFileMemoryMap{
	/*map pointers by section: variables,unary factors,edge factors*/
	const char*  memoryStartPtr;
	const char*  memoryEndPtr;
    char*  		 factorsStartPtr;
    char*  		 edgesStartPtr;
	void*  		fileDescriptor;
	size_t 		byteSize;

    /*binary edge list and indices from text edge list*/
	TOPOLOGY_VALUE_TYPE *temp_topology_global;
	TOPOLOGY_VALUE_TYPE *topology_offset_global;

   	/*graph main statistics*/
	size_t  num_of_edges;
	size_t  num_of_nodes;
	size_t  topologyArraySize;
	size_t  lastVertexLength;

}graphFileMap;


/***********************************************************************************************
 * Methods to build the Graph in memory
 ***********************************************************************************************/
bool graphLoader(std::string fileName);


/***********************************************************************************************
 * Methods to read file sections
 ***********************************************************************************************/

inline int loadGraphFile(char *graphFile, GraphFileMemoryMap & graphFilePtr);
//inline int unloadGraphFile(void*f, size_t length);

inline int getNumberOfVertices(GraphFileMemoryMap &graphFilePtr);
inline int getUnaryFactorCounter(GraphFileMemoryMap &graphFilePtrs,  int *factor_count_arr, int num_nodes, FACTOR_VALUE_TYPE_INFLATED *factors);
inline int getEdgeNeigbornsCounter(GraphFileMemoryMap &graphFilePtrs, int *factor_count_arr, int num_nodes);
inline int getNeigborns(GraphFileMemoryMap &graphFilePtrs , TOPOLOGY_VALUE_TYPE *temp_topology,
		const TOPOLOGY_VALUE_TYPE  *topologyOffset, size_t num_nodes, int numNodeId);


int readVerticesOrder(std::string  fileName, int numberOfVertex, VERTEX_VALUE_TYPE *globalArrayIndex);
int readPartitionSizes(std::string fileName, int num_sockets_to_use, ITERATOR_INDEX *sizeGlobalArrayDistribution);

void readPriors(std::string fileName, VERTEX_VALUE_TYPE **global_array_partitioned, int numNodes);
/***********************************************************************************************
 * Methods to map the file in memory.
 ***********************************************************************************************/

const char* read_file(const char* fname, size_t& length);
void        unmap_file(void* f, size_t length);
void 		handle_error(const char* msg);


/***********************************************************************************************
 * Methods to check graph statistics
 ***********************************************************************************************/

inline int checkNE(const TOPOLOGY_VALUE_TYPE  *topologyOffset, size_t num_nodes);
inline int displayArrays(const TOPOLOGY_VALUE_TYPE  *array, size_t size);


inline bool getline(std::ifstream& fin, std::string& line, size_t line_number) {
        return std::getline(fin, line).good();
}


inline std::string trim(const std::string& str) {
        std::string::size_type pos1 = str.find_first_not_of(" \t\r");
        std::string::size_type pos2 = str.find_last_not_of(" \t\r");
        return str.substr(pos1 == std::string::npos ? 0 : pos1, pos2 == std::string::npos ? str.size()-1 : pos2-pos1+1);
}



/**
 *Read the variable section
 *Format of the file:
 * variable:
 * ids
 * factors:
 * Uses \n and "f" to determine where to start and end.
 *@PARAMS: fStart, pointer to the beginning of the memory block
 *@PARAMS: fEnd, pointer to the end of the memory block
 *
 */
inline int getNumberOfVertices(GraphFileMemoryMap &graphFilePtrs){

	int   vertexCounter  	 = 0;
	char* variablesBlock	 = NULL;
	char* variablesBlockEnd  = NULL;

	const char* fStart = (char *) graphFilePtrs.memoryStartPtr;
	const char* fEnd   = (char *) graphFilePtrs.memoryEndPtr;


	struct timeval start_tv;


	gettimeofday(&start_tv, NULL);
	//move to the end of until variables:\n,
	if ((fStart = static_cast<const char*>(memchr(fStart, '\n', fEnd-fStart)))){
		fStart++;

		//move to the end of until "f",
		if ((variablesBlockEnd = (char*)(memchr(fStart, 'f',fEnd-fStart)))){
			variablesBlock = (char *) fStart;

			//counts each \n, assuming each id has a \n at then end
			while (variablesBlock && variablesBlockEnd !=variablesBlock){

					if((variablesBlock=(char*)(memchr(variablesBlock, '\n',variablesBlockEnd-variablesBlock)))) {
						vertexCounter++;
						variablesBlock++;
					}
			}
		}
	}

	graphFilePtrs.factorsStartPtr = variablesBlockEnd;
	graphFilePtrs.num_of_nodes    = vertexCounter;

	#ifdef DEBUG_BREAKDOWN_IME_ON
	struct timeval tv;
	double loadTime = 0;
	gettimeofday(&tv, NULL);
	loadTime = (tv.tv_sec - start_tv.tv_sec) +
	 			(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
	std::cout <<"\n **TIME numberOfVertexCount: "<<loadTime;
	#endif
	return vertexCounter;
}


inline int getUnaryFactorCounter3(GraphFileMemoryMap &graphFilePtrs,  int *factor_count_arr,  int num_nodes){
	char* factorBlockEnd  = NULL;
	char  rawLine[MAXLINESIZE];

	size_t size			  = 0;
	size_t variable		  = 0;


	char* factorBlock = (char *) graphFilePtrs.factorsStartPtr;
	char* fEnd   	  = (char *) graphFilePtrs.memoryEndPtr;


	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);


	std::cout <<"\ns1:"<<factorBlock;
		//move to the end line of "factors"
		if ((factorBlock =  (char*)(memchr(factorBlock, '\n',fEnd-factorBlock)))){
			factorBlock++;
			std::cout <<"\ns2:"<<factorBlock;

			while (factorBlock && fEnd !=factorBlock){

			//look for first "/"
				if ((factorBlockEnd =  (char*)(memchr(factorBlock, '\n',fEnd-factorBlock)))){
					size= factorBlockEnd- factorBlock;
					//std::cout <<"\ns2:"<<factorBlockEnd;
					memcpy(rawLine, factorBlock,size);
					rawLine[size]='\0';

					//parse line
					//	std::cout <<"\nrawline:"<<rawLine;

					factorBlockEnd++;
					//std::cout <<"\ns3:"<<factorBlockEnd;
					factorBlockEnd++;
					factorBlockEnd++;
					factor_count_arr[variable]+=2; //unary + edge  //sum number of vars
					factorBlock= factorBlockEnd;
					}
					else{
						break;
					}
			}
		}else{
			handle_error("\nError parsing factor and neighbors counter.");
		}

	#ifdef DEBUG_TIME_ON
		struct timeval tv;
		double loadTime = 0;
		gettimeofday(&tv, NULL);
		loadTime = (tv.tv_sec - start_tv.tv_sec) +
		 			(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
		std::cout <<"\n **TIME numberOfUnaryFactorCounter: "<<loadTime;
	#endif
	return 1;
}
/**
 * Read neigborns and factor counters
 * @PARAM:
 * @RETURN:
 */
inline int getUnaryFactorCounter(GraphFileMemoryMap &graphFilePtrs,  int *factor_count_arr,  int num_nodes,
		FACTOR_VALUE_TYPE_INFLATED *factors){
	char* factorBlockEnd  = NULL;
	char  rawLine[MAXLINESIZE];

	size_t size			  = 0;
	size_t variable		  = 0;

	char *p=NULL;
	char* factorBlock = (char *) graphFilePtrs.factorsStartPtr;
	char* fEnd   	  = (char *) graphFilePtrs.memoryEndPtr;


	//float *factors=(float*)malloc(num_nodes*2*sizeof(float));
	ITERATOR_INDEX factorIdx=0;
	ITERATOR_INDEX totalVars=0;

	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	//move to the end of the line of "factors:";
	if ((factorBlock =  (char*)(memchr(factorBlock, '\n',fEnd-factorBlock)))){
		factorBlock++;
	}

	while (factorBlock && fEnd !=factorBlock){

		factorBlockEnd =  (char*)memchr(factorBlock, '\n',fEnd-factorBlock);

		if (factorBlockEnd!=NULL){

			size= factorBlockEnd- factorBlock;
			memcpy(rawLine, factorBlock,size);
			rawLine[size]='\0';

			p =strtok(rawLine," ");
			if (p != NULL){
				variable = atoi (p);

				// parse"/"
				p = strtok (NULL, " ");

				if (p == NULL || strcmp(p, "/")==0){
					graphFilePtrs.edgesStartPtr = (factorBlock);
					break;
				}
				
				//std::cout <<"\nv:"<<variable;
				totalVars++;

				//getting factors value
				while ((p = strtok (NULL, " "))!=NULL){
						factors[factorIdx++] =inflate_factor_value(atof(p)); ;
						//	std::cout <<"\nfactor:"<<factors[factorIdx-1];
					}
				}

				factorBlockEnd++;
				factor_count_arr[variable]+=numVars; //unary + edge  //sum number of vars
				factorBlock= factorBlockEnd;
			}else{
					std::cout <<"Error parsing...";
			}
		}


		std::cout <<"\n+Total Factors read: "<<factorIdx;
		std::cout <<"\n+Total Vertices read:"<<totalVars;
	#ifdef DEBUG_TIME_ON
		struct timeval tv;
		double loadTime = 0;
		gettimeofday(&tv, NULL);
		loadTime = (tv.tv_sec - start_tv.tv_sec) +
		 			(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
		std::cout <<"\n **TIME numberOfUnaryFactorCounter: "<<loadTime;
	#endif
	return 1;
}
/**
 * Count edge neighbors for the
 * Return : 1 or 0 success
 */
inline int getEdgeNeigbornsCounter(GraphFileMemoryMap &graphFilePtrs, int* factor_count_arr, int num_nodes){

	char* factorBlockEnd  = NULL;
	char  rawLine[MAXLINESIZE];

	size_t size			  = 0;
	size_t variable		  = 0;

	char* factorBlock = (char *) graphFilePtrs.edgesStartPtr;
	char* fEnd   	  = (char *) graphFilePtrs.memoryEndPtr;
	char *p=NULL;
	ITERATOR_INDEX totalEdges=0;
	struct timeval start_tv;


	gettimeofday(&start_tv, NULL);

		std::cout <<"\n Parsing neighborn counters...";
		//move to the end line of "factors"
		while (factorBlock && fEnd !=factorBlock){

			//look for first "1 / 2 // 0.5 0.6 0.8 0.9/n"
				if ((factorBlockEnd =  (char*)(memchr(factorBlock, '\n',fEnd-factorBlock)))){
					size= factorBlockEnd- factorBlock;

					memcpy(rawLine, factorBlock,size);
					rawLine[size] = '\0';

					if(size<2){
						factorBlock =factorBlockEnd+1;
						continue;
					}
					p =strtok (rawLine, " ");
					//VAR 1
					if (p!=NULL){
						variable      = atol (p);
						//		std::cout <<"\nv:"<<variable;
						factor_count_arr[variable] += topologySizeByVertex;
					}

					// next: /
					p = strtok (NULL, " ");
					//next: //
					p = strtok (NULL, " ");

					//VAR2
					if (p!=NULL){
						variable      = atol (p);
						//std::cout <<"\nv:"<<variable;
						factor_count_arr[variable] += topologySizeByVertex;  //double for the unary factor + edge factor
					}

					totalEdges++;
					factorBlockEnd++;
					factorBlock= factorBlockEnd;
				}
				else{
					break;
				}
			}

		std::cout <<"\n+Total Edges read:"<<totalEdges;
	#ifdef DEBUG_BREAKDOWN_IME_ON
		struct timeval tv;
		double loadTime = 0;
		gettimeofday(&tv, NULL);
		loadTime = (tv.tv_sec - start_tv.tv_sec) +
				(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
				std::cout <<"\n **TIME numberOfEdgeNeigbornFactorCounter: "<<loadTime;
	#endif
	return 1;

}

/*copy factor values to the topology structure using
 * factor table*/
void copyFactors(TOPOLOGY_VALUE_TYPE *temp_toplogy, TOPOLOGY_VALUE_TYPE &vertexId1, TOPOLOGY_VALUE_TYPE &vertexId2,ITERATOR_INDEX offset ){

	std::cout <<"\n v1" <<vertexId1 <<" , v2"<<vertexId2;
	if (vertexId1<vertexId2){
		std::cout <<"\n**it is a host\n";
		temp_toplogy[offset++] = inflate_factor_value(-0.67334);// (0, 0)
		temp_toplogy[offset++] = inflate_factor_value(-0.71334);// (0, 1)
		temp_toplogy[offset++] = inflate_factor_value(-1.38629);// (1, 0)
		temp_toplogy[offset] = inflate_factor_value(-0.28768);// (1, 1)

	}else{
		std::cout <<"\n**it is a domain\n";
		temp_toplogy[offset++] = inflate_factor_value(-0.67334);// (0, 0)
		temp_toplogy[offset++] = inflate_factor_value(-1.38629);// (0, 1)
		temp_toplogy[offset++] = inflate_factor_value(-0.71334);// (1, 0)
		temp_toplogy[offset] = inflate_factor_value(-0.28768);// (1, 1)
	}
}

/*copy factor values to the topology structure using
 * values read from the file.*/
void copyFactors2(TOPOLOGY_VALUE_TYPE *temp_toplogy, TOPOLOGY_VALUE_TYPE &vertexId1, TOPOLOGY_VALUE_TYPE &vertexId2,ITERATOR_INDEX offset, float *edgeFactors  ){
  //	std::cout <<"\n v1" <<vertexId1 <<" , v2"<<vertexId2;

	if (vertexId1<vertexId2){
	  //		std::cout <<"\n**it is a host\n";
		temp_toplogy[offset++] = inflate_factor_value(edgeFactors[0]);// (0, 0)
		temp_toplogy[offset++] = inflate_factor_value(edgeFactors[1]);// (0, 1)
		temp_toplogy[offset++] = inflate_factor_value(edgeFactors[2]);// (1, 0)
		temp_toplogy[offset] = inflate_factor_value(edgeFactors[3]);// (1, 1)

	}else{
	  //	std::cout <<"\n**it is a domain\n";
		temp_toplogy[offset++] = inflate_factor_value(edgeFactors[0]);// (0, 0)
		temp_toplogy[offset++] = inflate_factor_value(edgeFactors[2]);// (0, 1)
		temp_toplogy[offset++] = inflate_factor_value(edgeFactors[1]);// (1, 0)
		temp_toplogy[offset] = inflate_factor_value(edgeFactors[3]);// (1, 1)
	}
}

/**
 * Read neighbors values to construct topology array
 * @PARAM: pointers to the file in memory, temp_topology to map edge list, total number of nodes
 * @RETURN: 1: sucess
 */
inline int getNeigborns(GraphFileMemoryMap &graphFilePtrs , TOPOLOGY_VALUE_TYPE *temp_topology,
		const TOPOLOGY_VALUE_TYPE  *topologyOffset, size_t num_nodes, int numNodeId){

	char* factorBlockEnd  = NULL;
	char  rawLine[MAXLINESIZE]="";
	char* factorBlock = (char *) graphFilePtrs.edgesStartPtr;
	char* fEnd   	  = (char *) graphFilePtrs.memoryEndPtr;
	char* p				   = NULL;

	int neighbornIndex    = 0;

	TOPOLOGY_VALUE_TYPE neighbors[10];
	TOPOLOGY_VALUE_TYPE  vertex		  = 0;
	TOPOLOGY_VALUE_TYPE* start_indexes= NULL;
	TOPOLOGY_VALUE_TYPE totalEdges	  = 0;
	TOPOLOGY_VALUE_TYPE size	          = 0;

	ITERATOR_INDEX i;
	ITERATOR_INDEX j;
	ITERATOR_INDEX offset;
	ITERATOR_INDEX si;


	float *edgeFactors=(float*)malloc(4*sizeof(float));
	start_indexes = (TOPOLOGY_VALUE_TYPE*)malloc(sizeof(TOPOLOGY_VALUE_TYPE)*num_nodes);
	std::memset( start_indexes,0,sizeof(TOPOLOGY_VALUE_TYPE)*num_nodes);


	struct timeval start_tv;

	gettimeofday(&start_tv, NULL);

	//move to the end of until "f" of factor
	neighbornIndex=0;
	while (factorBlock && fEnd !=factorBlock){
			//look for first "/"
			if ((factorBlockEnd =  (char*)(memchr(factorBlock, '\n',fEnd-factorBlock)))){
				size= factorBlockEnd- factorBlock;

						memcpy(rawLine, factorBlock,size);
						if (size<2){
							factorBlock=factorBlockEnd+1;
							continue;
						}
						rawLine[size]='\0';
						//std::cout <<"\nraw:"<< rawLine;
						/* get the first token */


						p =strtok (rawLine, " ");
						//VAR 1
						if (p!=NULL){
						neighbors[neighbornIndex] = atol (p);;
						//std::cout <<"\nv1:"<<neighbors[neighbornIndex];
						neighbornIndex++;
						}
						// /
						p = strtok (NULL, " ");
						p = strtok (NULL, " ");

						//VAR2
						if (p!=NULL){
							neighbors[neighbornIndex] = atol (p);
							//	std::cout <<" v2:"<<neighbors[neighbornIndex];
							neighbornIndex++;
						}

						/* walk through other tokens:factors */
						p = strtok (NULL, " ");
						int factorIdx=0;

						while ((p = strtok (NULL, " "))!=NULL){
							edgeFactors[factorIdx++] = atof(p);
							//	std::cout <<"\nf:"<<edgeFactors[factorIdx-1];
						}



				factorBlockEnd++;


				for  (i =0; i<neighbornIndex; i++){
					vertex = neighbors[i];
					offset = topologyOffset[vertex]; //+2 for the inital unary
					si     = start_indexes[vertex];


					offset = offset+ numVars+(si*topologySizeByVertex);

					for(j=0; j<neighbornIndex; j++) {
												 	// For all the neighbors in this factor, put in the vertex IDs of the neighbors.
						if(i != j){
							si++;
							//factors:
							//	copyFactors(temp_topology, vertex, neighbors[j],offset);

							copyFactors2(temp_topology, vertex, neighbors[j],offset,edgeFactors );
							offset+= numEdgeFactors;
							temp_topology[offset]  =  neighbors[j];


						}
					}
					// the next set of neighbors will be put in from this index (we don't have to traverse through the
					// temp_topology[vertex] array to figure out where to add the information from.
					start_indexes[vertex] = si;
					}

				totalEdges++;
				neighbornIndex = 0; //new row, no neighborn
				factorBlock= factorBlockEnd;
			}
			else{
				//std::cout <<"\n*end of edge factors..";
				break;
			}

		}

	graphFilePtrs.num_of_edges = totalEdges;
	if (start_indexes!=NULL){
		free(start_indexes);
		start_indexes=NULL;
	}

	if (edgeFactors!=NULL){

	}
	#ifdef DEBUG_BREAKDOWN_IME_ON
		struct timeval tv;
		double loadTime = 0;
		gettimeofday(&tv, NULL);
		loadTime = (tv.tv_sec - start_tv.tv_sec) +
		(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
		std::cout <<"\n **TIME getNeighborEdgeList:"<<loadTime;
	#endif
	return 1;
}


/*
 * LoadGraphFile: Open the file and map it in memory.
 * @PARAMS: file name
 * @RETURN: size of the file, if 0 then  error, >0 sucess
 * */
inline int loadGraphFile( char *graphFile, GraphFileMemoryMap & graphFilePtr){

	graphFilePtr.byteSize=0;
	graphFilePtr.memoryStartPtr = read_file(graphFile, graphFilePtr.byteSize);
	graphFilePtr.memoryEndPtr   = graphFilePtr.memoryStartPtr + graphFilePtr.byteSize;
	graphFilePtr.fileDescriptor = (void *) graphFilePtr.memoryStartPtr;
	return graphFilePtr.byteSize;
}
/*
inline int unloadGraphFile(void *fileDescriptor, size_t byteSize){

	return unmap_file(fileDescriptor,byteSize);
}*/

/*Check topology struct*/
inline int checkNE(const TOPOLOGY_VALUE_TYPE  *topologyOffset, size_t num_nodes){

	int64_t offset;
	for (size_t i=0; i <num_nodes; i++){

			offset = topologyOffset[i];
			if (offset <0 || offset>4999999975){
					std::cout <<"\n vetex ="<<i;
					std::cout <<"\n id ="<<i;
					std::cout <<"\n Offset ="<<offset;
					std::cout <<"\n topologyOffset[id] ="<<topologyOffset[i];
				}
		}
	return 0;
}

/*Disply topology struct*/
inline int displayArrays(const TOPOLOGY_VALUE_TYPE  *topology, size_t size ){

	std::cout <<"\n START array dispaly:  ";
	for (size_t i =0; i < size; i++){
		std::cout <<" "<< topology[i];
	}

	return 0;
}

/*Disply topology struct*/
inline int checkSumArrays(VERTEX_VALUE_TYPE  *arr, size_t size,int key ){

	VERTEX_VALUE_TYPE sum=0;
	//std::cout <<"\n START array checksum:  ";
	for (size_t i =0; i < size; i++){
		sum += arr[i];
	}
	std::cout <<"\n ID:"<<key<<" checksum:"<<sum;

	return 0;
}
inline void checkSumTopology(const TOPOLOGY_VALUE_TYPE  *topology, size_t size){

	long sumValue = 0;

	for (size_t i =0; i < size; i++){
		sumValue =  sumValue + topology[i];
	}
	std::cout <<"\n END topology checksum: "<<sumValue;
}

/*Read vertex order
 * File that contains the order of the vertex
 * @PARAMS:
 * @RETUNR:
 * 0-> sucess , 1-> error
 * */
inline int readVerticesOrder(std::string  fileName, int numberOfVertex, ITERATOR_INDEX *globalArrayIndex){


	ITERATOR_INDEX vertexIndex;
	size_t length;
	size_t size;
	char  rawLine[MAXLINESIZE]="";
	ITERATOR_INDEX val=0;

	const char* factorBlock = (char *) read_file(fileName.c_str(), length);
	char* fEnd   	  = (char *) factorBlock +length;
	char* factorBlockEnd  = NULL;


	if (factorBlock==NULL){
		std::cout <<" Error opening file for vertices order";
		return 1;
	}

	std::cout <<"\norder list:";//<<factorBlock;
	vertexIndex = 0;
	while (factorBlock && fEnd !=factorBlock && vertexIndex<numberOfVertex ){
				//look for first "/"
				if ((factorBlockEnd =  (char*)(memchr(factorBlock, '\n',fEnd-factorBlock)))){
						size= factorBlockEnd - factorBlock;
						memcpy(rawLine, factorBlock,size);
						//std::cout<<"\n size:"<<size<<"\t rawline"<<rawLine;

						rawLine[size]='\0';

						val =  strtoul(rawLine, NULL, 10);
						globalArrayIndex[val]=vertexIndex;
					//	std::cout <<"\ni=\t"<<vertexIndex<<" \tval ="<<globalArrayIndex[val]<<" rawline:"<<rawLine;
						vertexIndex++;
						factorBlockEnd++;
						factorBlock=factorBlockEnd;

				}
	}

	std::cout <<"\n +Total vertices read:"<<vertexIndex<<"\n";
	/*for (int i=0; i <numberOfVertex; i++){
		std::cout<<globalArrayIndex[i];
	}*/

	//int ch;
//	std::cin >>ch;
}

void readPriors(std::string fileName, VERTEX_VALUE_TYPE **global_array_partitioned, int numNodes){

	std::ifstream fPriors((fileName+LABELS).c_str());
	std::string line;
	ITERATOR_INDEX vertexId=0;
	VERTEX_VALUE_TYPE priorVal;
	char *p=NULL;
	int nNode=0;


	if (fPriors.good()){
   		//read variables
			while(getline(fPriors, line)) {
				p =strtok (&line[0], " ");


				vertexId =atol (p);
				p= strtok (NULL, "/n");
				priorVal = atol(p);

				for (nNode=0; nNode<numNodes;nNode++ ){
					global_array_partitioned[nNode][vertexId]=priorVal;
				}
			}
	}
	fPriors.close();
}

int readPartitionSizes(std::string fileName, int num_sockets_to_use, ITERATOR_INDEX *sizeGlobalArrayDistribution){

	std::ifstream fin(fileName.c_str());

	 std::string line;
        int line_number=-1;

	while(fin.good() && getline(fin, line, line_number++)){

		line = trim(line);
		assert(line_number<num_sockets_to_use);
		sizeGlobalArrayDistribution[line_number]=atoi(line.c_str());


	}

	fin.close();
    //printf("number of lines %d\n",line_number);
	assert(line_number==num_sockets_to_use);
	return line_number;
}


/*********************************************************************************************
 * File reader using mmap function to speed up reading process
 *********************************************************************************************/
/**
 * Read file data into memory.
 * @PARAMS:
 * Filename
 * lenght to be updated
 * file descriptor
 */
const char* read_file(const char* fname, size_t& length)
{
	const char* addr = NULL;
    int fd 			 = open(fname, O_RDONLY);
    struct 	stat sb;

    if (fd == -1){
    	handle_error("\nError on opening file.\n");
    }

    posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL

    // obtain file size
    if (fstat(fd, &sb) == -1){
    	close (fd);
    	handle_error("\nError on reading file stats");
    }
    length = sb.st_size;


    addr = static_cast<const char*>(mmap(NULL, length, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0u));
    if (addr == MAP_FAILED){
    	close (fd);
    	handle_error("\nError on executing mmap the file.");
    }

    close (fd);
    return addr;
}
/*
 * Unmap memory used for  the file
 * @PARAMS:memory pointer, size
 * @RETURNS: Nothing
 * */
void unmap_file(void* f, size_t length){

	if (munmap(f, length)<0){
		std::cout <<"\nError unmapping data file\n";
		exit (255);
	}
}

void handle_error(const char* msg) {
  std::cout<<msg;
  exit(255);
}




#endif/* GRAPHREADER_H_*/

