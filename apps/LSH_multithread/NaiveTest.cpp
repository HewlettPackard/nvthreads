#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>
#include <boost/atomic.hpp>
#include "headers.h"
#include "SelfTuning.h"
#include <iostream>
#include <vector>
#include <string>
#include "TimeSeriesBuilder.h"
#include "LSHManager.h"
#include "LSHHashFunction.h"
#include "IndexSearch.h"
#include "NaiveSearch.h"
#include "MergeResult.h"
#include <fstream>
#include <chrono>
#include <thread>

/*
#include <boost/bind.hpp>
#include <boost/asio.hpp>
*/

using namespace std;


/* struct to hold data to be passed to a thread
 *    this shows how multiple data items can be passed to a thread */
// SEarchCoordinatorTracker should be added
void testNaiveSearch(int topk_no, string datafile, string queryfile) {
	// create LSHManager
	LSHManager* pLSH = LSHManager::getInstance();

	timespec before, after;

	// create TimeSeriesBuilder to populate time series data
	TimeSeriesBuilder& tBuilder = pLSH->getTimeSeriesBuilder();
	tBuilder.deserialize(datafile);
cout << "Timeseries read done" << endl;

	tBuilder.constructCompactTs();

	SearchCoordinatorTracker& tracker = pLSH->getSearchCoordinatorTracker();
	string sId("search");

	std::ifstream infile(queryfile);
	int query_id = 0;
	for (std::string line; std::getline(infile, line); ) {
		vector<RealT> query;

		// get query points
		char* str = (char*)line.c_str();
		char* pch = strtok (str," ");
	//id = atoi(pch);
	//pch = strtok (NULL, ",");
		cout << "\n\n*****************query " << query_id++ << endl;
	//int i=0;
		while (pch != NULL)
		{
			query.push_back(atof(pch));
	//cout << i << "," << query[i] << endl;
			pch = strtok (NULL, " ");
	//	i++;
		}

        tracker.addSearchCoordinator(sId,query,topk_no);
        SearchCoordinator& searchCoord = tracker.getSearchCoordinator(sId);
        NaiveSearch& naiveSearch = searchCoord.getNaiveSearch();
        priority_queue<SearchResult, vector<SearchResult>, CompareSearchResult> topK;
        vector<SearchResult> vec_result;


        	clock_gettime(CLOCK_MONOTONIC, &before);

		naiveSearch.computeTopK(query, topk_no);

        	while(naiveSearch.getTopK().size() != 0) {
                	SearchResult searchResult = naiveSearch.getTopK().top();
			bool bfind = false;
			for(int j=0;j<vec_result.size();j++) {
				SearchResult mt = vec_result[j];
               			if(mt.getId() == searchResult.getId() &&
               			   mt.getOffset() == searchResult.getOffset()) {
					bfind = true;
					break;
				}
			
			}
			naiveSearch.getTopK().pop();
		//cout << "**id=" << searchResult.getId() << ",offset=" << searchResult.getOffset() << ",distance=" << searchResult.getDistance() << endl;
			if(bfind) continue;
			vec_result.push_back(searchResult);
		}

	for(int i=0;i<vec_result.size();i++) {
		SearchResult searchResult = vec_result[i]; 
		if(topK.size() < topk_no) {
			topK.push(searchResult);
		}
		else {
			SearchResult mt = topK.top();
		 	if(mt.getDistance() > searchResult.getDistance()) {
		   		topK.pop();
			 	topK.push(searchResult);
		    	}
		}
	}
        	clock_gettime(CLOCK_MONOTONIC, &after);


	while(topK.size() > 0) {
	
               	SearchResult searchResult = topK.top();
               	int id = searchResult.getId();
               	int offset = searchResult.getOffset();
               	float distance = searchResult.getDistance();

		cout << "id=" << id << ",offset=" << offset << ",distance=" << distance << endl;

               	topK.pop();
	}
	timespec diff_time = diff(before, after);
	long search_time = diff_time.tv_sec*1000 + diff_time.tv_nsec/1000000;
        cout << "search_time(ms)=" << search_time << endl;
	tracker.removeSearchCoordinator(sId);
	}
}

int main(int nargs, char **args){

	if(nargs < 4) {
		printf("NaiveTest <topk_no> <datafile> <queryfile>\n");
		fflush(stdout);
	} else {
		int topk_no = atoi(args[1]);
		//cout << "topk_no=" << topk_no << endl;
		string datafile = string(args[2]);
		//cout << "datafile=" << datafile << endl;
		string queryfile = string(args[3]);
		//cout << "queryfile=" << queryfile << endl;
		testNaiveSearch(topk_no, datafile, queryfile);
	}

}

