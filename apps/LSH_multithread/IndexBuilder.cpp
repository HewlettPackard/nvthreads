/*
 * IndexBuilder.cpp
 *
 *  Created on: Dec 18, 2013
 *      Author: kimmij
 */

#include "IndexBuilder.h"
#include "LSHManager.h"
#include "TimeSeriesBuilder.h"
#include "TimeSeries.h"
#include "HashIndex.h"
#include "LSHHashFunction.h"
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include "headers.h"
#include "LSH.h"

//added by Jun Li, to have tcmalloc to release system memory after cleaning up
//#include <google/malloc_extension.h>

LSHManager* LSHManager::m_pInstance = 0;

IndexBuilder::IndexBuilder() {
	// TODO Auto-generated constructor stub

}

IndexBuilder::~IndexBuilder() {
	// TODO Auto-generated destructor stub
}


/**
 * Build time series index using a single set of random vectors for all R. This is a new method that enables search with pre-computation of hashes
 * Adding new method to call getIndexR0 method that enable queryhashes precomputation
 * for perturbations.
 * The rest of the logic is the same than the original method call now buildIndexAllVectors
 * Added by: Tere Gonzalez
 * April 10, 2014
 *
 * NOTE: To back the original method just rename the original method.
 */
void IndexBuilder::buildIndex(int Rindex, pair<int,int> range_L, int querylen) {

	timeval begin,end;
	time_t delta;


	gettimeofday(&begin, nullptr);
	TimeSeriesBuilder& tBuilder = LSHManager::getInstance()->getTimeSeriesBuilder();

	LSH_HashFunction& lshHash = LSHManager::getInstance()->getLSH_HashFunction();

	RealT rValue= lshHash.getR(Rindex);

	gettimeofday(&end, nullptr);
	delta = (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);
	cout << "getTimeSeriesBuilder/LSH_HashFunction " << delta << " microseconds" << endl;

	gettimeofday(&begin, nullptr);
	//map<int,TimeSeries> ts_map = tBuilder.getAllTimeSeries();
	//vector<TimeSeries>& ts_map = tBuilder.getAllTimeSeries();
	int ts_cnt = tBuilder.getCountAllTimeSeries();

	gettimeofday(&end, nullptr);
	delta = (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);
	cout << "getAllTimeSeries " << delta << " microseconds" << endl;

	int cnt=0;
	time_t time1 = 0;
	time_t time2 = 0;
	time_t time3 = 0;
	vector<pair<Uns32T,Uns32T>> hindex;

	int i=0;
	//for(map<int, TimeSeries>::iterator iterator = ts_map.begin(); iterator != ts_map.end(); iterator++) {


	for(int i=0;i<ts_cnt;i++) {
		RealT *vals = tBuilder.getCompactTs(i);
		size_t length = tBuilder.getTsLength(i);
		//TimeSeries &ts = ts_map[i]; //iterator->second;
		//int id = ts.getId();
		//int length = ts.getValues().size();

		int idx=0;

		for(int j=0;j<length-querylen+1;j+=LSHGlobalConstants::POS_TIMESERIES) {
			//printf("ts[%d]=%f\n", j,ts.getValues().at(j));
			//fflush(stdout);
			cnt++;
			gettimeofday(&begin, nullptr);
			hindex.clear();
			time_t t = lshHash.getIndexR0(rValue, range_L, (RealT*)&(vals[j]), querylen, hindex);
			time3 += t;
			gettimeofday(&end, nullptr);
			time1 += (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);
			//printf("hashIndex.put start\n");

			gettimeofday(&begin, nullptr);
			//pair<int,int> p = pair<int,int>(id,j);
			for(int l=0;l<hindex.size();l++) {
				HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(Rindex,range_L.first+l);
				//hashIndex.put(hindex[l].first, hindex[l].second, (pair<int,int> const&)p);
				hashIndex.put(hindex[l].first, hindex[l].second, (LSHGlobalConstants::NUM_TIMESERIES_POINTS)*i+idx); //(pair<int,int> const&)p);

				//printf("hashIndex.put [%d][%d]\n", hindex[l].first, hindex[l].second);

			}
			gettimeofday(&end, nullptr);
			time2 += (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);

			idx++;


		}
	}
	cout << "AllTimeSeries size=" << ts_cnt << endl;
	cout << "getIndex " << time1 << " microseconds" << endl;
	cout << "getIndex pre-processing " << time3 << " microseconds" << endl;
	cout << "hashIndex.put " << time2 << " microseconds" << endl;

	// Added by Mijung for two int array hash table
	if(hashtable_type == LSHGlobalConstants::TWO_ARRAYS_BASED) {

		cout << "creating 2-d array..." << endl;
		for(int l=range_L.first;l<=range_L.second;l++) {
			HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(Rindex,l);
			hashIndex.create2tables();
			hashIndex.cleanup();
		}
		//MallocExtension::instance()->ReleaseFreeMemory();
		cout << "completed 2-d array and trying to free memory back to OS" << endl;
	}


}

