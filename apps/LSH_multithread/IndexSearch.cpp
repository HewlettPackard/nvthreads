/*
 * IndexSearch.cpp
 *
 *  Created on: Dec 18, 2013
 *      Author: kimmij
 */

#include <random>
#include <cmath>
#include <iostream>
#include "IndexSearch.h"
#include "LSHManager.h"
#include "headers.h"
#include <unordered_set>
#include <vector>
#include <unistd.h>
#include <sys/time.h>
#include "QueryHashFunctions.h"
#include <armadillo>
#include <blaze/math/DynamicMatrix.h>

using blaze::DynamicMatrix;

using namespace arma;

IndexSearch::IndexSearch() {
	// TODO Auto-generated constructor stub

}

IndexSearch::~IndexSearch() {
	// TODO Auto-generated destructor stub
}

/**
 * Search method using query hashes as precomputed vectors
 * to speep up perturbation indexing.
 * Paramters: query, R, L rande, results and queryHashes.
 * queryHashes class implemented a new method of getting index using a single set of random vectors.
 *
 * ASSUMPTION: The first R is RIndex =0, if this is not true, then the logic need to change.
 * Created by Tere Gonzalez
 * April 10, 2014
 */
time_t IndexSearch::searchIndex(vector<RealT> & query, int R, pair<int,int> const& range_L, int pert_no, MergeResult& mergeResult,QueryHashFunctions &queryHashes){

	LSH_HashFunction& lshHash 		= LSHManager::getInstance()->getLSH_HashFunction();
	//Getting hash structures for R=0 only
	PRNearNeighborStructT& nnStruct = lshHash.getNnStruct(0);
	DynamicMatrix<RealT>& a = lshHash.getMat(0);
	PUHashStructureT & uHash		= lshHash.getPUHash(0);
	//cout <<endl<<" ...Method WITH PRECOMPUTATION***********WITH INNERJOIN";

	vector <RealT> atimesByRW;

	/*Parameters for the hashes*/
	int kLength		= nnStruct->hfTuplesLength;
	int W 			= nnStruct->parameterW;
	RealT rValue 	= lshHash.getR(R);
//cout << "search test rValue=" << rValue << endl;
	int lLength		= range_L.second-range_L.first+1;

	//h1 and h2 results values
	Uns32T			h1[lLength];
	Uns32T			h2[lLength];


	time_t htime = 0;
	time_t htime1= 0;
	time_t htime2= 0;
	time_t htime3= 0;
	struct timespec before, after;


	clock_gettime(CLOCK_MONOTONIC,&before);

	//if R==0, then we compute the vectors, otherwise reuse the vectos
	if( R==0 ) {
		// vector a has been added for matrix multiplication for Hash generation
		if (!queryHashes.computeVectors(query,  kLength, lLength,pert_no,nnStruct->lshFunctions, a)){
			cout <<endl<<"error creating vectors";
			return 0; //Error computing vectors, we can not continue..
			}
	}

	clock_gettime(CLOCK_MONOTONIC,&after);
	htime1 =(diff(before, after).tv_nsec)/1000;



	//computing atimesbyRandW

	/*vector<RealT> &atimesX= queryHashes.getATimesX();

	atimesByRW.resize(query.size());
	for (int d =0; d < query.size(); d++) {
		atimesByRW [d] =atimesX/(rValue);
	}*/




	//get the index for the query
	queryHashes.getIndex(W,rValue,h1,h2,nnStruct->lshFunctions,uHash);

	for (int l=0; l< lLength; l++){
		HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(R,range_L.first+l);
		hashIndex.get(h1[l],h2[l], mergeResult);
	}

	htime2=0;

	//get the index for each perturbation
	for ( int n = 0; n < pert_no; n++)  {
		//get the index for the delta : query perturbation
		clock_gettime(CLOCK_MONOTONIC,&before);
		queryHashes.getIndex(W,rValue,h1,h2,nnStruct->lshFunctions,uHash,n);
		clock_gettime(CLOCK_MONOTONIC,&after);
		htime2+=((diff(before, after).tv_nsec)/1000);

		for (int l=0; l< lLength; l++){
				HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(R,range_L.first+l);
				hashIndex.get(h1[l],h2[l], mergeResult);
		}
	}



	htime = htime2 +htime1;
	//cout<<endl<<"vectors total time1:"<<htime1;
	//cout<<endl<<"h1h2 total time:"<<htime2;

	

#ifdef DESERIALIZE_CANDIDATES
 	bitset<LSHGlobalConstants::TOTAL_NUM_TIMESERIES_POINTS>& mergedResult = mergeResult.getMergedResult();
        static int query_id = -1;
        if(R == 0) query_id++;

        char filename[50];
        sprintf(filename,"./candidates_K21L600P7/candidates_%f.02_%f.02_%d", query[0], query[1], R);
        //sprintf(filename,"./K21L600P7candidates_query990/candidates_%d_%d", query_id, R);
cout << "candfile=" << filename << endl;
cout << "deserialize\n";
        //timespec before, after;
        //clock_gettime(CLOCK_MONOTONIC, &before);
        FILE * f = fopen(filename,"r");
        if(f == NULL)
                return htime;

        size_t result;
        int pcnt = 0;
        int p_row, p_col;

        while(true) {
                result = fread(&p_row, 1, sizeof(int), f);
                if(result <= 0)
                        break;
                fread(&p_col, 1, sizeof(int), f);
                int pos = p_row*LSHGlobalConstants::NUM_TIMESERIES_POINTS+p_col/LSHGlobalConstants::POS_TIMESERIES;
                mergedResult[pos] = true;

                if(pcnt < 10) {
                        cout << p_row << "," << p_col << endl;
                        pcnt++;
                }
        }
        fclose(f);
        //clock_gettime(CLOCK_MONOTONIC, &after);
        //long time_deser = diff(before, after).tv_nsec;
        //cout << "deserialize time=" << time_deser << endl;
#endif

	return htime;

}

