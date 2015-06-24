#include <stdlib.h>
#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>
#include <boost/atomic.hpp>
#include "headers.h"
#include "SelfTuning.h"
#include <iostream>
#include <vector>
#include <string>
#include "TimeSeriesBuilder.h"
#include "LSHManager.h"
#include "LSHHashFunction.h"
#include "IndexSearch.h"
#include "NaiveSearch.h"
#include "MergeResult.h"
#include <fstream>
#include <chrono>
#include <thread>
#include <blaze/Blaze.h>
#include <blaze/util/ThreadPool.h>
#include <pthread.h>
#include <blaze/math/smp/threads/ThreadBackend.h>

/***************************************************************
// LSHtest is for testing c++-based implementation of LSH search
***************************************************************/

// threadpool using blaze library
typedef blaze::ThreadPool< std::thread
                            , std::mutex
                            , std::unique_lock<std::mutex>
                            , std::condition_variable > ThreadPool; 
// it is initialized with one and assign the number of threads input as an argument
ThreadPool blaze_threadpool(1);


// these are atomic functions used for mcs lock
#define atomic_xadd(P, V) __sync_fetch_and_add((P), (V))
#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))
#define atomic_inc(P) __sync_add_and_fetch((P), 1)
#define atomic_dec(P) __sync_add_and_fetch((P), -1) 
#define atomic_add(P, V) __sync_add_and_fetch((P), (V))
#define atomic_set_bit(P, V) __sync_or_and_fetch((P), 1<<(V))
#define atomic_clear_bit(P, V) __sync_and_and_fetch((P), ~(1<<(V)))

/* Compile read-write barrier */
#define barrier() asm volatile("": : :"memory")

/* Pause instruction to prevent excess processor bus usage */ 
#define cpu_relax() asm volatile("pause\n": : :"memory")


// for mcs lock
typedef union rwticket rwticket;

union rwticket
{
	unsigned u;
	unsigned short us;
	__extension__ struct
	{
		unsigned char write;
		unsigned char read;
		unsigned char users;
	} s;
};

static void rwticket_wrlock(rwticket *l)
{
	unsigned me = atomic_xadd(&l->u, (1<<16));
	unsigned char val = me >> 16;
	
	while (val != l->s.write) cpu_relax();
}

static void rwticket_wrunlock(rwticket *l)
{
	rwticket t = *l;
	
	barrier();

	t.s.write++;
	t.s.read++;
	
	*(unsigned short *) l = t.us;
}

static int rwticket_wrtrylock(rwticket *l)
{
	unsigned me = l->s.users;
	unsigned char menew = me + 1;
	unsigned read = l->s.read << 8;
	unsigned cmp = (me << 16) + read + me;
	unsigned cmpnew = (menew << 16) + read + me;

	if (cmpxchg(&l->u, cmp, cmpnew) == cmp) return 0;
	
	return EBUSY;
}

static void rwticket_rdlock(rwticket *l)
{
	unsigned me = atomic_xadd(&l->u, (1<<16));
	unsigned char val = me >> 16;
	
	while (val != l->s.read) cpu_relax();
	l->s.read++;
}

static void rwticket_rdunlock(rwticket *l)
{
	atomic_inc(&l->s.write);
}

static int rwticket_rdtrylock(rwticket *l)
{
	unsigned me = l->s.users;
	unsigned write = l->s.write;
	unsigned char menew = me + 1;
	unsigned cmp = (me << 16) + (me << 8) + write;
	unsigned cmpnew = ((unsigned) menew << 16) + (menew << 8) + write;

	if (cmpxchg(&l->u, cmp, cmpnew) == cmp) return 0;
	
	return EBUSY;
}


using namespace std;

// boost thread
boost::atomic_int threads_finished(0);
boost::atomic_int ts_thread_finished(0);
boost::atomic_int init_index_thread_finished(0);

// priority queue for maintaining topk 
priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult> common_topk;

// dist_maxsofar is the maximum distance of current topk 
atomic<RealT> dist_maxsofar;

// number of perturbation queries
int pert_no;

// number of top-k
int topk_no;

