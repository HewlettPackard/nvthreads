/*
 * TimeSeries.h
 *
 *  Created on: Dec 17, 2013
 *      Author: kimmij
 */

#ifndef TIMESERIES_H_
#define TIMESERIES_H_

#include <vector>
#include <string>
#include <stdio.h>
#include "headers.h"

using namespace std;

class TimeSeries {
private:
       int m_id;
       vector<RealT> m_vals;  //** maybe change to array
public:
       TimeSeries();
       TimeSeries(int id, vector<RealT> vals);

       virtual ~TimeSeries();

       int getId() {
       	return m_id;
       };

       vector<RealT> const& getValues() {
       	return m_vals;
       };

	bool readFields(string line);
	bool writeFields(std::ofstream &outfile); 
	
	void cleanup() {
		vector<RealT> empty {};
		m_vals = empty;
	}


};



#endif /* TIMESERIES_H_ */
