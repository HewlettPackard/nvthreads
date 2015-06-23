/*
 * SearchCoordinator.h
 *
 *  Created on: Jan 3, 2014
 *      Author: kimmij
 */

#ifndef SEARCHCOORDINATOR_H_
#define SEARCHCOORDINATOR_H_

#include <vector>
#include "MergeResult.h"
#include "NaiveSearch.h"
#include "IndexSearch.h"
#include "headers.h"

using namespace std;

class SearchCoordinator {
private:
	IndexSearch indexSearch;
	MergeResult mergeResult;
	NaiveSearch naiveSearch;
	vector<RealT> m_query;
	int m_topk_no;

	/**
	 * Query hashes vector that holds a precomputation of "atimesX" and "atimesDelta" for the hashes computation
	 *it also implement a new method to get the index for the query points
	 *the object exists for the search life cycle
	 */
	QueryHashFunctions queryHashes;


public:
	SearchCoordinator(string searchID, vector<RealT> const& query, int topk_no);
	SearchCoordinator();

	virtual ~SearchCoordinator();

	MergeResult& getMergeResult() {
		return mergeResult;
	};
	IndexSearch& getIndexSearch() {
		return indexSearch;
	};
	NaiveSearch& getNaiveSearch() {
		return naiveSearch;
	};

	vector<RealT>& getQuery() {
		return m_query;
	};

	int getTopkNo() {
		return m_topk_no;
	};

	void cleanup();

	/**
	 * Get the queryHash functions for the query search
	 * the object exists for the search life cycle
	 */
	QueryHashFunctions& getQueryHashFunctions(){
				return queryHashes;
			};

};

#endif /* SEARCHCOORDINATOR_H_ */
