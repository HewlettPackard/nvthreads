/*
 * HashIndex.cpp
 *
 *  Created on: Dec 17, 2013
 *      Author: kimmij
 */

#include "HashIndex.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
//#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

HashIndex::HashIndex() {

}

HashIndex::HashIndex(int R, int L, int capacity) : Rindex(R), Lindex(L), LSH_HashTable(capacity) {

}

HashIndex::~HashIndex() {
	//printf("HashIndex destroyed\n");
}
//the following commented code is required to see if an integer array can improve a vector when passing to E2LSH for the h1 and h2 computation
//void HashIndex::put(int hash, int collision) {


 //   LSH_HashTable[hash].push_back(collision);


//}

//the following commented code is required to see if an integer array can improve a vector when passing to E2LSH for the h1 and h2 computation
//void HashIndex::get(int hash, MergeResult& mergeResult) {

  //  if(LSH_HashTable.find(hash) != LSH_HashTable.end()){
    	//printf("HashIndex::get retrieved size=[%d]\n", LSH_HashTable[hash].size());
    //	mergeResult.mergeResult(LSH_HashTable[hash]);
    //}
//}
//
void HashIndex::postProcessing() {

	for(unordered_map < h1h2Index, vector<int>, h1h2Index_hash, h1h2Index_equal >::iterator iterator = LSH_HashTable.begin(); iterator != LSH_HashTable.end(); iterator++) {
		vector<int> vec_values = iterator->second;
		vec_values.shrink_to_fit();
	}
}

// the following two functions are for memory statistics
//*how many buckets.
int HashIndex::getNumberOfBuckets() {
	return LSH_HashTable.bucket_count();
}

//*for each buckets, whether it points to the empty collision chain, or to the non-empty collision chain
//*for the collision chain, identify how many elements in the collision chain.
int HashIndex::getTotalMemoryOccupied() {

	//The total memory occupied by the hash table will  be computed as:
	//(total bucket size)* (size of (key + size of (a reference))   +
	// Sum over all of the non-empty bucket (the size of the object metadata +
	// the element in the collision set* the size of each element in the collision )
	int sz = sizeof(LSH_HashTable);
	for (unsigned i = 0; i < LSH_HashTable.bucket_count(); ++i) {
     		sz +=   2*sizeof(void*)+sizeof(size_t);  // 2*sizeof(void*) is for _M_next and _M_value and sizeof(size_t) is for _M_hash code
    
		for ( auto x = LSH_HashTable.begin(i); x!= LSH_HashTable.end(i); ++x ) {
                	sz += sizeof(x); // x is std::pair         
  			sz += sizeof(x->first) + sizeof(x->second) + x->second.capacity()*sizeof(int);
              	} 
    	}

	return sz;
}

// create two int tables for hash index
void HashIndex::create2tables() {

	//cout << "start to create 2-d array hash tables (R,L)=" << Rindex << "," << Lindex << endl;

	hashtable_type = LSHGlobalConstants::TWO_ARRAYS_BASED;
	// memory allocation for the first int array 
	bucket_cnt = LSH_HashTable.bucket_count();
	arr1stTable = (int*)malloc(bucket_cnt*sizeof(int));

	// hash function
	unordered_map < h1h2Index, vector<int>, h1h2Index_hash, h1h2Index_equal >::hasher fn = LSH_HashTable.hash_function();

	size_t pos = 0;
	vector<int> temp_vec;
	// populate the temporary vector for the second int array
	for (unsigned i = 0; i < LSH_HashTable.bucket_count(); ++i) {

		arr1stTable[i] = -1;

		for ( auto x = LSH_HashTable.begin(i); x!= LSH_HashTable.end(i); ++x ) {
			temp_vec.push_back((int)fn(x->first)); // key


			if(arr1stTable[i] == -1) {
				arr1stTable[i] = pos;
			}
			pos++;
			temp_vec.push_back(x->second.size()); // size
			pos++;
			for(int k=0;k<x->second.size();k++) {
				temp_vec.push_back(x->second[k]);  // collisions
				pos++;

			}
			// clean up the vector
			x->second.clear();
			x->second.shrink_to_fit();
        	}

		// markup for the ending 
		temp_vec.push_back(-1); // ending key
		pos++;
		temp_vec.push_back(0); // ending size
		pos++;
	}	
	// memory allocation for the second int array 
	arr2ndTable = (int*)malloc((temp_vec.size())*sizeof(int));
	// memory copy from temporary vector to the second int array 
	memcpy(arr2ndTable, temp_vec.data(), temp_vec.size()*sizeof(int));
	
	LSH_HashTable.clear(); // clean up the unordered_map
	
	//cout << "finish to create 2-d array hash tables (R,L)=" << Rindex << "," << Lindex << endl;
}



