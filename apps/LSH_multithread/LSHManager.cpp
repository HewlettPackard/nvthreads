/*
 * LSHManager.cpp
 *
 *  Created on: Jan 3, 2014
 *      Author: kimmij
 */

#include "LSHManager.h"
#include "TimeSeriesBuilder.h"
#include "IndexBuilder.h"
#include "HashIndex.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

LSHManager::LSHManager() {
   //added by Jun Li
   compacted=false; //initialized to not being compacted.
}

LSHManager::~LSHManager() {
	printf("LSHManager gets deleted\n");
}

void LSHManager::initHashIndex(int R_num, int L_num, int capacity) {
	num_R = R_num;
	num_L = L_num;
	for(int i=0;i<R_num;i++) {
		vector<HashIndex> vec;
		for(int j=0;j<L_num;j++) {
			HashIndex hashIndex(i,j,capacity);
			vec.push_back(hashIndex);
		}
		vecHashIndex.push_back(vec);
	}
}



/*
HashIndex LSHManager::createHashIndex(int capacity) {

	int sz = vecHashIndex.size();
	int sz1 = 0;
	if(sz > 0) {
		sz1 = vecHashIndex[sz-1].size();
	}
	HashIndex hashIndex(sz,sz1,capacity);
	vector<HashIndex> vec;
	vec.push_back(hashIndex);
	vecHashIndex.push_back(vec);
	return hashIndex;
}
*/


void LSHManager::serialize() {
	/*
	int fd = open ("hashindex.out", O_WRONLY | O_CREAT, 0666);
	cout << "serialize num_R=" << num_R << ", num_L=" << num_L << endl;
	for(int i=0;i<num_R;i++) {
		for(int j=0;j<num_L;j++) {
			vecHashIndex[i][j].serialize(fd);
		}
	}
	*/
	//close(fd);
}
void LSHManager::deserialize() {
	/*
	int fd = open ("hashindex.out", O_RDONLY);
	for(int i=0;i<num_R;i++) {
		for(int j=0;j<num_L;j++) {
			vecHashIndex[i][j].deserialize(fd);
		}
	}
	*/
	//close(fd);
}

//Jun Li: I do not want to have the JNI call to come to this C++ call directly. Instead, I am using "initiateSearchCoordinatorAtR" JNI
//call to do the work, which then should not be multithreaded environment 
void LSHManager::constructCompactTs() {
  if(!compacted) {
           timeSeriesBuilder.constructCompactTs();//we will do this only once.
           compacted=true;
           cout << "done with time series builder memory compaction"<<endl;
  }
}



