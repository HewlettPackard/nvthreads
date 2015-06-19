/*
 * NaiveSearch.cpp
 *
 *  Created on: Jan 3, 2014
 *      Author: kimmij
 */

#include "NaiveSearch.h"
#include "MergeResult.h"
#include "TimeSeriesBuilder.h"
#include "TimeSeries.h"
#include "LSHManager.h"
#include "SearchResult.h"

#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>


#include <iostream>
#include <fstream>
#include <chrono>
#include <stdio.h>

using namespace std;

// for performance profiling by Jun
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
  int ret;
  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                group_fd, flags);
  return ret;
}


NaiveSearch::NaiveSearch() {
	// TODO Auto-generated constructor stub

}

NaiveSearch::~NaiveSearch() {
	// TODO Auto-generated destructor stub
}

// NaiveSearch (Linear scan)
// input: query point, topk_no
priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& NaiveSearch::_computeTopK_notreduced(
							      vector<RealT> const& query, int topk_no){

	// measure elapsed time
	timespec total_before, total_after;
        clock_gettime(CLOCK_MONOTONIC, &total_before);

	// get TimeSeriesBuilder instance
	TimeSeriesBuilder& tBuilder = LSHManager::getInstance()->getTimeSeriesBuilder();

	// get total data count
	size_t ts_cnt = tBuilder.getCountAllTimeSeries();  

	SearchResult ms; // current data point
	SearchResult maxsofar; // data point with maximum distance in top-k so far
	int  totalNumberOfMultiplications = 0; // total number of multiplication (for statistics)

	// for each point of the input data
	for(int i=0;i<ts_cnt;i++) {

        	RealT *rowP = tBuilder.getCompactTs(i); // get the i-th row of 2-d array of time series
		RealT* vals = rowP; // starting pointer of values of the current data point


		RealT dist1; // maximum distance in top-k so far
		RealT dist2 = 0.0; // initial distance
		bool abandon=false; // true if early abadonment condition is met
		// initial prefetching of 0,16,32 and 48-th elements
		_mm_prefetch((char*)&dist2,_MM_HINT_T0);
		_mm_prefetch((char*)&vals[0],_MM_HINT_T0);
                _mm_prefetch((char*)&vals[16],_MM_HINT_T0);
                _mm_prefetch((char*)&vals[32],_MM_HINT_T0);
                _mm_prefetch((char*)&vals[48],_MM_HINT_T0);

		int sz = query.size(); // query size
			
		// we use 4 for loop strides for loop unrolling/vectorization
		for(int k=0;k<sz-4;k+=4) {
			_mm_prefetch((char*)&vals[k+64],_MM_HINT_T0); // prefetch the 64-th element ahead. Note naive approach uses 64-th element prefetching while LSH uses 80-th element prefetching on which both approaches get maximum gain

			RealT val = 0;
			for(int l=0;l<4;l++) {
			     	val += (vals[k+l]-query[k+l])*(vals[k+l]-query[k+l]);
			}	
			dist2 += val;

			totalNumberOfMultiplications++; // for statistics
 
			// check if early abadonment condition is met
			if(topk.size() == topk_no) {
                       		if(dist1 <= dist2) {
					bool abandon=true;
                                	break;  // kick early abandonment 
                                }
                        }
                }
		// leftover of for loop for loop unrolling/vectorization with 4 strides
		if(abandon == false) {
			for(int k=query.size()-4;k<query.size();k++) {
		        	RealT val = vals[k]-query[k];
				dist2 += val*val;//pow(ts.getValues()[j+k] - query[k], 2);
			}
		}

		// if top-k is full
		if(topk.size() == topk_no) {
			// if the distance < the maximum distance in top-k so far,
			// the existing point of maximum distance is dropped out of top-k and the point of maximum distance is switched
			if(dist1 > dist2) {
				ms.set(i,0,query.size());
				ms.setDistance(dist2);

				topk.pop();
				topk.push(ms);
				maxsofar = topk.top(); 
				dist1 = maxsofar.getDistance();

			}
		} else { // if the top-k queue is not full yet, then push the current point in top-k
			ms.set(i,0,query.size());
			ms.setDistance(dist2);

			topk.push(ms);
			maxsofar = topk.top(); 
			dist1 = maxsofar.getDistance();

		}

	}
	// measure elasped time
        clock_gettime(CLOCK_MONOTONIC, &total_after);
        long total_time = diff(total_before, total_after).tv_nsec;
        cout << "naive_stat=" << total_time << "," <<  totalNumberOfMultiplications << endl;


	return topk;
}