/*
 * Search index method.
 * It computes perturbations and then get index of the points
 * This method is the same for all the R iterations
 * Created by Mijung Kim
 */
time_t IndexSearch::searchIndex(vector<RealT> & query, int R, pair<int,int> const& range_L, int pert_no, MergeResult& mergeResult) {
	LSH_HashFunction& lshHash = LSHManager::getInstance()->getLSH_HashFunction();

	timeval begin,end;
	time_t htime = 0;

	cout <<endl<<"Method NO PRECOMPUTATION*********** AND INNER PRODUCT";

	//set<pair<Uns32T,Uns32T>> merged_hindex[range_L.second-range_L.first+1];

	// h1 and h2 for the query point
	gettimeofday(&begin, nullptr);
	vector<pair<Uns32T,Uns32T>> hindex;
	lshHash.getIndex(R, range_L, (RealT*)&(query.at(0)), query.size(), hindex);
	gettimeofday(&end, nullptr);
	htime += (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);

	// retrieve candidate points and pass to mergeResult
	//printf("query[%f][%f] \n", query[0],query[1]);
	//printf("query hashIndex[%d][%d] \n", hindex[0].first,hindex[0].second);
	for(int i=0;i<hindex.size();i++) {
		HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(R,range_L.first+i);
		//printf("h1[%d], h2[%d] \n", hindex.at(i).first, hindex.at(i).second);
		//merged_hindex[i].insert(make_pair(hindex.at(i).first, hindex.at(i).second));
		hashIndex.get(hindex.at(i).first, hindex.at(i).second, mergeResult);
	}

	//  Create perturbation list
	//  1. Generate d independent normal random variables x[i][1], x[i][2], ..., x[i][d], each with zero mean and unit variance.
	//  2. Compute Xnorm = sqrt(x[i][1]*x[i][1]+x[i][2]*x[i][2]+ ...+x[i][d]*x[i][d])
	//  3. Set delta[i] to be the vector (x[i][1], x[i][2], ..., x[i][d])/Xnorm
	//  4. Compute H(Q+delta[i]), the LSH Hash of Q+delta[i]
	//  5. Add H(Q+delta[i]) to S_H(l)

	vector<pair<Uns32T,Uns32T>> hindex_pert;
	RealT delta[query.size()];
	RealT dsum;
	// For the following three variables, maybe private member for IndexSearch or SearchCoordinator?
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::normal_distribution<> dist(0,1);
	for(int j=0;j<pert_no;j++) {
		dsum = 0.0;
		//gen.seed(time(nullptr)*j);

		gen.seed(45354*j+j*j);

		for(int d=0;d<query.size();d++) {
			delta[d] = dist(gen);
			dsum += pow(delta[d],2);
		}
		dsum = sqrt(dsum);
		for(int d=0;d<query.size();d++) {
			delta[d] = query[d] + lshHash.getR(R)*delta[d]/dsum;
		}
		//printf("%d delta[0]=%f\n", j, delta[0]);

		hindex_pert.clear();
		// h1 and h2 for the perturbation point
		gettimeofday(&begin, nullptr);
		lshHash.getIndex(R, range_L,(RealT*)&(delta), query.size(), hindex_pert);
		gettimeofday(&end, nullptr);
		htime += (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);

		// retrieve candidate points and pass to mergeResult
		for(int i=0;i<hindex_pert.size();i++) {
			//printf("Lindex=%d\n", i);
			HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(R,range_L.first+i);
			hashIndex.get(hindex_pert.at(i).first, hindex_pert.at(i).second, mergeResult);
		//	merged_hindex[i].insert(make_pair(hindex_pert.at(i).first, hindex_pert.at(i).second));

		}
	}
	//for(int i=0;i<hindex.size();i++) {
	//	HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(R,range_L.first+i);
	//	for (set<pair<Uns32T,Uns32T> >::iterator it=merged_hindex[i].begin(); it!=merged_hindex[i].end(); ++it) {
	//		pair<int, int> val = *it;
	//		hashIndex.get(val.first, val.second, mergeResult);
	//	}
	//}
	return htime;
}

