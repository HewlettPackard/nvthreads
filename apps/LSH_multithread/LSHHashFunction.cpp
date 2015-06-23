/*
 * LSHHashFunction.cpp
 *
 *  Created on: Jan 7, 2014
 *      Author: kimmij
 */

#include <iostream>
#include "LSHHashFunction.h"
#include "headers.h"

#include <blaze/math/DynamicMatrix.h>
#include <blaze/math/DenseRow.h>
#include <blaze/math/DynamicVector.h>

using blaze::DynamicMatrix;
using blaze::DynamicVector;
using blaze::rowVector;

using namespace std;

LSH_HashFunction::LSH_HashFunction() {

}

LSH_HashFunction::~LSH_HashFunction() {
	// TODO Auto-generated destructor stub
}

void LSH_HashFunction::init(int Rindex, RealT paramR, RealT paramR2, int useUfunctions, int paramK, int paramL, int paramM, int paramT, int dimension, RealT paramW, int nPoints) {
	PRNearNeighborStructT nnStruct = initPRNearNeighborStructT(paramR, paramR2, useUfunctions, paramK, paramL, paramM, paramT, dimension, paramW);
	PUHashStructureT uhash = newUHash(HT_LINKED_LIST, nPoints, nnStruct->parameterK, FALSE);
	if(vec_nnStruct.size() > Rindex) {
		vec_nnStruct[Rindex] = nnStruct;
		vec_uhash[Rindex] = uhash;
		Rlist.push_back(paramR);
		//Rlist[Rindex] = paramR;
	} else {
		vec_nnStruct.push_back(nnStruct);
		vec_uhash.push_back(uhash);
		Rlist.push_back(paramR);
	}
	// vector a is created for hash computation using matrix multiplication
        DynamicMatrix<RealT> a(nnStruct->nHFTuples*nnStruct->hfTuplesLength,nnStruct->dimension);
        for(IntT i = 0; i < nnStruct->nHFTuples; i++){
                for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
                        DynamicVector<RealT,rowVector> tmp_a(nnStruct->dimension, nnStruct->lshFunctions[i][j].a);
                        row(a, i*nnStruct->hfTuplesLength+j) = tmp_a;
                }
        }
	if(vec_nnStruct.size() > Rindex) {
        	vec_a[Rindex] = a;
	} else {
        	vec_a.push_back(a);
	}
}
void LSH_HashFunction::initLSHHashFunctions(int R_num) {
	for(int i=0;i<R_num;i++) {
		PRNearNeighborStructT nnStruct;
		PUHashStructureT uhash;
		vec_nnStruct.push_back(nnStruct);
		vec_uhash.push_back(uhash);
		DynamicMatrix<RealT> a;
		vec_a.push_back(a);
	}
}
void LSH_HashFunction::setLSHHash(int Rindex, PRNearNeighborStructT pLSH, PUHashStructureT pHash) {
	vec_nnStruct[Rindex] = pLSH;
	vec_uhash[Rindex] = pHash;
	Rlist.push_back(pLSH->parameterR);
	// vector a is created for hash computation using matrix multiplication
	DynamicMatrix<RealT> a(pLSH->nHFTuples*pLSH->hfTuplesLength,pLSH->dimension);
        for(IntT i = 0; i < pLSH->nHFTuples; i++){
                for(IntT j = 0; j < pLSH->hfTuplesLength; j++){
                        DynamicVector<RealT,rowVector> tmp_a(pLSH->dimension, pLSH->lshFunctions[i][j].a);
                        row(a, i*pLSH->hfTuplesLength+j) = tmp_a;
                }
        }
        vec_a[Rindex] = a;
}

bool LSH_HashFunction::deserialize(string lsh_hashfile) {

	FILE *pFile;
	pFile = fopen (lsh_hashfile.c_str() , "r");

	if(fread (&(nPoints) , sizeof(int) , 1, pFile) <= 0) return false;
	nPoints = 688390;
	int rcount;
	if(fread (&(rcount) , sizeof(int) , 1, pFile) <= 0) return false;
	rcount = 17;
	for(int i=0;i<rcount;i++) {
		int sz;
		if(fread (&(sz) , sizeof(int) , 1, pFile) <= 0) return false;
		sz = 387336;
		void* data = malloc(sz);
		if(fread (data, 1, sz, pFile) <= 0) return false;
		PRNearNeighborStructT nnStruct = deserializePRNearNeighborStructT(data);
		vec_nnStruct.push_back(nnStruct);

		free(data);

		PUHashStructureT uhash = newUHash(HT_LINKED_LIST, nPoints, nnStruct->parameterK, TRUE);
		if(fread (&(sz) , sizeof(int) , 1, pFile) <= 0) return false;
		sz = 152;
		data = malloc(sz);
		if(fread (data, 1, sz, pFile) <= 0) return false;
		uhash = deserializePUHashStructureT(uhash, data);
		vec_uhash.push_back(uhash);

		free(data);
	}
	return true;
}