void HashIndex::print() {

	printf("HashIndex size=%d\n", LSH_HashTable.size());

	//int s=0;



	//for(unordered_map<int, unordered_map<int, vector<pair< int, int > > > >::iterator iterator = LSH_HashTable.begin(); iterator != LSH_HashTable.end(); iterator++) {
//		int key = iterator->first;
		//printf("HashIndex [%d] size=%d\n", key, LSH_HashTable[key].size());
//		s += LSH_HashTable[key].size();

		//for(unordered_map<int, vector<pair< int, int > > >::iterator iterator1 = LSH_HashTable[key].begin(); iterator1 != LSH_HashTable[key].end(); iterator1++) {
		//	int key1 = iterator1->first;
		//	s += LSH_HashTable[key][key1].size();
		//}
		//vector<pair< int, int > > vec_values = iterator->second;
	//	if(LSH_HashTable[key].size() > 1)
		//printf("HashIndex [%d] size=%d\n", key, LSH_HashTable[key].size());
	//}
//	printf("HashIndex total size=%d\n", s);

		/*
		for(int i=0;i<vec_values.size();i++) {
			int id = vec_values[i].first;
			int offset = vec_values[i].second;
			printf("key[%d]=id[%d],offset[%d]\n", key, id, offset);
		}
		*/
	//}
	//fflush(stdout);

}
/*
int HashIndex::serialize(int fd) {
	cout << "serialize begin" << endl;
	timeval begin,end;
	gettimeofday(&begin, nullptr);
	int sz = LSH_HashTable.size();
	//cout << "serialize sz=" << sz << endl;
	if(write (fd, &(sz), sizeof(int)) <= 0) return -1;
	for(unordered_map<int, vector<pair< int, int > > >::iterator iterator = LSH_HashTable.begin(); iterator != LSH_HashTable.end(); iterator++) {
		int key = iterator->first;
		//cout << "serialize key=" << key << endl;
		if(write (fd, &(key), sizeof(int)) <= 0) return -1;
		vector<pair< int, int > > v = iterator->second;
		int subsz = v.size();
		//cout << "serialize subsz=" << subsz << endl;

		if(write (fd, &(subsz), sizeof(int)) <= 0) return -1;
		for(int i=0;i<subsz;i++) {
			//cout << "serialize v[" << i << "].first=" << v[i].first << endl;

			if(write (fd, &(v[i].first), sizeof(int)) <= 0) return -1;
			//cout << "serialize v[" << i << "].second=" << v[i].second << endl;

			if(write (fd, &(v[i].second), sizeof(int)) <= 0) return -1;
		}
	}
	gettimeofday(&end, nullptr);
	time_t time = (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);
	cout << "serialize done: " << time << " microseconds" << endl;
	return 0;
}
int HashIndex::deserialize(int fd) {
	cout << "deserialize begin" << endl;
	timeval begin,end;
	gettimeofday(&begin, nullptr);

	int sz;
	if(read (fd, &(sz), sizeof(int)) <= 0) return -1;
	for(int i=0;i<sz;i++) {
		int key;
		if(read (fd, &(key), sizeof(int)) <= 0) return -1;
		int subsz;
		if(read (fd, &(subsz), sizeof(int)) <= 0) return -1;
		for(int j=0;j<subsz;j++) {
			int id;
			int offset;
			if(read (fd, &(id), sizeof(int)) <= 0) return -1;
			if(read (fd, &(offset), sizeof(int)) <= 0) return -1;
			LSH_HashTable[key].push_back(make_pair(id,offset));
		}
	}
	gettimeofday(&end, nullptr);
	time_t time = (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);
	cout << "deserialize done: " << time << " microseconds" << endl;

	return 0;
}
*/

