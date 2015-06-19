/*
 * IndexBuilder.h
 *
 *  Created on: Dec 18, 2013
 *      Author: kimmij
 */

#ifndef INDEXBUILDER_H_
#define INDEXBUILDER_H_

#include "HashIndex.h"
#include "headers.h"
#include "LSH.h"

using namespace std;

class IndexBuilder {
private:
        int hashtable_type = LSHGlobalConstants::TWO_ARRAYS_BASED;

public:
	IndexBuilder();
	virtual ~IndexBuilder();

	void setHashTableType(int ht) {
		hashtable_type = ht;
	};

	/*build time series index using a single set of random vectors for all R. This is a new method that enables search with pre-computation of hashes*/
	void buildIndex(int Rindex, pair<int,int> range_L, int querylen);

	/*build time series index using different random vectors for each R: This is the original method.*/
	void buildIndexAllR(int Rindex, pair<int,int> range_L, int querylen);
};

#endif /* INDEXBUILDER_H_ */
