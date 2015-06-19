/*
 * SearchCoordinatorTracker.cpp
 *
 *  Created on: Jan 27, 2014
 *      Author: kimmij
 */

#include "SearchCoordinatorTracker.h"
#include "SearchCoordinator.h"

SearchCoordinatorTracker::SearchCoordinatorTracker() {
	// TODO Auto-generated constructor stub

}

SearchCoordinatorTracker::~SearchCoordinatorTracker() {
	// TODO Auto-generated destructor stub
}

void SearchCoordinatorTracker::addSearchCoordinator(string searchId, vector<RealT>& query, int topk_no) {
	//create the search coordinator first.
	SearchCoordinator searchCoord(searchId,query,topk_no);
	trackingSearchCoordinator[searchId] = searchCoord;
}

void SearchCoordinatorTracker::cleanup() {
	for ( auto it = trackingSearchCoordinator.begin(); it != trackingSearchCoordinator.end(); ++it ) {
		((SearchCoordinator&)it->second).cleanup();
	}
}

