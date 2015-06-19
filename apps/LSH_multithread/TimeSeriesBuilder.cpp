/*
 * TimeSeriesBuilder.cpp
 *
 *  Created on: Dec 17, 2013
 *      Author: kimmij
 */

#include "TimeSeriesBuilder.h"
#include <fstream>

TimeSeriesBuilder::TimeSeriesBuilder() {


}

TimeSeriesBuilder::~TimeSeriesBuilder() {
	// TODO Auto-generated destructor stub
}

//map<int, TimeSeries>& TimeSeriesBuilder::getAllTimeSeries() {
vector<TimeSeries>& TimeSeriesBuilder::getAllTimeSeries() {

	return timeseries_map;
}

void TimeSeriesBuilder::print() {
	/*
	for(map<int, TimeSeries>::iterator iterator = timeseries_map.begin(); iterator != timeseries_map.end(); iterator++) {
		TimeSeries &ts = iterator->second;
		int id = ts.getId();
		printf("Timeseries id[%d],vals[0][%f]\n", id, ts.getValues().at(0));

	}
	fflush(stdout);
*/
}

TimeSeries const& TimeSeriesBuilder::get(int id) {

	return timeseries_map[id];
}

bool TimeSeriesBuilder::find(int id) {
	/*
	if ( timeseries_map.find(id) == timeseries_map.end() )
		return false;
	else
		return true;
		*/
	return true;
}

void TimeSeriesBuilder::put(int id, TimeSeries const& timeseries)  {
	//timeseries_map[id] = timeseries;

	timeseries_map.push_back(timeseries);
}


// construct 2-d pointer array of candidates
void TimeSeriesBuilder::constructCompactTs() {

// this is only used by serializatin the input data, which is used by LSHtest
#ifdef SERIALIZE_INPUT
        cout << "start serializing" << endl;
        serialize("ts_file4M_part1");
        cout << "finish serializing" << endl;
#endif

  this->ts_cnt = timeseries_map.size();
cout << "start constructCompactTs, ts_cnt=" << ts_cnt << endl;
  this->compact_timeseries= (RealT**)malloc(this->ts_cnt*sizeof(RealT*));
  this->ts_length = (size_t*)malloc(this->ts_cnt*sizeof(size_t)); 
  for (int i=0; i<this->ts_cnt; i++) {
    TimeSeries &ts=timeseries_map[i];
    vector<RealT> const &vals = ts.getValues();
    int columnSize = vals.size();
    this->compact_timeseries[i]=(RealT*)malloc(columnSize*sizeof(RealT));
    for (int j=0; j<columnSize; j++) {
      this->compact_timeseries[i][j] = vals.at(j);
    }
    this->ts_length[i] = columnSize;
  }
// free values (Note that only values are freed, 'id' is still used)
cout << "start freeTsVector" << endl;
  freeTsVector();
cout << "end freeTsVector" << endl;
}