time_t LSH_HashFunction::getIndex(int Rindex, pair<int,int> Lindex, RealT* points, int dimension, Uns32T* hindex) {

	Uns32T h1[Lindex.second-Lindex.first+1];
	Uns32T h2[Lindex.second-Lindex.first+1];
	//Uns32T h1[104];
	//Uns32T h2[104];

	//printf("LSH_HashFunction::getIndex [%d][%d]\n", Lindex.first, Lindex.second);
	time_t t = getHashIndex_rangeL(Lindex.first, Lindex.second, h1,  h2, vec_nnStruct[Rindex], vec_uhash[Rindex], points, dimension);
	//getHashIndex(h1,  h2, vec_nnStruct[Rindex], vec_uhash[Rindex], points, dimension);

	//printf("LSH_HashFunction::getIndex #1\n");

	for(int i=0;i<Lindex.second-Lindex.first+1;i++) {
		Uns32T prime = 31;
	    Uns32T hash = 1;
	    hash = prime * hash + h1[i];
	    hash = prime * hash + h2[i];
		hindex[i] = hash;
	}
	return t;
	//printf("LSH_HashFunction::getIndex #2\n");
}

void LSH_HashFunction::getIndex(int Rindex, pair<int,int> Lindex, PPointT points, int dimension, Uns32T* hindex) {

	Uns32T h1[Lindex.second-Lindex.first+1];
	Uns32T h2[Lindex.second-Lindex.first+1];
	//Uns32T h1[104];
	//Uns32T h2[104];

	//printf("LSH_HashFunction::getIndex [%d][%d]\n", Lindex.first, Lindex.second);
	getHashIndex_rangeL(Lindex.first, Lindex.second, h1,  h2, vec_nnStruct[Rindex], vec_uhash[Rindex], points, dimension);
	//getHashIndex(h1,  h2, vec_nnStruct[Rindex], vec_uhash[Rindex], points, dimension);

	//printf("LSH_HashFunction::getIndex #1\n");

	for(int i=0;i<Lindex.second-Lindex.first+1;i++) {
		Uns32T prime = 31;
	    Uns32T hash = 1;
	    hash = prime * hash + h1[i];
	    hash = prime * hash + h2[i];
		hindex[i] = hash;
	}

	//printf("LSH_HashFunction::getIndex #2\n");
}
/**
 * Get index funtion using E2LSH code
 * Add by Mijung Kim
 */
time_t LSH_HashFunction::getIndex(int Rindex, pair<int,int> Lindex, RealT* points, int dimension, vector<pair<Uns32T,Uns32T> >& vec_hindex) {

	Uns32T h1[Lindex.second-Lindex.first+1];
	Uns32T h2[Lindex.second-Lindex.first+1];
	//Uns32T h1[104];
	//Uns32T h2[104];

	RealT pointsByR[dimension];
	RealT rValue= getR(Rindex);

	//Dividing by R value in order to support precomputation method.
	//done by Tere April 2014
	for(IntT d = 0; d < dimension; d++) {
		pointsByR[d] = points[d] /rValue;
	}

	//printf("LSH_HashFunction::getIndex [%d][%d]\n", Lindex.first, Lindex.second);
	//time_t t = getHashIndex_rangeL(Lindex.first, Lindex.second, h1,  h2, vec_nnStruct[Rindex], vec_uhash[Rindex], points, dimension);

	time_t t = getHashIndex_rangeL(Lindex.first, Lindex.second, h1,  h2, vec_nnStruct[Rindex], vec_uhash[Rindex], pointsByR, dimension);

	//getHashIndex(h1,  h2, vec_nnStruct[Rindex], vec_uhash[Rindex], points, dimension);

	//printf("LSH_HashFunction::getIndex #1\n");

	for(int i=0;i<Lindex.second-Lindex.first+1;i++) {
	//for(int i=0;i<Lindex.second+1;i++) {
		vec_hindex.push_back(make_pair(h1[i],h2[i]));
		//printf("vec_hindex.push_back hIndex[%d]=%d, control1[%d]=%d\n", i, h1[i], i, h2[i]);

	}
	return t;
	//printf("LSH_HashFunction::getIndex #2\n");
}