/**
 * build time series index using different random vectors for each R: This is the original method.
 * Implemeted by Mijung
 * NOTE: to reuse this method please rename this methods as buildIndex and raname the previous method
 */
void IndexBuilder::buildIndexAllR(int Rindex, pair<int,int> range_L, int querylen) {

	timeval begin,end;
	time_t delta;

	gettimeofday(&begin, nullptr);
	TimeSeriesBuilder& tBuilder = LSHManager::getInstance()->getTimeSeriesBuilder();
	LSH_HashFunction& lshHash = LSHManager::getInstance()->getLSH_HashFunction();

	gettimeofday(&end, nullptr);
	delta = (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);
	cout << "getTimeSeriesBuilder/LSH_HashFunction " << delta << " microseconds" << endl;

	gettimeofday(&begin, nullptr);
	//map<int,TimeSeries> ts_map = tBuilder.getAllTimeSeries();
	//vector<TimeSeries>& ts_map = tBuilder.getAllTimeSeries();
	int ts_cnt = tBuilder.getCountAllTimeSeries();

	gettimeofday(&end, nullptr);
	delta = (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);
	cout << "getAllTimeSeries " << delta << " microseconds" << endl;

	int cnt=0;
	time_t time1 = 0;
	time_t time2 = 0;
	time_t time3 = 0;
	vector<pair<Uns32T,Uns32T>> hindex;

	int i=0;
	//for(map<int, TimeSeries>::iterator iterator = ts_map.begin(); iterator != ts_map.end(); iterator++) {
	for(int i=0;i<ts_cnt;i++) {
 		RealT *vals = tBuilder.getCompactTs(i);
                size_t length = tBuilder.getTsLength(i);

		//TimeSeries &ts = ts_map[i]; //iterator->second;
		//int id = ts.getId();
		//int length = ts.getValues().size();

		int idx=0;

		for(int j=0;j<length-querylen+1;j+=LSHGlobalConstants::POS_TIMESERIES) {
			//printf("ts[%d]=%f\n", j,ts.getValues().at(j));
			//fflush(stdout);
			cnt++;
			gettimeofday(&begin, nullptr);
			hindex.clear();
			time_t t = lshHash.getIndex(Rindex, range_L, (RealT*)&(vals[j]), querylen, hindex);
			time3 += t;
			gettimeofday(&end, nullptr);
			time1 += (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);
			//printf("hashIndex.put start\n");

			gettimeofday(&begin, nullptr);
			//pair<int,int> p = pair<int,int>(id,j);
			for(int l=0;l<hindex.size();l++) {
				HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(Rindex,range_L.first+l);
				//hashIndex.put(hindex[l].first, hindex[l].second, (pair<int,int> const&)p);
				hashIndex.put(hindex[l].first, hindex[l].second, (LSHGlobalConstants::NUM_TIMESERIES_POINTS)*i+idx); //(pair<int,int> const&)p);

				//printf("hashIndex.put [%d][%d]\n", hindex[l].first, hindex[l].second);

			}
			gettimeofday(&end, nullptr);
			time2 += (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);

			idx++;
		}
	}
	cout << "AllTimeSeries size=" << ts_cnt << endl;
	cout << "getIndex " << time1 << " microseconds" << endl;
	cout << "getIndex pre-processing " << time3 << " microseconds" << endl;
	cout << "hashIndex.put " << time2 << " microseconds" << endl;

	//for(int l=range_L.first;l<=range_L.second;l++) {
	//	HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(Rindex,l);
	//	hashIndex.print();
	//}
}
