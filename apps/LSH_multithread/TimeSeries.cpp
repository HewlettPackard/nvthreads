/*
 * TimeSeries.cpp
 *
 *  Created on: Dec 17, 2013
 *      Author: kimmij
 */

#include "TimeSeries.h"
#include <stdio.h>
#include <string.h>
#include <fstream>

TimeSeries::TimeSeries() {

}

TimeSeries::TimeSeries(int id, vector<RealT> vals) : m_id(id), m_vals(vals) {

}

TimeSeries::~TimeSeries() {
	// TODO Auto-generated destructor stub
}

bool TimeSeries::writeFields(std::ofstream &outfile) {

        outfile << m_id << "," << m_vals[0];
        for(int i=1;i<m_vals.size();i++) {
            outfile <<  "," << m_vals[i];
        }
        outfile << endl;
}

bool TimeSeries::readFields(string line) {

	char* str = (char*)line.c_str();
	char* pch = strtok (str,",");
	m_id = atoi(pch);
	//printf ("id=%s\n",pch);
	pch = strtok (NULL, ",");
	int i=0;
	while (pch != NULL)
	{
		m_vals.push_back(atof(pch));
		pch = strtok (NULL, ",");
	}
/*
	int result;
	result = fread (&(id) , sizeof(long) , 1 , pFile);
	if(result != sizeof(long))
		return false;
	result = fread (&(start_time) , 1, sizeof(long) , pFile);
	if(result != sizeof(long))
		return false;
	result = fread (&(interval) , 1, sizeof(long) , pFile);
	if(result != sizeof(long))
		return false;
	int sz;
	result = fread (&(sz) , 1, sizeof(int) , pFile);
	if(result != sizeof(int))
			return false;

	for(int i=0;i<sz;i++) {
		result = fread (&(vals[i]) , 1, sizeof(RealT) , pFile);
		if(result != sizeof(RealT))
			return false;

	}
	*/
	return true;
}


