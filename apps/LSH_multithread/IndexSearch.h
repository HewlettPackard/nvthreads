/*
 * IndexSearch.h
 *
 *  Created on: Dec 18, 2013
 *      Author: kimmij
 */

#ifndef INDEXSEARCH_H_
#define INDEXSEARCH_H_
#include <vector>
#include <map>
#include "MergeResult.h"


using namespace std;

class IndexSearch {

public:
	IndexSearch();
	virtual ~IndexSearch();

	/*search  method that computes query hashes for all the R iterations: This is the original method*/
	time_t searchIndex(vector<RealT> & query, int R, pair<int,int> const& range_L, int pert_no, MergeResult& mergeResult);

	/*search method that computes query hashes using percomputed vectos and using a single set of random numbers*/
	time_t searchIndex(vector<RealT> & query, int R, pair<int,int> const& range_L, int pert_no, MergeResult& mergeResult,QueryHashFunctions &queryHashes);

	/*search method for testing that emulate original method, but it uses precomputation vectos for all Rs.*/
	time_t searchIndexPrecomputationAllR(vector<RealT> & query, int R, pair<int,int> const& range_L, int pert_no, MergeResult& mergeResult,QueryHashFunctions &queryHashes);

	/**
 	 * This is for the search simulation with random bitsets
 	 * added by Mijung
 	 */
	void searchRandomIndex(MergeResult& mergeResult, int cnt) {
		if(cnt == 0) {
			return;
		} 
		if(cnt == LSHGlobalConstants::TOTAL_NUM_TIMESERIES_POINTS) {
			mergeResult.setAll();
		} else {
        		mergeResult.setRandomSets(cnt);
		}
	};

};

#endif /* INDEXSEARCH_H_ */