// for top-k LSH search
// input: R index, query point, mergeResult (candidates), topk_no
priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& NaiveSearch::_computeTopK_reduced(
								      int Rindex, vector<RealT> const& query, MergeResult& mergeResult, int topk_no){


	// bitset of merged results instance
	bitset<LSHGlobalConstants::TOTAL_NUM_TIMESERIES_POINTS>& mergedResult = mergeResult.getMergedResult();
	static int query_id = -1; // query_id (for statistics)
	if(Rindex == 0) query_id++;  // query_id incremented for the first R index

	// for measuring elasped time
	timespec total_before, total_after;
	clock_gettime(CLOCK_MONOTONIC, &total_before);
	// measure split time for each task 
#ifdef SPLIT_TIME_MEASURE 
	timespec before, after;
	clock_gettime(CLOCK_MONOTONIC, &before);
#endif
	// get time series builder instance
	TimeSeriesBuilder& tBuilder = LSHManager::getInstance()->getTimeSeriesBuilder();
#ifdef SPLIT_TIME_MEASURE 
	clock_gettime(CLOCK_MONOTONIC, &after);
	long time1 = diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC, &before);
#endif
	// threshold for early abandonment
	RealT threshold[2];
	{
		LSH_HashFunction& lshHash = LSHManager::getInstance()->getLSH_HashFunction();
		n_cand[0] = 0;
		n_cand[1] = 0;
		threshold[0] = lshHash.getR(Rindex)*lshHash.getR(Rindex); // R^2 
		threshold[1] = 1.5*threshold[0];  

	}
#ifdef SPLIT_TIME_MEASURE 
	clock_gettime(CLOCK_MONOTONIC, &after);
	long time2 = diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC, &before);
#endif
#ifdef SPLIT_TIME_MEASURE 
	clock_gettime(CLOCK_MONOTONIC, &after);
	long time3 = diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC, &before);
#endif
	size_t bitset_count = mergedResult.count(); 
#ifdef SPLIT_TIME_MEASURE 
	clock_gettime(CLOCK_MONOTONIC, &after);
	long time4 = diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC, &before);
#endif
	// memory allocation for 2-d candidates 
        int *position_row=(int*) malloc(bitset_count*sizeof(int));
#ifdef SPLIT_TIME_MEASURE 
	clock_gettime(CLOCK_MONOTONIC, &after);
	long time5 = diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC, &before);
#endif
	// memory allocation for 2-d candidates 
        int *position_col=(int*) malloc(bitset_count*sizeof(int));
#ifdef SPLIT_TIME_MEASURE 
	clock_gettime(CLOCK_MONOTONIC, &after);
	long time6 = diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC, &before);
#endif
        int count = 0; //number of time series segment will be involved for scanning.

	// serialize candidates retrieved
#ifdef SERIALIZE_CANDIDATES
	char filename[50];
	//sprintf(filename,"candidates_%f.02_%f.02_%d", query[0], query[1], Rindex);
	sprintf(filename,"./K21L600P7candidates_query990/candidates_%d_%d", query_id, Rindex);
cout << "candfile=" << filename << endl;
cout << "serialize\n";
	FILE * f = fopen(filename,"w");
	int pcnt = 0;
        for(int i=0;i<mergedResult.size();i++) {
                if(mergedResult[i]) {
                        position_row[count]= i/LSHGlobalConstants::NUM_TIMESERIES_POINTS;
                        position_col[count]= i%LSHGlobalConstants::NUM_TIMESERIES_POINTS*LSHGlobalConstants::POS_TIMESERIES;
			fwrite(&position_row[count], 1, sizeof(int), f);
			fwrite(&position_col[count], 1, sizeof(int), f);
			if(pcnt < 10) {
				cout << position_row[count] << "," << position_col[count] << endl;
				pcnt++;
			}
                        count++;
                }
        }
	fclose(f);	
