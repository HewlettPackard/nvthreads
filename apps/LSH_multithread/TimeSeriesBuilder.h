/*
 * TimeSeriesBuilder.h
 *
 *  Created on: Dec 17, 2013
 *      Author: kimmij
 */
#ifndef TIMESERIESBUILDER_H_
#define TIMESERIESBUILDER_H_
#include <map>
#include "TimeSeries.h"
//#include <google/malloc_extension.h>

using namespace std;

class TimeSeriesBuilder {
private:
       //map<int, TimeSeries> timeseries_map;
       vector<TimeSeries> timeseries_map;
       //pointer based compact representation 
       RealT **compact_timeseries;
       size_t ts_cnt;
       size_t* ts_length;
public:
       TimeSeriesBuilder();
       virtual ~TimeSeriesBuilder();

       bool find(int id);
       TimeSeries const& get(int id);
       void put(int id, TimeSeries const& timeseries);
       //map<int, TimeSeries>& getAllTimeSeries();
       vector<TimeSeries>& getAllTimeSeries(); 
       void print();

public: 
       void constructCompactTs(); 
       RealT *getCompactTs(int id) {
    	     return compact_timeseries[id];
       }
       size_t getCountAllTimeSeries(){
	     return ts_cnt;
       }
       size_t getTsLength(int i) {
             return ts_length[i];
       }
       void freeTsVector() {
                for(int i=0;i<timeseries_map.size();i++) {
                        TimeSeries &ts = timeseries_map[i];
            		ts.cleanup();
                }
		//MallocExtension::instance()->ReleaseFreeMemory();
       };

       void serialize(string filename) {
                std::ofstream outfile(filename.c_str());
                for(int i=0;i<timeseries_map.size();i++) {
                        TimeSeries &ts = timeseries_map[i];
                        ts.writeFields(outfile);
                }
       };
       void deserialize(string filename) {
            std::ifstream infile(filename.c_str());
            std::string line;
            while (std::getline(infile, line)) {
                        TimeSeries ts;
                        bool res = ts.readFields(line);
                        timeseries_map.push_back(ts);
                        if(!res)
                                break;
                }
	    infile.close();

       };
       
};


#endif /* TIMESERIESBUILDER_H_ */
