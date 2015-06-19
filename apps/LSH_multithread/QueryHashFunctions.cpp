/*
 * QueryHashFunctions.cpp
 *
 *  Created on: Mar 24, 2014
 *      Author: gomariat
 */

#include "QueryHashFunctions.h"
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
#include <armadillo>
#include <blaze/math/DynamicMatrix.h>
#include <blaze/math/DenseColumn.h>
#include <blaze/math/DynamicVector.h>

using blaze::DynamicMatrix;
using blaze::DynamicVector;

using namespace arma;


QueryHashFunctions::QueryHashFunctions() {

}

QueryHashFunctions::~QueryHashFunctions() {
	// TODO Auto-generated destructor stub
	cleanUp();
}
/**
 * Init precomputed arrays for a given query length, L,K
 * if the array is different of null, only clean the mmemory
 * assuming that the querylengths are the same;
 * parameters: lenght of each array
 * return: false if the memory can not be alllocated
 */
bool QueryHashFunctions::init(int k, int L, int perTotal, int queryLenght){

	kLength				= k;
	lLength				= L;
	perturbationNumber	= perTotal;
	queryLength			= queryLenght;


	long deltaLength	 	= queryLength * perturbationNumber;
	long aTimesXLength 		= lLength * kLength;
	long aTimesDeltaLength 	= lLength * kLength * perturbationNumber;



	if (deltaLength <0  || aTimesXLength<=0|| aTimesDeltaLength<0) {
		std::cout << endl <<"Invalid range of parameters. Please verify L, K, or perturbation";
		std::cout << endl <<"k:"<<k<<" L:"<<L<<" perTotal" << perTotal<<" queryLenght"<< queryLenght;
		return false;
	}

	//delta.resize(deltaLength);
	//aTimesDelta.resize(aTimesDeltaLength);
	//aTimesX.resize(aTimesXLength);

 	return true;
}
/*
 * Cleanup vectors and properties
 *
 */
void QueryHashFunctions::cleanUp(){

	//delta.resize(0);
	//aTimesDelta.resize(0);
	//aTimesX.resize(0);

	kLength				= 0;
	lLength				= 0;
	perturbationNumber	= 0;
	queryLength			= 0;
}
/**
 * Compute precomputed vectors in order to compute g(v) = (a*x +b)/w, where W=c/R
 * delta: store perturbations of the query
 * atimesx:compute the a*x, where x is the points int the query
 * atimesDelta: compute the a*delta, where delta is rand/sqrt(sum(x))
 */
bool QueryHashFunctions::computeVectors(vector<RealT>& query, int k, int L, int totalPerturbations, LSHFunctionT **lshR0Functions, DynamicMatrix<RealT> &a ) {

		if (!init(k,L,totalPerturbations,query.size())) {
			std:cout<<"Invalid memory allocation for vectors";
			return false;
		}

		// the first column is for query point
		DynamicVector<RealT> query_point(queryLength, query.data());
		// Other columns for perturbation points
		DynamicMatrix<RealT> query_pert(queryLength,1+totalPerturbations);
		column(query_pert,0) = query_point;
		
		// normalize the perturbation points
		for (int n=1; n<=perturbationNumber; n++) {
			arma_rng::set_seed(45354*n+n*n); 
			Col<RealT> nn = normalise(randn<Col<RealT>>(queryLength));
			DynamicVector<RealT> delta_vec(queryLength, nn.memptr()); 
			column(query_pert, n) = delta_vec; 
		}

		// for debugging
		//cout << "a size=" << a.rows() << "," << a.columns() << endl;
		//cout << "query_pert size=" << query_pert.rows() << "," << query_pert.columns() << endl;
		aTimesXDelta = a*query_pert; // using blaze matrix multiplication

		return true;
}
/**
 *  Computing buckets, h1 and h2 indexs for a L lenght and a query
 *  using precomputed vectors
 *  retruns the values in h1yL and h2byL
 */