#endif
	// scanning bitset and assign the 2-d candidates from the bitset
#ifndef SERIALIZE_CANDIDATES
        for(int i=0;i<mergedResult.size();i++) {
                if(mergedResult[i]) {
                        position_row[count]= i/LSHGlobalConstants::NUM_TIMESERIES_POINTS;
                        position_col[count]= i%LSHGlobalConstants::NUM_TIMESERIES_POINTS*LSHGlobalConstants::POS_TIMESERIES;
                        count++;
                }
        }
#endif

#ifdef SPLIT_TIME_MEASURE 
	clock_gettime(CLOCK_MONOTONIC, &after);
	long time7 = diff(before, after).tv_nsec;

        //copy only the pointer to the start position of the time series segment to be compared against.
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
       
	SearchResult ms; // current data point
	SearchResult maxsofar; // data point with maximum distance in top-k so far


#ifdef SPLIT_TIME_MEASURE 
	long time10 = 0;
	long time11 = 0; 
#endif
	int  totalNumberOfMultiplications = 0; // total number of multiplication done (for statistics)
	// for each candidate, count: total number of candidates
	for(int i=0;i<count;i++) {
#ifdef SPLIT_TIME_MEASURE 
			clock_gettime(CLOCK_MONOTONIC, &before);
#endif
			RealT dist1;
			RealT dist2 = 0.0;
                        RealT *startPos = cachedTimeSeriesPtr[i]; // starting position of data values for each candidate
			bool abandon=false; // true if early abandonment condition is met
                        RealT m = ( dist1 <  threshold[0]) ? dist1 : threshold[0]; // minimum distance out of R^2 (threshold[0] and maximum distance of top-k, which is for early abadonment for each point
			// prefetch 0,16,32,48 and 64-th elements initially
			// A block of 16 elementis is prefetched 
			// we got this prefetch distance by increasing it until there is no gain 
			_mm_prefetch((char*)&dist2,_MM_HINT_T0);
			_mm_prefetch((char*)&startPos[0],_MM_HINT_T0);
			_mm_prefetch((char*)&startPos[16],_MM_HINT_T0);
			_mm_prefetch((char*)&startPos[32],_MM_HINT_T0);
			_mm_prefetch((char*)&startPos[48],_MM_HINT_T0);
			_mm_prefetch((char*)&startPos[64],_MM_HINT_T0);
			
			// 4 loop strides for loop unrolling/vectorization
                        for(int k=0;k<query.size()-4;k+=4) {
				// prefetch 80-th element ahead
				_mm_prefetch((char*)&startPos[k+80],_MM_HINT_T0);

				// compute distance between time series offset and query
				// inner loop for loop unrolling/vectorization
                                RealT val = 0;
                                for(int l=0;l<4;l++) {
                                        val += (startPos[k+l]-query[k+l])*(startPos[k+l]-query[k+l]);
                                }
                                dist2 += val; 

				totalNumberOfMultiplications++; // for statistics 

				// check if top-k is full
                                if(topk.size() == topk_no) {
					// check if early abandonment condition is met
                                  	if(m <= dist2) // change to have stronger early abandonment condition
					{
                                  		abandon = true;
                                        	break;  
                                  	}
                                }
                        }
			// check if early abadonment is not met
			// then leftover for loop is done (since we are using 4 loop strides)
			if(abandon == false) {
				for(int k=query.size()-4;k<query.size();k++) {
			        	RealT val = startPos[k]-query[k];
					dist2 += val*val;
				}
			}

			
#ifdef SPLIT_TIME_MEASURE 
			clock_gettime(CLOCK_MONOTONIC, &after);
			time10 = time10 + diff(before, after).tv_nsec;
			
			clock_gettime(CLOCK_MONOTONIC, &before);
#endif
                       
                        // if top-k is full and distance >= R^2, we skip this point
                        if(topk.size() == topk_no && dist2 >=threshold[0])
				continue;


			// if the distance < R^2, increment n_cand, which is used for stopping condition for top-k evaluation in the client side 
			if(dist2 < threshold[0])
				n_cand[0]++;
			

			// if top-k is full
			if(topk.size() == topk_no) {
				// if distance < maximum distance in top-k so far,
				// the existing point of maximum distance is dropped out of top-k and the point of maximum distance is switched for it
				if(dist1 > dist2) {
					ms.set(position_row[i],position_col[i],query.size());
					ms.setDistance(dist2);

					topk.pop();
					topk.push(ms);
					maxsofar = topk.top(); 
					dist1 = maxsofar.getDistance();

				}
			} else { // if top-k is not full, any point with any distance will be put into top-k 
                             	ms.set(position_row[i],position_col[i],query.size());
				ms.setDistance(dist2);

				topk.push(ms);
				maxsofar = topk.top(); 
				dist1 = maxsofar.getDistance();

			}
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
        free(position_row);
#ifdef SPLIT_TIME_MEASURE 
	clock_gettime(CLOCK_MONOTONIC, &after);
	long time14 = diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC, &before);
