/*
 * LSHManager.h
 *
 *  Created on: Jan 3, 2014
 *      Author: kimmij
 */

#ifndef LSH_H_
#define LSH_H_


typedef unsigned int bitset_type;



class LSHGlobalConstants {
public:
        //static const unsigned int TOTAL_NUM_TIMESERIES_POINTS = 172944;
	static const unsigned int TOTAL_NUM_TIMESERIES_POINTS = 4000000;
	static const unsigned int NUM_TIMESERIES_POINTS = 1; //=17250/48
	static const unsigned int POS_TIMESERIES = 384;

	enum {TWO_ARRAYS_BASED, UNORDERED_MAP};

};


class LSHPerformanceCounter {

private:
	int totalPointsInFinalNaiveSearch;
	int totalTimeSpentInPertubation;  //milliseconds;
	int totalTimeSpentInNaiveSearch;  //milliseconds

public:
	inline void setTotalPointsInFinalNaiveSearch(int val) {
		totalTimeSpentInNaiveSearch = val;
	};

	inline int getTotalTimeSpentInFinalNaiveSearch() const {
		return totalTimeSpentInNaiveSearch;
	};

	void reset() {
		totalTimeSpentInNaiveSearch = 0;
		totalTimeSpentInPertubation = 0;  //milliseconds;
	};
};



#endif /* LSH_H_ */