void QueryHashFunctions::getIndex(int W, RealT rValue, Uns32T *h1byL, Uns32T *h2byL,LSHFunctionT **lshR0Functions,PUHashStructureT & uHash) {

	Uns32T pointULSHVectors[kLength];
	Uns32T h1;
	Uns32T h2;
	int pos1 = 0;
	int pos2 = 0;

	RealT accValue	= 0;
	int val=0;

		for (int l=0; l<lLength; l++) {
			pos1=l*kLength;
			for(int i = 0; i < kLength; i++){
				//compute hashes h(v)= (ax+b)/w;
				pointULSHVectors[i] = (Uns32T)(FLOOR_INT32((aTimesXDelta(pos1+i,0)/rValue + lshR0Functions[l][i].b) /W ));
			}

			h1 = computeProductModDefaultPrime(uHash->mainHashA, pointULSHVectors, kLength);
			h2 = computeProductModDefaultPrime(uHash->controlHash1, pointULSHVectors, kLength);


			if (h1 > UH_PRIME_DEFAULT){
			     h1 = h1 - UH_PRIME_DEFAULT;
			}
			h1= h1%uHash->hashTableSize;
			if (h2 > UH_PRIME_DEFAULT){
			       h2 = h2 - UH_PRIME_DEFAULT;
			}
			h1byL[l]=h1;
			h2byL[l]=h2;
		}
}
/**
 *  Computing buckets, h1 and h2 indexs for a L lenght and  a iPerturbation of the query
 *  using precomputed vectors
 *  returns the values in h1yL and h2byL
 */
void QueryHashFunctions::getIndex(int W, RealT rValue, Uns32T *h1byL, Uns32T *h2byL,LSHFunctionT **lshR0Functions,PUHashStructureT & uHash, int iPerturbation) {

	Uns32T pointULSHVectors[kLength];
	Uns32T h1;
	Uns32T h2;
	int pos1 = 0;
	int pos2 = 0;

	RealT accValue		= 0;


	for ( int l = 0; l< lLength; l++) {

		pos1 = l*kLength;
		//pos2 = iPerturbation*lLength*kLength+ l*kLength;
		for(int i = 0; i < kLength; i++) {
			//compute hashes g(v)= (sum(ax)+b)/w;
			// h(v)= floor(( aTimesX/R + aTimesDelta  +b)/w);
			pointULSHVectors[i] = (Uns32T)(FLOOR_INT32(((aTimesXDelta(pos1+i,0)/rValue)+aTimesXDelta(pos1+i,iPerturbation+1)+lshR0Functions[l][i].b ) / W ));
				}
		//sum(r'*a)mod prime
		h1 =computeProductModDefaultPrime(uHash->mainHashA, pointULSHVectors, kLength);
		h2= computeProductModDefaultPrime(uHash->controlHash1, pointULSHVectors, kLength);

		if (h1 > UH_PRIME_DEFAULT){
		      h1 = h1 - UH_PRIME_DEFAULT;
		}
		h1= h1% uHash->hashTableSize;

		if (h2 > UH_PRIME_DEFAULT){
		      h2 = h2 - UH_PRIME_DEFAULT;
		}

		h1byL[l]=h1;
		h2byL[l]=h2;
	}
}
/**
 * compute sum(r*a) for getting h index
 * h(a1, a2,.. ak)= (sum(r'*a mod prime) mod prime)
 *  by  applying h-(2^32-5 instead of mod prime
 *  See manual E2LSH code page 13
 */
inline Uns32T QueryHashFunctions::computeProductModDefaultPrime(Uns32T *a, Uns32T *b, IntT size){

	LongUns64T h = 0;
	IntT i = 0;
	for(IntT i = 0; i < size; i++){
		h = h + (LongUns64T)a[i] * (LongUns64T)b[i];
		h = (h & TWO_TO_32_MINUS_1) + 5 * (h >> 32);
		if (h >= UH_PRIME_DEFAULT) {
			h = h - UH_PRIME_DEFAULT;
		}
    CR_ASSERT(h < UH_PRIME_DEFAULT);
  }
  return h;
}


