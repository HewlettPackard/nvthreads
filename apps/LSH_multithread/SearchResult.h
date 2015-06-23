/*
 * SearchResult.h
 *
 *  Created on: Dec 18, 2013
 *      Author: kimmij
 */
#include <vector>
#ifndef SEARCHRESULT_H_
#define SEARCHRESULT_H_

#include "headers.h"

using namespace std;

class SearchResult {
private:
	int id;
	int offset;
	int length;
	RealT distance;
	RealT R;
	int R_idx;
public:
	SearchResult();
	SearchResult(int id, int offset, int length, vector<RealT> const& data, RealT distance, RealT R, int R_idx);
	SearchResult(int id, int offset, int length);

	virtual ~SearchResult();

	void set(int id, int offset, int length) {
		this->id = id;
		this->offset = offset;
		this->length = length;
	};
	void setId(int id) {
		this->id = id;
	};
	RealT getDistance() {
		return distance;
	};
	void setDistance(RealT dist) {
		distance = dist;
	};
	int getId() {
		return id;
	};
	int getOffset() {
		return offset;
	};
	void print();
	void cleanup();

};

class CompareSearchResult {
public:
    bool operator()(SearchResult& r1, SearchResult& r2)
    {
    	RealT dist1 = r1.getDistance();
    	RealT dist2 = r2.getDistance();

		if (dist1 < dist2) return true;
		return false;
    }
};

#endif /* SEARCHRESULT_H_ */