// current R index
int curR;

// bitset size
size_t bitset_size;

// bitset count
atomic<size_t> bitset_count;

// bitset
bitset_type* mergedResult;

// block size for block-based bitset
#define BITSET_BLK_SZ (sizeof(bitset_type)*8) 

// mcs lock
rwticket tl;

// struct to hold data to be passed to a thread 
typedef struct get_cand_data
{
int start_idx; 
int count; 
int n;
} cand_data;
typedef struct str_thdata
{
    LSHManager* pLSH;
    int R_index;
    int low_L; 
    int high_L; 
    int querylength;
} thdata;
/* prototype for thread routine */
// individual index building threads
void * indexbuilding_function ( void *ptr )
{
    thdata *data;            
    data = (thdata *) ptr;  /* type cast to a pointer to thdata */
    printf("Thread %d %d %d running\n", data->R_index, data->low_L, data->high_L);

    IndexBuilder indexBuilder = data->pLSH->getIndexBuilder();
    indexBuilder.buildIndex(data->R_index, make_pair(data->low_L,data->high_L),  data->querylength);
    threads_finished++;
    printf("Thread %d %d %d finished\n", data->R_index, data->low_L, data->high_L);
    
} /* print_message_function ( void *ptr ) */

// input data thread
void load_input(LSHManager* pLSH, string datafile) {
	cout << "Timeseries read thread running" << endl;

	TimeSeriesBuilder& tBuilder = pLSH->getTimeSeriesBuilder();
	tBuilder.deserialize(datafile);
	tBuilder.constructCompactTs();
	ts_thread_finished = 1;
	cout << "Timeseries read thread finished" << tBuilder.getCountAllTimeSeries() << endl;
}

// initialize index thread
void init_index(LSHManager* pLSH, int R_num, int L_num) {

 	int capacity = 700000;
	pLSH->initHashIndex(R_num,L_num,capacity);
	init_index_thread_finished = 1;
	cout << "initHashIndex finished" << endl;
}

