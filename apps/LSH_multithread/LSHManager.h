/*
 * LSHManager.h
 *
 *  Created on: Jan 3, 2014
 *      Author: kimmij
 */

#ifndef LSHMANAGER_H_
#define LSHMANAGER_H_

#include <vector>
#include <memory>
#include "HashIndex.h"
#include "IndexBuilder.h"
#include "TimeSeriesBuilder.h"
#include "LSHHashFunction.h"
#include "SearchCoordinatorTracker.h"
#include "LSH.h"

using namespace std;


class LSHManager {
private:
	static LSHManager* m_pInstance;
	int num_R;
	int num_L;

	int size_hashIndex;
	vector<vector<HashIndex> > vecHashIndex;  // HashIndex for each R and L
	SearchCoordinatorTracker searchCoordinatorTracker;

	IndexBuilder indexBuilder;
	TimeSeriesBuilder timeSeriesBuilder;
	LSH_HashFunction lsh_hashFunction;

	LSHPerformanceCounter performCounter;

//for time series memory compaction.
private: 
        bool compacted; 
public:
	LSHManager();
	virtual ~LSHManager();
	static LSHManager* getInstance() {
		if(!m_pInstance) {
			m_pInstance = new LSHManager();
		}
		return m_pInstance;
	};

	void initHashIndex(int R_num, int L_num, int capacity);
	void setNumR(int numR) {
		num_R = numR;
	};
	void setNumL(int numL) {
		num_L = numL;
	};

	SearchCoordinatorTracker& getSearchCoordinatorTracker() {
		return searchCoordinatorTracker;
	};

	HashIndex& getHashIndex(int Rindex, int Lindex) {
		return vecHashIndex[Rindex][Lindex];
	};

	void setHashIndex(int Rindex, int Lindex, HashIndex hashIndex) {
		vecHashIndex[Rindex][Lindex] = hashIndex;
	};

	IndexBuilder& getIndexBuilder() {
		return indexBuilder;
	};
	TimeSeriesBuilder& getTimeSeriesBuilder()
	{
		return timeSeriesBuilder;
	};
	LSH_HashFunction& getLSH_HashFunction() {
		return lsh_hashFunction;
	};
	void serialize();
	void deserialize();

	LSHPerformanceCounter& getLSHPerformanceCounter () {
	     return performCounter;
	};

public: 
	//Jun Li: I do not want to have the JNI call to come to this C++ call directly. Instead, I am using "initiateSearchCoordinatorAtR" JNI
	//call to do the work, which then should not be multithreaded environment 
	void constructCompactTs();
};


#endif /* LSHMANAGER_H_ */
