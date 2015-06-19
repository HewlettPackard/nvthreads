/*
 * MergeResult.h
 *
 *  Created on: Dec 19, 2013
 *      Author: kimmij
 */

#ifndef MERGERESULT_H_
#define MERGERESULT_H_
#include "SearchResult.h"
#include "LSH.h"
#include <vector>
#include <bitset>
#include <set>
#include <iostream> 
#include <algorithm> 
using namespace std;

class MergeResult {
private:
	//set<pair<int,int> > mergedResult; // set of pair of timeseries id and offset
	//set<int> setResult;
	bitset<LSHGlobalConstants::TOTAL_NUM_TIMESERIES_POINTS> mergedResult; // bitset of timeseries id and offset  ** make passing parameter
	//bitset<1886> tsResult;
public:
	MergeResult();
	virtual ~MergeResult();
	//void mergeResult(pair<int,int>& mergedResult);
	//void mergeResult(vector<pair<int, int> >& hindex);
	//void mergeResult(set<pair<int, int> >& vec_timeseries);
	void mergeResult(const vector<int>& candidates)  {
		//for(int i=0;i<candidates.size();i++) {
		for(int candidate:candidates){
			mergedResult.set(candidate);
	    }
	};
	void setResult(size_t pos) {
		mergedResult.set(pos);
		//cout << pos << "b";
	
	};
	// added by Mijung for setting all bitsets
	void setAll() {
		mergedResult.set();
	};	
	// added by Mijung for setting random bitsets 
	void setRandomSets(int cnt) {
	
		srand(1);
		std::vector<int> randvector;
  		for (int i=1; i<LSHGlobalConstants::TOTAL_NUM_TIMESERIES_POINTS; ++i) randvector.push_back(i);

		std::random_shuffle ( randvector.begin(), randvector.end() );

		for(int i=0;i<cnt;i++) {
			//cout << i << " set id:" << randvector[i] << endl;
			mergedResult.set(randvector[i]);
               	}
		cout << "cnt=" << mergedResult.count() << endl;
	};

	bitset<LSHGlobalConstants::TOTAL_NUM_TIMESERIES_POINTS>& getMergedResult() {
		return mergedResult;
	};
	bool isCandidate(int idx) {
		return mergedResult[idx];
	};
	int getCandidateCount(){
		return mergedResult.count();
	};
	//set<int>& getSetResult();

	//void print(vector<pair<int,int>>& filter);
	void cleanup();
};



#endif /* MERGERESULT_H_ */