// set bitset positions for candidates
int setBitPosition(bitset_type val, int* position_row, int* position_col, int p, int cnt) {
    bitset_type loc;
    size_t pos, i;
    for (loc=1, pos=1; pos<=BITSET_BLK_SZ; loc<<=1, pos++) {
        if (val & loc) {
//printf("setbit for %u=%d\n", val, pos);
		i = p+pos-1;	
        	position_row[cnt]= i/LSHGlobalConstants::NUM_TIMESERIES_POINTS;
                position_col[cnt]= i%LSHGlobalConstants::NUM_TIMESERIES_POINTS*LSHGlobalConstants::POS_TIMESERIES;
		cnt++;
	}
    }
    return cnt;
}
// multithreaded topk computation
void parallel_computeTopK(int Rindex, vector<RealT> const& query, int tid, int ncore){

	int i = tid; // thread id
	int count = 0; // candidate count
	// integer arrays for bitset positions 
	int* position_row = (int*) malloc(bitset_count*sizeof(int));
	int* position_col = (int*) malloc(bitset_count*sizeof(int));

        // store bitset positions of candidates
       	while(true) { 
                if(mergedResult[i] != 0) {
			count = setBitPosition(mergedResult[i], position_row, position_col, i*BITSET_BLK_SZ, count);
                }
		i += ncore;
		if(i >= bitset_size) break;
        }
	//cout << "parallel computeTopK invoked tid=" << tid << endl;
        static int query_id = -1; // query_id (for statistics)
        if(Rindex == 0) query_id++;  // query_id incremented for the first R index

        // for measuring elasped time
        //timespec before, after;
        timespec total_before, total_after;
        clock_gettime(CLOCK_MONOTONIC, &total_before);
        // threshold for early abandonment
        RealT threshold[2];
        {
                LSH_HashFunction& lshHash = LSHManager::getInstance()->getLSH_HashFunction();
                threshold[0] = lshHash.getR(Rindex)*lshHash.getR(Rindex); // R^2
                threshold[1] = 1.5*threshold[0];

        }
        // get time series builder instance
        TimeSeriesBuilder& tBuilder = LSHManager::getInstance()->getTimeSeriesBuilder();
#ifdef SPLIT_TIME_MEASURE
        clock_gettime(CLOCK_MONOTONIC, &before);
#endif
        // memory allocation for values for each candidate
        RealT **cachedTimeSeriesPtr = (RealT**)malloc(count*sizeof(RealT *));
#ifdef SPLIT_TIME_MEASURE
        clock_gettime(CLOCK_MONOTONIC, &after);
        long time8 = diff(before, after).tv_nsec;

        clock_gettime(CLOCK_MONOTONIC, &before);
#endif
        // assign the position and values for each candidate
        for (int i=0;i<count;i++) {
          RealT *rowP = tBuilder.getCompactTs(position_row[i]);
          RealT *sourceAddress =rowP+position_col[i];
          cachedTimeSeriesPtr[i]=sourceAddress;
        }
#ifdef SPLIT_TIME_MEASURE
        clock_gettime(CLOCK_MONOTONIC, &after);
        long time9 = diff(before, after).tv_nsec;
#endif

        SearchResult ms; // current point
        SearchResult maxsofar; // point with maximum distance in top-k so far
#ifdef SPLIT_TIME_MEASURE
        long time10 = 0;
        long time11 = 0;
#endif
        int  totalNumberOfMultiplications = 0; // total number of Eclidean distance computation done (for statistics)
	int ret;
        long time_waitlock = 0;
        // for each candidate, count: total number of candidates
        for(int i=0;i<count;i++) {
#ifdef SPLIT_TIME_MEASURE
                        clock_gettime(CLOCK_MONOTONIC, &before);
#endif
                        RealT cur_dist = 0.0; // distance for current point
                        // naive approach to compute distance between time series offset and query
                        RealT *startPos = cachedTimeSeriesPtr[i]; // starting position for data of each candidate
                        bool abandon=false; // true if early abandonment condition is met
                        //RealT m = ( dist_maxsofar <  threshold[0]) ? dist_maxsofar : threshold[0]; // minimum distance out of R^2 (threshold[0] and maximum distance of top-k, which is for early abadonment for each point
                        //RealT m;
                        // prefetch 0,16,32,48 and 64-th elements initially
                        // Since a block of 16 elementis is prefetched
                        // we keep increasing until there is no gain
                        _mm_prefetch((char*)&cur_dist,_MM_HINT_T0);
                        _mm_prefetch((char*)&startPos[0],_MM_HINT_T0);
                        _mm_prefetch((char*)&startPos[16],_MM_HINT_T0);
                        _mm_prefetch((char*)&startPos[32],_MM_HINT_T0);
                        _mm_prefetch((char*)&startPos[48],_MM_HINT_T0);
                        _mm_prefetch((char*)&startPos[64],_MM_HINT_T0);

                        // 4 loop strides for loop unrolling/vectorization
                        for(int k=0;k<query.size()-4;k+=4) {
                                // prefetch 80-th element ahead
                                _mm_prefetch((char*)&startPos[k+80],_MM_HINT_T0);

                                // inner loop for loop unrolling/vectorization
                                RealT val = 0;
                                for(int l=0;l<4;l++) {
                                        val += (startPos[k+l]-query[k+l])*(startPos[k+l]-query[k+l]);
                                }
                                cur_dist += val;

                                totalNumberOfMultiplications++; // total number of Eclidean distance computation

                                // check if top-k is full
                                if(common_topk.size() == topk_no) {
                                        // check if early abandonment condition is met
                                        if(dist_maxsofar <= cur_dist) {
                                                abandon = true;
                                                break;  // change to have stronger early abandonment condition
                                        }
                                        if(threshold[0] <= cur_dist) {
                                                abandon = true;
                                                break;  // change to have stronger early abandonment condition
                                        }
                                }
                        }
                        // check if early abadonment is not met
                        // then leftover for loop is done (since we are using 4 loop strides)
                        if(abandon == false) {
                                for(int k=query.size()-4;k<query.size();k++) {
                                        RealT val = startPos[k]-query[k];
                                        cur_dist += val*val;
                                }
                        } else {
				continue;
			}


#ifdef SPLIT_TIME_MEASURE
                        clock_gettime(CLOCK_MONOTONIC, &after);
                        time10 = time10 + diff(before, after).tv_nsec;

                        clock_gettime(CLOCK_MONOTONIC, &before);
#endif

                        // if top-k is full and distance >= R^2, we skip this point
			//printf("partial_topk[%d].size=%d, cur_dist[%d][%f]dist_maxsofar[%f]\n", tid, partial_topk[tid].size(), i+start_idx, cur_dist,dist_maxsofar);
                        if(common_topk.size() == topk_no && cur_dist >= threshold[0])
                                continue;

			// if the distance < R^2, increment n_cand, which is used for stopping condition for top-k evaluation
			//printf("rindex,tid,cur_dist,threshold=%d,%d,%f,%f\n", Rindex, tid, cur_dist, threshold[0]);

        //clock_gettime(CLOCK_MONOTONIC, &before);
			rwticket_wrlock(&tl);
        //clock_gettime(CLOCK_MONOTONIC, &after);
        //time_waitlock += diff(before, after).tv_nsec;
                        if(common_topk.size() == topk_no) {
                                // if the distance < the maximum distance in top-k so far,
                                // the existing point of maximum distance is dropped out of top-k and the point of maximum distance is switched
                                if(dist_maxsofar > cur_dist) {
                                        ms.set(position_row[i],position_col[i],query.size());
                                        ms.setDistance(cur_dist);

                                        common_topk.pop();
                                        common_topk.push(ms);
                                        maxsofar = common_topk.top();
                                        dist_maxsofar = maxsofar.getDistance();

                                }
                        } else { // if top-k is not full, any point with any distance will be put into top-k
                                ms.set(position_row[i],position_col[i],query.size());
                                ms.setDistance(cur_dist);
                                common_topk.push(ms);
                                maxsofar = common_topk.top();
                                dist_maxsofar = maxsofar.getDistance();

                        }
			rwticket_wrunlock(&tl);

			//ret = pthread_spin_unlock(&_lock);
			//_mutex.unlock();
#ifdef SPLIT_TIME_MEASURE
                        clock_gettime(CLOCK_MONOTONIC, &after);
                        time11 = time11 + diff(before, after).tv_nsec;
#endif

        }
        //cout << "totalNumberOfMultiplications=" <<  totalNumberOfMultiplications << endl;

        //before return it, free the memory allocated
#ifdef SPLIT_TIME_MEASURE
        clock_gettime(CLOCK_MONOTONIC, &before);
#endif
        free(cachedTimeSeriesPtr);
#ifdef SPLIT_TIME_MEASURE
        clock_gettime(CLOCK_MONOTONIC, &after);
        long time12 = diff(before, after).tv_nsec;

        clock_gettime(CLOCK_MONOTONIC, &before);
#endif
#ifdef SPLIT_TIME_MEASURE
        clock_gettime(CLOCK_MONOTONIC, &after);
        long time13 = diff(before, after).tv_nsec;

        clock_gettime(CLOCK_MONOTONIC, &before);
#endif
        //free(position_row);
#ifdef SPLIT_TIME_MEASURE
        clock_gettime(CLOCK_MONOTONIC, &after);
        long time14 = diff(before, after).tv_nsec;

        clock_gettime(CLOCK_MONOTONIC, &before);
#endif
        //free(position_col);
#ifdef SPLIT_TIME_MEASURE
        clock_gettime(CLOCK_MONOTONIC, &after);
        long time15 = diff(before, after).tv_nsec;

        // for statistics
        //printf("**elaspedtime,%d,%d,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n", Rindex, mergeResult.getCandidateCount(),time1,time2,time3,time4,time5,time6,time7,time8,time9,time10,time11,time12,time13,time14,time15);
#endif
        clock_gettime(CLOCK_MONOTONIC, &total_after);
        long total_time = diff(total_before, total_after).tv_nsec;
        printf("lsh_stat=%d,%d,%d,%d,%d\n", query_id, Rindex ,total_time, count ,totalNumberOfMultiplications);

	free(position_row);
	free(position_col);
}

