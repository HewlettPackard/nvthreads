/*
 * SearchResult.cpp
 *
 *  Created on: Dec 18, 2013
 *      Author: kimmij
 */

#include "SearchResult.h"

SearchResult::SearchResult()
{

}

SearchResult::SearchResult(int id, int offset, int length, vector<RealT> const& data, RealT distance, RealT R, int R_idx)
{

	// TODO Auto-generated constructor stub

}

SearchResult::SearchResult(int id, int offset, int length)
{
	this->id = id;
	this->offset = offset;
	this->length = length;
}


SearchResult::~SearchResult() {
	// TODO Auto-generated destructor stub
}


void SearchResult::print() {
	printf("id=[%d],offset=[%d],distance=[%f]\n", id, offset, distance);
}

void SearchResult::cleanup() {
}

