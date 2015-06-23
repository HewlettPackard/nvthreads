/*
 * HashIndex.h
 *
 *  Created on: Dec 17, 2013
 *      Author: kimmij
 */

#ifndef HASHINDEX_H_
#define HASHINDEX_H_
#include <unordered_map>
#include <bitset>
#include "MergeResult.h"
#include "LSH.h"
#define CHECK_BIT(var,pos) ((var) & (ptrdiff_t(1)<<(pos)))

using namespace std;

struct h1h2Index {

	int h1;
	int h2;

	h1h2Index(int i1, int i2) : h1(i1), h2(i2) {
	};


        size_t hf() 
        {
                const int prime = 31;
                int hash = 1;
                hash = prime * hash + h1;
                hash = prime * hash + h2;

                return hash;
        }


};

struct h1h2Index_hash {

	size_t operator()(const h1h2Index& hIndex) const
	{
		const int prime = 31;
		int hash = 1;
		hash = prime * hash + hIndex.h1;
		hash = prime * hash + hIndex.h2;

		return hash;
	}
};

struct h1h2Index_equal {
	bool operator()(const h1h2Index& hIndex, const h1h2Index& hIndex2) const
	{
		if(hIndex.h1 == hIndex2.h1 && hIndex.h2 == hIndex2.h2)
			return true;
		else
			return false;
	}

};


class HashIndex {
private:
	//unordered_map < int, unordered_map<int,vector<pair< int, int > > > >  LSH_HashTable;  // key: h1 and h2, value: set of time series (id, offset) (no duplicates)
	//unordered_map < int, vector<pair< int, int > > >  LSH_HashTable;  // key: h1 and h2, value: set of time series (id, offset) (no duplicates)
	unordered_map < h1h2Index, vector<int>, h1h2Index_hash, h1h2Index_equal >  LSH_HashTable;  // key: h1 and h2, value: set of time series (id, offset) (no duplicates)


	int Rindex;
	int Lindex;
	int size_hashIndex;

	int hashtable_type = LSHGlobalConstants::UNORDERED_MAP;
	// two int tables
	int* arr1stTable;
	int* arr2ndTable;
	size_t bucket_cnt;

public:
	   HashIndex();
	   HashIndex(int Rindex, int Lindex, int capacity);
       virtual ~HashIndex();

       int getRindex() {
    		return Rindex;
       };
       int getLindex() {
    		return Lindex;
       };

       // clean up the unordered_map
       void cleanup() {
            unordered_map < h1h2Index, vector<int>, h1h2Index_hash, h1h2Index_equal >  empty {};
            LSH_HashTable = empty; 
       };

       void get(int h1, int h2, MergeResult& mergeResult) {

    		h1h2Index h(h1,h2); // make hash index
		if(hashtable_type == LSHGlobalConstants::UNORDERED_MAP) {
                	unordered_map < h1h2Index, vector<int>, h1h2Index_hash, h1h2Index_equal >::const_iterator p =LSH_HashTable.find(h);
            		if (p != LSH_HashTable.end()) {
               			mergeResult.mergeResult((vector<int>&)p->second);
            		}
			return;
		}

		// using two int tables
    		size_t hc = h.hf();  // hash code
    		size_t key = hc % bucket_cnt;  // generate key
    		int pos = arr1stTable[key];
		
		//cout << "key=" << key << ",get pos=" << pos << endl;
		// key is not found
    		if(pos == -1) {
			return;
    		}
	
    		while(arr2ndTable[pos] != -1 && arr2ndTable[pos+1] != 0) {  // while it is not the ending 
			bool bfind = false;
			if(arr2ndTable[pos] == hc) {
				bfind = true;
			}
			pos++;
			size_t sz = arr2ndTable[pos];
			if(bfind) {
				for(size_t i=0;i<sz;i++) {
					pos++;
					mergeResult.setResult(arr2ndTable[pos]); // send candidates to mergeResult
				}
				break;
			} else {
				pos += (sz+1);
			}
		}
		
       };
       size_t set_boolarr(int h1, int h2, bitset_type* mergedResult, size_t bitset_size) { //, int* position_row, int* position_col, int set_cnt) {
		
		size_t set_cnt = 0;
		size_t sz = sizeof(bitset_type);

                h1h2Index h(h1,h2); // make hash index

                // using two int tables
                size_t hc = h.hf();  // hash code
                size_t key = hc % bucket_cnt;  // generate key
                int pos = arr1stTable[key];

                //cout << "key=" << key << ",get pos=" << pos << endl;
                // key is not found
                if(pos == -1) {
                        return set_cnt;
                }

                while(arr2ndTable[pos] != -1 && arr2ndTable[pos+1] != 0) {  //HECK_BIT(var,pos) ((var) & (1<<(pos)))
                        bool bfind = false;
                        if(arr2ndTable[pos] == hc) {
				bfind = true;
                        }
                        pos++;
                        size_t sz = arr2ndTable[pos];
                        if(bfind) {
                                for(size_t i=0;i<sz;i++) {
                                        pos++;
					size_t p1 = arr2ndTable[pos]/bitset_size;
					size_t p2 = arr2ndTable[pos]%bitset_size;
//cout << "before setbit mergedResult=" << mergedResult[p1] << "," << arr2ndTable[pos] << "," << p1 << "," << p2 << endl; 
					if(CHECK_BIT(mergedResult[p1],p2) == 0) {
						mergedResult[p1] |= ptrdiff_t(1) << p2;
//cout << "after setbit mergedResult=" << mergedResult[p1] << endl; 
						//position_row[set_cnt]= arr2ndTable[pos]/LSHGlobalConstants::NUM_TIMESERIES_POINTS;
                				//position_col[set_cnt]= arr2ndTable[pos]%LSHGlobalConstants::NUM_TIMESERIES_POINTS*LSHGlobalConstants::POS_TIMESERIES;
						set_cnt++;
					}
                                }
                                break;
                        } else {
                                pos += (sz+1);
                        }
                }
		return set_cnt;

       };

       void put(int h1, int h2, int collision) {
		   h1h2Index h(h1,h2);
		   LSH_HashTable[h].push_back(collision);
       };
       //the following commented code is required to see if an integer array can improve a vector when passing to E2LSH for the h1 and h2 computation
       //void put(int hash, int collision);
       //void get(int hash, MergeResult& mergeResult);

       void print();
       void postProcessing();

       //*how many buckets.
       int getNumberOfBuckets();
       //*for each buckets, whether it points to the empty collision chain, or to the non-empty collision chain
       //*for the collision chain, identify how many elements in the collision chain.
       int getTotalMemoryOccupied();
       void create2tables();


       //int serialize(int fd);
       //int deserialize(int fd);

};

#endif /* HASHINDEX_H_ */