// for multithreaded index retrieval
void parallel_retIndex(int startL, int lLength, Uns32T* h1, Uns32T* h2) {
	size_t cnt;
        for (int l=0; l< lLength; l++){
                HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(curR,startL+l);
                cnt = hashIndex.set_boolarr(h1[startL+l],h2[startL+l], mergedResult, BITSET_BLK_SZ);
		bitset_count += cnt;
        }
}


time_t parallel_searchIndex(vector<RealT> & query, int R, pair<int,int> const& range_L, QueryHashFunctions &queryHashes, int ncore){

        LSH_HashFunction& lshHash               = LSHManager::getInstance()->getLSH_HashFunction();
        //Getting hash structures for R=0 only
        PRNearNeighborStructT& nnStruct = lshHash.getNnStruct(0);
        DynamicMatrix<RealT>& a = lshHash.getMat(0);
        PUHashStructureT & uHash                = lshHash.getPUHash(0);

        vector <RealT> atimesByRW;

        /*Parameters for the hashes*/
        int kLength             = nnStruct->hfTuplesLength;
        int W                   = nnStruct->parameterW;
        RealT rValue    = lshHash.getR(R);
        int lLength             = range_L.second-range_L.first+1;

        //h1 and h2 results values
        Uns32T                  h1[lLength];
        Uns32T                  h2[lLength];


        time_t htime = 0;
        time_t htime1= 0;
        time_t htime2= 0;
        time_t htime3= 0;
        struct timespec before, after;


        clock_gettime(CLOCK_MONOTONIC,&before);
        //if R==0, then we compute the vectors, otherwise reuse the vectos
        if( R==0 ) {
                // vector a has been added for matrix multiplication for Hash generation
                if (!queryHashes.computeVectors(query,  kLength, lLength,pert_no,nnStruct->lshFunctions, a)){
                        cout <<endl<<"error creating vectors";
                        return 0; //Error computing vectors, we can not continue..
                        }
        }

        clock_gettime(CLOCK_MONOTONIC,&after);
        htime1 =(diff(before, after).tv_nsec)/1000;

        //get the index for the query
        queryHashes.getIndex(W,rValue,h1,h2,nnStruct->lshFunctions,uHash);

        // multithreaded index retrieval
	curR = R;
	int startL = 0;
        int L_count = ceil((float)lLength/(float)ncore);
        for(int n=0;n<1;n++) {
               if(startL+L_count > lLength) {
                        L_count = lLength - startL;
               }
               blaze_threadpool.schedule(parallel_retIndex, startL,L_count, h1,h2);
               startL += L_count;
               if(startL > lLength) break;
        }
        blaze_threadpool.wait();

        htime2=0;

        //get the index for each perturbation
        for ( int n = 0; n < pert_no; n++)  {
                //get the index for the delta : query perturbation
                clock_gettime(CLOCK_MONOTONIC,&before);
                queryHashes.getIndex(W,rValue,h1,h2,nnStruct->lshFunctions,uHash,n);
                clock_gettime(CLOCK_MONOTONIC,&after);
                htime2+=((diff(before, after).tv_nsec)/1000);

                for (int l=0; l< lLength; l++){
                                HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(R,range_L.first+l);
                                bitset_count += hashIndex.set_boolarr(h1[l],h2[l], mergedResult, BITSET_BLK_SZ);
                }
        }



        htime = htime2 +htime1;
        //cout<<endl<<"vectors total time1:"<<htime1;
        //cout<<endl<<"h1h2 total time:"<<htime2;

        return htime;

}