#endif
        free(position_col);
#ifdef SPLIT_TIME_MEASURE 
	clock_gettime(CLOCK_MONOTONIC, &after);
	long time15 = diff(before, after).tv_nsec;

	// for statistics
	printf("**elaspedtime,%d,%d,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n", Rindex, mergeResult.getCandidateCount(),time1,time2,time3,time4,time5,time6,time7,time8,time9,time10,time11,time12,time13,time14,time15);
#endif
	clock_gettime(CLOCK_MONOTONIC, &total_after);
	long total_time = diff(total_before, total_after).tv_nsec;
	//cout << "lsh_stat=" << query_id << "," << Rindex << "," << total_time << "," << count << "," << n_cand[0] << endl;

	// this is for making sure every candiate returned is within R^2
	while(!topk.empty()) {
    		SearchResult res = topk.top();	
		if(res.getDistance() >= threshold[0]) 
			topk.pop();
		else
			break;
	}	
       	return topk;
}

// Common interface for LSH and Naive,  Rindex is -1 for Naive Search
priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& NaiveSearch::computeTopK(int Rindex, vector<RealT> const& query, MergeResult& mergeResult, int topk_no) {

 #ifdef PERF_MONITOR
  struct perf_event_attr pe1, pe2, pe3, pe4;
  long long count1, count2, count3, count4;
  //fd1: LLC read miss; fd2: dTLB miss; fd3: branch miss; fd4: LLC cache access.
  int fd1, fd2, fd3, fd4;

  memset(&pe1, 0, sizeof(struct perf_event_attr));
  memset(&pe2, 0, sizeof(struct perf_event_attr));
  memset(&pe3, 0, sizeof(struct perf_event_attr));
  memset(&pe4, 0, sizeof(struct perf_event_attr));

  ///////////////////////////: LLC miss                                                                                                                            
  pe1.type = PERF_TYPE_HW_CACHE;
  pe1.size = sizeof(struct perf_event_attr);
  pe1.config = (PERF_COUNT_HW_CACHE_LL)|(PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
  pe1.disabled = 1;
  pe1.exclude_kernel = 1;
  pe1.exclude_hv = 1;

  fd1 = perf_event_open(&pe1, 0, -1, -1, 0);
  if (fd1 == -1) {
    fprintf(stderr, "Error opening leader %llx for LLC miss \n", pe1.config);
    exit(EXIT_FAILURE);
  }

  ioctl(fd1, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd1, PERF_EVENT_IOC_ENABLE, 0);


  ///////////////////////////: dTLB miss                                                                                                                           

  pe2.type = PERF_TYPE_HW_CACHE;
  pe2.size = sizeof(struct perf_event_attr);
  pe2.config = (PERF_COUNT_HW_CACHE_DTLB)|(PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
  pe2.disabled = 1;
  pe2.exclude_kernel = 1;
  pe2.exclude_hv = 1;


  fd2 = perf_event_open(&pe2, 0, -1, -1, 0);
  if (fd2 == -1) {
    fprintf(stderr, "Error opening leader %llx for D TLB  miss \n", pe2.config);
    exit(EXIT_FAILURE);
  }

  ioctl(fd2, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd2, PERF_EVENT_IOC_ENABLE, 0);

  ///////////////////////////: branch prediction                                                                                                                   

  pe3.type = PERF_TYPE_HARDWARE;
  pe3.size = sizeof(struct perf_event_attr);
  pe3.config = PERF_COUNT_HW_BRANCH_MISSES;

  pe3.disabled = 1;
  pe3.exclude_kernel = 1;
  pe3.exclude_hv = 1;


  fd3 = perf_event_open(&pe3, 0, -1, -1, 0);
  if (fd3 == -1) {
    fprintf(stderr, "Error opening leader %llx for branch prediction miss\n", pe3.config);
    exit(EXIT_FAILURE);
  }

  ioctl(fd3, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd3, PERF_EVENT_IOC_ENABLE, 0);

  ////////////////////////////: LLC access 
  pe4.type = PERF_TYPE_HW_CACHE;
  pe4.size = sizeof(struct perf_event_attr);
  pe4.config = (PERF_COUNT_HW_CACHE_LL)|(PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
  pe4.disabled = 1;
  pe4.exclude_kernel = 1;
  pe4.exclude_hv = 1;

  fd4 = perf_event_open(&pe4, 0, -1, -1, 0);
  if (fd4 == -1) {
    fprintf(stderr, "Error opening leader %llx for LLC access \n", pe4.config);
    exit(EXIT_FAILURE);
  }

  ioctl(fd4, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd4, PERF_EVENT_IOC_ENABLE, 0);

#endif //PERF_MONITOR 

    //actual work
    priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& result = _computeTopK_reduced(Rindex, query,mergeResult, topk_no);

#ifdef PERF_MONITOR

  ioctl(fd1, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd2, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd3, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd4, PERF_EVENT_IOC_DISABLE, 0);

  read(fd1, &count1, sizeof(long long));
  read(fd2, &count2, sizeof(long long));
  read(fd3, &count3, sizeof(long long));
  read(fd4, &count4, sizeof(long long));

  printf("***hardware,%d, %lld,%lld,%lld,%lld,%6.3f\n", Rindex, count1, count2, count3, count4, (count1*((float)1.0))/count4);

  close(fd1);
  close(fd2);
  close(fd3);
  close(fd4);

  fflush(stdout); //so that we all get the outputed data 

#endif //PERF_MONITOR

  return result;

}

// This is for Naive Search
priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& NaiveSearch::computeTopK(vector<RealT> const& query, int topk_no) {
 
#ifdef PERF_MONITOR

  struct perf_event_attr pe1, pe2, pe3, pe4;
  long long count1, count2, count3,count4;
  //fd1: LLC read miss; fd2: dTLB miss; fd3: branch miss;fd4: LLC access                                                                                                           
  int fd1, fd2, fd3, fd4;

  memset(&pe1, 0, sizeof(struct perf_event_attr));
  memset(&pe2, 0, sizeof(struct perf_event_attr));
  memset(&pe3, 0, sizeof(struct perf_event_attr));
  memset(&pe4, 0, sizeof(struct perf_event_attr));

  ///////////////////////////: LLC miss                                                                                                                            
  pe1.type = PERF_TYPE_HW_CACHE;
  pe1.size = sizeof(struct perf_event_attr);
  pe1.config = (PERF_COUNT_HW_CACHE_LL)|(PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
  pe1.disabled = 1;
  pe1.exclude_kernel = 1;
  pe1.exclude_hv = 1;

  fd1 = perf_event_open(&pe1, 0, -1, -1, 0);
  if (fd1 == -1) {
    fprintf(stderr, "Error opening leader %llx for LLC miss \n", pe1.config);
    exit(EXIT_FAILURE);
  }

  ioctl(fd1, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd1, PERF_EVENT_IOC_ENABLE, 0);


  ///////////////////////////: dTLB miss                                                                                                                           

  pe2.type = PERF_TYPE_HW_CACHE;
  pe2.size = sizeof(struct perf_event_attr);
  pe2.config = (PERF_COUNT_HW_CACHE_DTLB)|(PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
  pe2.disabled = 1;
  pe2.exclude_kernel = 1;
  pe2.exclude_hv = 1;


  fd2 = perf_event_open(&pe2, 0, -1, -1, 0);
  if (fd2 == -1) {
    fprintf(stderr, "Error opening leader %llx for D TLB  miss \n", pe2.config);
    exit(EXIT_FAILURE);
  }

  ioctl(fd2, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd2, PERF_EVENT_IOC_ENABLE, 0);

  ///////////////////////////: branch prediction                                                                                                                   

  pe3.type = PERF_TYPE_HARDWARE;
  pe3.size = sizeof(struct perf_event_attr);
  pe3.config = PERF_COUNT_HW_BRANCH_MISSES;

  pe3.disabled = 1;
  pe3.exclude_kernel = 1;
  pe3.exclude_hv = 1;


  fd3 = perf_event_open(&pe3, 0, -1, -1, 0);
  if (fd3 == -1) {
    fprintf(stderr, "Error opening leader %llx for branch prediction miss\n", pe3.config);
    exit(EXIT_FAILURE);
  }

  ioctl(fd3, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd3, PERF_EVENT_IOC_ENABLE, 0);

  ////////////////////////////: LLC access 
  pe4.type = PERF_TYPE_HW_CACHE;
  pe4.size = sizeof(struct perf_event_attr);
  pe4.config = (PERF_COUNT_HW_CACHE_LL)|(PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
  pe4.disabled = 1;
  pe4.exclude_kernel = 1;
  pe4.exclude_hv = 1;

  fd4 = perf_event_open(&pe4, 0, -1, -1, 0);
  if (fd4 == -1) {
    fprintf(stderr, "Error opening leader %llx for LLC access \n", pe4.config);
    exit(EXIT_FAILURE);
  }

  ioctl(fd4, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd4, PERF_EVENT_IOC_ENABLE, 0);


#endif //PERF_MONITOR
    
    //actual work
    priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& result = _computeTopK_notreduced(query, topk_no);

#ifdef PERF_MONITOR

  ioctl(fd1, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd2, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd3, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd4, PERF_EVENT_IOC_DISABLE, 0);

  read(fd1, &count1, sizeof(long long));
  read(fd2, &count2, sizeof(long long));
  read(fd3, &count3, sizeof(long long));
  read(fd4, &count4, sizeof(long long));

  printf("***hardware,%lld,%lld,%lld,%lld,%6.3f\n", count1, count2, count3, count4,  (count1*((float)1.0))/count4);

  close(fd1);
  close(fd2);
  close(fd3);
  close(fd4);

  fflush(stdout); //so that we all get the outputed data 


#endif //PERF_MONITOR

  return result;
}

void NaiveSearch::cleanup(){
	//printf("Naive Search cleanup topk size=%d\n", topk.size());

}

priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& NaiveSearch::getTopK() {

	return topk;
}

priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& NaiveSearch::mergeTopK(vector<RealT> const& query, NaiveSearch& naiveSearch, int topk_no) {

	priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& tk = naiveSearch.getTopK();

	// merge top-k
	while(tk.size() != 0) {
		SearchResult ms = tk.top();
		tk.pop();

		if(topk.size() < topk_no) {
			topk.push(ms);
		} else {
			SearchResult maxsofar = topk.top();
			RealT dist1 = maxsofar.getDistance();
			RealT dist2 = ms.getDistance();
			if(dist1 < dist2) {
				continue;
			} else {
				topk.pop();
				topk.push(ms);
			}
		}
	}

	return topk;
}