/**
 * This is a test method ONLY
 * Search method using query hashes as precomputed vectors
 * to test original computation for all the R.
 * Paramters: query, R, L rande, results and queryHashes.
 * queryHashes class implemented a new method of getting index using a single set of random vectors.
 *
 * ASSUMPTION: This method should be removed when the code is stabilized
 * Created by Tere Gonzalez
 * April 10, 2014
 */
time_t IndexSearch::searchIndexPrecomputationAllR(vector<RealT> & query, int R, pair<int,int> const& range_L, int pert_no, MergeResult& mergeResult,QueryHashFunctions &queryHashes) {

	LSH_HashFunction& lshHash 		= LSHManager::getInstance()->getLSH_HashFunction();
	PRNearNeighborStructT& nnStruct = lshHash.getNnStruct(R);
	DynamicMatrix<RealT>& a = lshHash.getMat(R);
	PUHashStructureT & uHash		= lshHash.getPUHash(R);

	cout <<endl<<"Running precomputation...starting search index";
	int kLength		= nnStruct->hfTuplesLength;
	int W 			= nnStruct->parameterW;
	RealT rValue 	= lshHash.getR(R);


	cout <<endl<<"@LValue:"<<range_L.second<<" --->"<<range_L.first;
	int lLength		= range_L.second-range_L.first+1;

	Uns32T			h1[lLength];
	Uns32T			h2[lLength];
	struct timespec before, after;
	long time1;
	long time2;
	long time3;

	cout<<endl<<"R:"<<R<<"Rindex"<<rValue;

	clock_gettime(CLOCK_MONOTONIC,&before);
	//if( R==0 ) {
		if (!queryHashes.computeVectors(query,  kLength, lLength,pert_no,nnStruct->lshFunctions, a)){
			return 0; //Error computing vectors, we can not continue..
			}
		//}

	clock_gettime(CLOCK_MONOTONIC,&after);

	cout<<endl<<"..............hashes start";
	time1=diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC,&before);
	queryHashes.getIndex(W,rValue,h1,h2,nnStruct->lshFunctions,uHash);
	clock_gettime(CLOCK_MONOTONIC,&after);
	time2 =diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC,&before);

	for (int l=0; l< lLength; l++){
		HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(R,l);
		//hashIndex.get(hash, mergeResult);
		hashIndex.get(h1[l],h2[l], mergeResult);
	}

	clock_gettime(CLOCK_MONOTONIC,&after);
	time3 =diff(before, after).tv_nsec;

	clock_gettime(CLOCK_MONOTONIC,&after);

	for ( int n = 0; n < pert_no; n++)  {
		clock_gettime(CLOCK_MONOTONIC,&before);

		queryHashes.getIndex(W,rValue,h1,h2,nnStruct->lshFunctions,uHash,n);

		clock_gettime(CLOCK_MONOTONIC,&after);
		time2 += diff(before, after).tv_nsec;

		clock_gettime(CLOCK_MONOTONIC,&before);

		for (int l=0; l< lLength; l++){
			HashIndex& hashIndex = LSHManager::getInstance()->getHashIndex(R,l);
			hashIndex.get(h1[l],h2[l], mergeResult);
		}
		clock_gettime(CLOCK_MONOTONIC,&after);
		time3 += diff(before, after).tv_nsec;

	}
	clock_gettime(CLOCK_MONOTONIC,&after);
	time2=diff(before, after).tv_nsec;
	cout <<endl<<"time1:"<<time1;
	cout <<endl<<"time2:"<<time2;
	cout <<endl<<"time3:"<<time3;
	cout<<endl<<"..............hashes start"<<endl<<endl;

	time1= (time1+time2+time3)/1000;
	cout  <<endl<<"total time:"<<time1<<endl;
	return time1;

}


