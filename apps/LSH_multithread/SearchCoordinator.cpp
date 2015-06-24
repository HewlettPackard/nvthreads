/*
 * SearchCoordinator.cpp
 *
 *  Created on: Jan 3, 2014
 *      Author: kimmij
 */

#include "SearchCoordinator.h"
#include "headers.h"
#include "MergeResult.h"

SearchCoordinator::SearchCoordinator(string searchID, vector<RealT> const& query, int topk_no) : m_query(query), m_topk_no(topk_no)
{
	//printf("SearchCoordinator with parameters constructed\n");
}
SearchCoordinator::SearchCoordinator()
{

}
SearchCoordinator::~SearchCoordinator() {
	//printf("SearchCoordinator destroyed\n");
	//fflush(stdout);
}





void SearchCoordinator::cleanup() {
	mergeResult.cleanup();
	naiveSearch.cleanup();
}
