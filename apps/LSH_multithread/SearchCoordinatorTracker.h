/*
 * SearchCoordinatorTracker.h
 *
 *  Created on: Jan 27, 2014
 *      Author: kimmij
 */

#ifndef SEARCHCOORDINATORTRACKER_H_
#define SEARCHCOORDINATORTRACKER_H_

#include <vector>
#include <unordered_map>
#include "SearchCoordinator.h"

using namespace std;

class SearchCoordinatorTracker {
private:
	//unordered_map<string, vector<SearchCoordinator> > trackingSearchCoordinator;
	unordered_map<string, SearchCoordinator> trackingSearchCoordinator;

public:
	SearchCoordinatorTracker();
	virtual ~SearchCoordinatorTracker();
	void addSearchCoordinator(string searchId, vector<RealT>& query, int topk_no);
	SearchCoordinator& getSearchCoordinator(string searchId) {
		return trackingSearchCoordinator[searchId];
	};
	void cleanup(string searchId) {
		if(trackingSearchCoordinator.find(searchId) != trackingSearchCoordinator.end()) {
			trackingSearchCoordinator[searchId].cleanup();
		}
	};
	void removeSearchCoordinator(string searchId) {
		trackingSearchCoordinator.erase(searchId);
	};
	void cleanup();
};

#endif /* SEARCHCOORDINATORTRACKER_H_ */
