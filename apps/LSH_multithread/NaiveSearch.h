/*
 * NaiveSearch.h
 *
 *  Created on: Jan 3, 2014
 *      Author: kimmij
 */

#ifndef NAIVESEARCH_H_
#define NAIVESEARCH_H_

#include <vector>
#include <set>
#include <queue>
#include "SearchResult.h"
#include "MergeResult.h"

using namespace std;

class NaiveSearch {
private:
	priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult> topk;
	int n_cand[2]; // statistics for numbers of candidates < R^2, 1.5*R^2
public:
	NaiveSearch();
	virtual ~NaiveSearch();

	// query: query point, range: range of candidates, mergeResult: reference of MergeResult from which candidates are retrieved
	priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& computeTopK(int Rindex, vector<RealT> const& query, MergeResult& mergeResult, int topk_no);

	// naive search
	priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& computeTopK(vector<RealT> const& query, int topk_no);


	priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& mergeTopK(vector<RealT> const& query, NaiveSearch& naiveSearch, int topk_no); // final topK
	priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& getTopK();

	int* getCandCntWithinThreshold() {
		return n_cand;
	};
	void cleanup();

private:
        //actual implementation for top-k LSH search
	priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& _computeTopK_reduced(
                                     int Rindex, vector<RealT> const& query, MergeResult& mergeResult, int topk_no);

	priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult>& _computeTopK_notreduced(
                                      vector<RealT> const& query, int topk_no);
};


#endif /* NAIVESEARCH_H_ */