/**
 *GetIndexMethod that use only R0 RandomNumbers
 *in order to use precomputatioon of query Hashes
 *Parameters rValue, instead of rIndex
 */
time_t LSH_HashFunction::getIndexR0(RealT rValue, pair<int,int> Lindex, RealT* points, int dimension, vector<pair<Uns32T,Uns32T> >& vec_hindex) {

	RealT  pointsByR[dimension];
	Uns32T h1[Lindex.second-Lindex.first+1];
	Uns32T h2[Lindex.second-Lindex.first+1];
	Uns32T prointProjections[vec_nnStruct[0]->hfTuplesLength];

	Uns32T h1Val=0;
	Uns32T h2Val=0;
	time_t t;



	for(IntT d = 0; d < dimension; d++) {
		pointsByR[d] = points[d] /rValue;
	}


	for(IntT lValue =Lindex.first, j=0; lValue<Lindex.second+1; lValue++, j++) {

		computeLSH(vec_nnStruct[0],lValue,pointsByR,prointProjections);

		h1Val = computeProductModDefaultPrime(vec_uhash[0]->mainHashA, prointProjections, vec_nnStruct[0]->hfTuplesLength);
		h2Val = computeProductModDefaultPrime(vec_uhash[0]->controlHash1, prointProjections, vec_nnStruct[0]->hfTuplesLength);

		combineH1H2(h1Val,h2Val,vec_uhash[0]->hashTableSize);

		h1[j] = h1Val;
		h2[j] = h2Val;
	}

	for(int i=0;i<Lindex.second-Lindex.first+1;i++) {
		vec_hindex.push_back(make_pair(h1[i],h2[i]));
	}

	return t;
}

/**
 * Compute LSH Hashes h(a,b)= floor((sum(a*x) +b)/W)
 * for the points in points
 * This code is based on LocalitySensitiveHashing.cpp line 1210
 * Retrieve the values in vector Value
 */
void LSH_HashFunction::computeLSH(PRNearNeighborStructT nnStruct, IntT lIndex, RealT *point, Uns32T *vectorValue) {

	RealT value;

	for(int kIndex = 0; kIndex < nnStruct->hfTuplesLength; kIndex++) {
	   value = 0;

	   for(IntT d = 0; d < nnStruct->dimension; d++){
	         value += point[d] * nnStruct->lshFunctions[lIndex][kIndex].a[d];
	       }

	    vectorValue[kIndex] = (Uns32T)(FLOOR_INT32((value + nnStruct->lshFunctions[lIndex][kIndex].b) / nnStruct->parameterW));
	 }
}
/*
 * Compute h(a1, a2,.. ak)= sum(r'*a mod prime) formula
 * replacing the mod prime for shifting the h
 * if alpha <2^32-5 then alpha
 * else alpha >=2^32-5 then a- (2^32-5)
 * See manual E2LSH code page 13
 * Note this code has been extracted from e2lsh code
 *  Added by Tere Gonzalez
 *  Added on April 22, 2014
 */
Uns32T LSH_HashFunction::computeProductModDefaultPrime(Uns32T *a, Uns32T *b, IntT size) {

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
/**
 * Compute second mod in the formula
 * h(a1, a2,.. ak)= (sum(r'*a mod prime) mod prime)
 *  by  applying h-(2^32-5 instead of mod prime
 *  See manual E2LSH code page 13
 *  Note this code has been extracted from e2lsh code
 *  Added by Tere Gonzalez
 *  Added on April 22, 2014
 */
void 	LSH_HashFunction::combineH1H2(Uns32T &h1, Uns32T &h2, Int32T hashTableSize) {
	if (h1 > UH_PRIME_DEFAULT){
		      h1 = h1 - UH_PRIME_DEFAULT;
		}
		h1= h1% hashTableSize;

		if (h2 > UH_PRIME_DEFAULT){
		      h2 = h2 - UH_PRIME_DEFAULT;
		}
}