// stand-alone LSH test 
void testLSHManager(int R_num, int L_num, int topk_no, RealT* rVals, string datafile, string queryfile, int ncore) {

	// create LSHManager
	LSHManager* pLSH = LSHManager::getInstance();

	//int R_num = 4;
	//int L_num = 200;
	//int topk_no = 5;
	//int pert_no = 2;
	//RealT rVals[4] = {1.2,2.04,3.4680,9.7365};

	// create TimeSeriesBuilder to populate time series data
	boost::thread ts_thread(load_input, pLSH, datafile); 
	while(ts_thread_finished == 0) {
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
	ts_thread.join();

	// create hash index
	boost::thread init_index_thread(init_index, pLSH, R_num, L_num); 
	while(init_index_thread_finished == 0) {
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
	init_index_thread.join();

	// create sample LSH hash functions
	// we hard-code each LSH parameter since c++ can not use the LSH Hash file used in Java
	int dimension = 384;
	int useUfunctions = 0;//1;
	int paramK = 21; //22; // parameter K of the algorithm.
	int paramM = 22;//6;
	int paramL = 800; //12; // parameter L of the algorithm.
	RealT paramW = 3.0; //370.2669449999992; //4.000000000; // parameter W of the algorithm.
	int paramT = 4000000; // parameter T of the algorithm.
	int nPoints = 4000000;

/* LSH parameters for the face data set
        int dimension = 5800;
        int useUfunctions = 0;//1;
        int paramK = 17; //22; // parameter K of the algorithm.
        int paramM = 22;//6;
        int paramL = 230; //12; // parameter L of the algorithm.
        RealT paramW = 3.0; //370.2669449999992; //4.000000000; // parameter W of the algorithm.
        int paramT = 556975; // parameter T of the algorithm.
        int nPoints = 556975;
*/


 	LSH_HashFunction& lsh_hash = pLSH->getLSH_HashFunction();
        lsh_hash.initLSHHashFunctions(R_num);
	cout << "initLSHHashFunction done" << endl;

	timespec before, after;
	timespec before_scan, after_scan;
	timespec before_schedule, after_schedule;
	timespec before_thread, after_thread;
	timespec before_merge, after_merge;
	timespec before_search, after_search;
	timespec before_final_topk, after_final_topk;
#ifndef DESERIALIZE_CANDIDATES
	boost::thread_group threadpool;
	int numThreads = 10;
	int numLforOneThread = 1;
	boost::shared_ptr< boost::asio::io_service > ioservice( 
		new boost::asio::io_service
		);
	//work object
	boost::shared_ptr< boost::asio::io_service::work > work(
			new boost::asio::io_service::work( *ioservice )
		);

	// multithreaded index building
	cout << "buildIndex  start" << endl;
       	clock_gettime(CLOCK_MONOTONIC, &before);
	for(int i=0;i<numThreads;i++) {
		threadpool.create_thread(boost::bind(&boost::asio::io_service::run, ioservice));
	}
	int numTasks;
	if(L_num%numLforOneThread == 0) 
		numTasks = L_num/numLforOneThread;
	else
		numTasks = L_num/numLforOneThread+1;
        thdata data[R_num][numTasks];
	for(int i=0;i<R_num;i++) { 
		int low = 0;
		int high = numLforOneThread-1;
		int j=0;
		while(true) {
			if(high > L_num-1) {
				high = L_num-1;
			}
                	data[i][j].pLSH = pLSH;
                	data[i][j].R_index = i;
                	data[i][j].low_L = low;
                	data[i][j].high_L = high;
			data[i][j].querylength = dimension;
			low = low+numLforOneThread;
			high = high+numLforOneThread;
			if(low > L_num-1) {
				break;
			}
			j++;
		}
		
	}
	for(int i=0;i<R_num;i++) { 
		RealT paramR = rVals[i]; 
		RealT paramR2 = paramR*paramR; 
		lsh_hash.init(i, paramR, paramR2, useUfunctions, paramK, paramL, paramM, paramT, dimension, paramW, nPoints);
		for(int k=0;k<numTasks;k++) {
			ioservice->post(boost::bind(&indexbuilding_function, &data[i][k]));

		}
	}
	while(threads_finished < numTasks*R_num) {
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
	work.reset();
	ioservice->stop();
	threadpool.join_all();

       	clock_gettime(CLOCK_MONOTONIC, &after);
	timespec diff_time = diff(before, after);
	long build_time = diff_time.tv_sec*1000 + diff_time.tv_nsec/1000000;
	cout << "buildIndex  done" << endl;
        cout << "buildIndex_time(ms)=" << build_time << endl;
#else
	cout << "Skipped to build index" << endl;
	for(int i=0;i<R_num;i++) { 
		RealT paramR = rVals[i]; 
		RealT paramR2 = paramR*paramR; 
		lsh_hash.init(i, paramR, paramR2, useUfunctions, paramK, paramL, paramM, paramT, dimension, paramW, nPoints);
	}
#endif

	// set the number of threads to thread pool
	blaze::setNumThreads( ncore );
	blaze_threadpool.resize( ncore );

	bitset_size =  ceil(float(LSHGlobalConstants::TOTAL_NUM_TIMESERIES_POINTS)/float(BITSET_BLK_SZ)); 
	mergedResult = new bitset_type[bitset_size];
	cout << "bitset_size=" << bitset_size << endl;

	LSH_HashFunction& lshHash = pLSH->getLSH_HashFunction();

	SearchCoordinatorTracker& tracker = pLSH->getSearchCoordinatorTracker();
	string sId("search");

	int pshared;
	int ret;


	std::ifstream infile(queryfile);
	int query_id = 0;

	// search for each query point
	for (std::string line; std::getline(infile, line); ) {

		vector<RealT> query;

		// get query points
		char* str = (char*)line.c_str();
		char* pch = strtok (str," ");
		cout << "\n\n*************query " << query_id++ << endl;
		while (pch != NULL)
		{
			query.push_back(atof(pch));
			pch = strtok (NULL, " ");
		}

        	tracker.addSearchCoordinator(sId,query,topk_no);
        	SearchCoordinator& searchCoord = tracker.getSearchCoordinator(sId);
        	MergeResult& mergeResult = searchCoord.getMergeResult();
        	IndexSearch& indexSearch = searchCoord.getIndexSearch();
        	QueryHashFunctions &queryHashes = searchCoord.getQueryHashFunctions();
        	NaiveSearch& naiveSearch = searchCoord.getNaiveSearch();
        	priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult> topK;
        	vector<SearchResult> vec_result;

		// for time measures 
        	clock_gettime(CLOCK_MONOTONIC, &before);
		float schedule_time = 0.0;
		float thread_time = 0.0;
		float merge_time = 0.0;
		float scan_time = 0.0;
		float index_search_time = 0.0;
		float final_topk_time = 0.0;

		for(int i=0;i<R_num;i++) { 

		clock_gettime(CLOCK_MONOTONIC, &before_search);
		memset(mergedResult, 0, bitset_size*sizeof(bitset_type));
		bitset_count = 0;
		parallel_searchIndex(query, i, make_pair(0,L_num-1), queryHashes, ncore);  // we should have range of L values
		clock_gettime(CLOCK_MONOTONIC, &after_search);
		diff_time = diff(before_search, after_search);
		index_search_time = (float)diff_time.tv_sec*1000 + (float)diff_time.tv_nsec/1000000;
	
		clock_gettime(CLOCK_MONOTONIC, &before_scan);
		cout << "bitset_count = " << bitset_count << endl;
		clock_gettime(CLOCK_MONOTONIC, &after_scan);
		diff_time = diff(before_scan, after_scan);
		scan_time = (float)diff_time.tv_sec*1000 + (float)diff_time.tv_nsec/1000000;
		clock_gettime(CLOCK_MONOTONIC, &before_schedule);

                for(int n=0;n<ncore;n++) {
			blaze_threadpool.schedule(parallel_computeTopK, i, query, n, ncore);
		}
		clock_gettime(CLOCK_MONOTONIC, &after_schedule);
		diff_time = diff(before_schedule, after_schedule);
		schedule_time += (float)diff_time.tv_sec*1000 + (float)diff_time.tv_nsec/1000000;
		clock_gettime(CLOCK_MONOTONIC, &before_thread);
		blaze_threadpool.wait();
		clock_gettime(CLOCK_MONOTONIC, &after_thread);
		diff_time = diff(before_thread, after_thread);
		thread_time = (float)diff_time.tv_sec*1000 + (float)diff_time.tv_nsec/1000000;
		clock_gettime(CLOCK_MONOTONIC, &before_merge);
		RealT threshold = lshHash.getR(i)*lshHash.getR(i); 
        	while(common_topk.size() != 0) {
               		SearchResult searchResult = common_topk.top();
			if(searchResult.getDistance() >= threshold) {
                               	common_topk.pop();
				continue;
			}
       
			bool bfind = false;
			for(int j=0;j<vec_result.size();j++) {
				SearchResult mt = vec_result[j];
         			if(mt.getId() == searchResult.getId() &&
              		   	mt.getOffset() == searchResult.getOffset()) {
					bfind = true;
					break;
				}
			}
			common_topk.pop();
			if(bfind) continue;
			vec_result.push_back(searchResult);
		}
		clock_gettime(CLOCK_MONOTONIC, &after_merge);
		diff_time = diff(before_merge, after_merge);
		merge_time = (float)diff_time.tv_sec*1000 + (float)diff_time.tv_nsec/1000000;
		tracker.cleanup(sId);
         	if(vec_result.size() >= topk_no)
			break;
		}

		clock_gettime(CLOCK_MONOTONIC, &before_final_topk);
		for(int i=0;i<vec_result.size();i++) {
			SearchResult searchResult = vec_result[i]; 
			if(topK.size() < topk_no) {
				topK.push(searchResult);
			}
			else {
				SearchResult mt = topK.top();
		 		if(mt.getDistance() > searchResult.getDistance()) {
		   			topK.pop();
			 		topK.push(searchResult);
		    		}
			}
			clock_gettime(CLOCK_MONOTONIC, &after_final_topk);
			diff_time = diff(before_final_topk, after_final_topk);
			final_topk_time = (float)diff_time.tv_sec*1000 + (float)diff_time.tv_nsec/1000000;  
		}

       		clock_gettime(CLOCK_MONOTONIC, &after);

		while(topK.size() > 0) {
	
               		SearchResult searchResult = topK.top();
               		int id = searchResult.getId();
               		int offset = searchResult.getOffset();
               		float distance = searchResult.getDistance();

			cout << "id=" << id << ",offset=" << offset << ",distance=" << distance << endl;

               		topK.pop();
		}
		diff_time = diff(before, after);
		float search_time = (float)diff_time.tv_sec*1000 + (float)diff_time.tv_nsec/1000000;
        	cout << "search_time(ms)=" << search_time << endl;
        	cout << "search_index_time(ms)=" << index_search_time << endl;
        	cout << "scan_time(ms)=" << scan_time << endl;
        	cout << "schedule_time(ms)=" << schedule_time << endl;
        	cout << "thread_time(ms)=" << thread_time << endl;
        	cout << "merge_time(ms)=" << merge_time << endl;
        	cout << "final topk time(ms)=" << final_topk_time << endl;
		tracker.removeSearchCoordinator(sId);
	}
}

int main(int nargs, char **args){

	if(nargs < 9) {
		printf("LSHtest <R_num> <L_num> <topk_no> <pert_no> <R values, e.g.: 2.23,4.52,5.67> <datafile> <queryfile> <ncore>\n");
	} else {
		int R_num = atoi(args[1]);
		cout << "R_num=" << R_num << endl;
		int L_num = atoi(args[2]);
		cout << "L_num=" << L_num << endl;
		topk_no = atoi(args[3]);
		cout << "topk_no=" << topk_no << endl;
		pert_no = atoi(args[4]);
		cout << "pert_no=" << pert_no << endl;
		RealT rvals[R_num];
        	char* pch = strtok (args[5],",");
		int i=0;
		while (pch != NULL)
        	{
                	rvals[i] = atof(pch);
		cout << "rvals[" << i << "]=" << rvals[i] << endl;
                	pch = strtok (NULL, ",");
			i++;
        	}
		string datafile = string(args[6]);
		cout << "datafile=" << datafile << endl;
		string queryfile = string(args[7]);
		cout << "queryfile=" << queryfile << endl;
		int ncore = atoi(args[8]);
		cout << "ncore=" << ncore << endl;
		testLSHManager(R_num, L_num, topk_no, rvals, datafile, queryfile,ncore);
	}
        return 0;


}

