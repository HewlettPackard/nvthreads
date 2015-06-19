/*
 * MergeResult.cpp
 *
 *  Created on: Dec 19, 2013
 *      Author: kimmij
 */

#include "MergeResult.h"
#include <vector>
#include <algorithm>

MergeResult::MergeResult() {
	// TODO Auto-generated constructor stub

}

MergeResult::~MergeResult() {
	// TODO Auto-generated destructor stub
}
/*
void MergeResult::mergeResult(pair<int, int>& timeseries) {

	mergedResult.insert(timeseries);
}
void MergeResult::mergeResult(vector<pair<int, int> >& vec_timeseries) {

	std::copy( vec_timeseries.begin(), vec_timeseries.end(), std::inserter( mergedResult, mergedResult.end() ) );

}
*/
/*
void MergeResult::mergeResult(set<pair<int, int> >& vec_timeseries) {

	std::copy( vec_timeseries.begin(), vec_timeseries.end(), std::inserter( mergedResult, mergedResult.end() ) );

}
*/




//void MergeResult::mergeResult(bitset<688390>& bs) {
//}



/*
void MergeResult::print(vector<pair<int,int>>& filter) {

	for (set<pair<int,int> >::iterator it=mergedResult.begin(); it!=mergedResult.end(); ++it) {

		pair<int, int> val = *it;  // id and offset
		for(int i=0;i<filter.size();i++) {
			pair<int,int> p = filter.at(i);
			if(p.first == val.first && p.second == val.second) {
				printf("MergeResult id=[%d], offset=[%d]\n", val.first, val.second);
				break;
			}
		}
	}
	fflush(stdout);
}
*/
void MergeResult::cleanup(){

	mergedResult.reset();
	//tsResult.reset();

	//setResult.clear();
	//printf("MergeResult cleanup size=%d\n",mergedResult.size());
}


