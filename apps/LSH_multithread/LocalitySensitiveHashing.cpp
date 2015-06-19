/*
 * Copyright (c) 2004-2005 Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * MIT grants permission to use, copy, modify, and distribute this software and
 * its documentation for NON-COMMERCIAL purposes and without fee, provided that
 * this copyright notice appears in all copies.
 *
 * MIT provides this software "as is," without representations or warranties of
 * any kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability, fitness for a particular purpose, and
 * noninfringement.  MIT shall not be liable for any damages arising from any
 * use of this software.
 *
 * Author: Alexandr Andoni (andoni@mit.edu), Piotr Indyk (indyk@mit.edu)
 */

/*
  The main functionality of the LSH scheme is in this file (all except
  the hashing of the buckets). This file includes all the functions
  for processing a PRNearNeighborStructT data structure, which is the
  main R-NN data structure based on LSH scheme. The particular
  functions are: initializing a DS, adding new points to the DS, and
  responding to queries on the DS.
 */
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "limits.h"
#include "headers.h"
#include <sys/time.h>
#include <iostream>


void printRNNParameters(FILE *output, RNNParametersT parameters){
  ASSERT(output != NULL);
  fprintf(output, "R\n");
  fprintf(output, "%0.9lf\n", parameters.parameterR);
  fprintf(output, "Success probability\n");
  fprintf(output, "%0.9lf\n", parameters.successProbability);
  fprintf(output, "Dimension\n");
  fprintf(output, "%d\n", parameters.dimension);
  fprintf(output, "R^2\n");
  fprintf(output, "%0.9lf\n", parameters.parameterR2);
  fprintf(output, "Use <u> functions\n");
  fprintf(output, "%d\n", parameters.useUfunctions);
  fprintf(output, "k\n");
  fprintf(output, "%d\n", parameters.parameterK);
  fprintf(output, "m [# independent tuples of LSH functions]\n");
  fprintf(output, "%d\n", parameters.parameterM);
  fprintf(output, "L\n");
  fprintf(output, "%d\n", parameters.parameterL);
  fprintf(output, "W\n");
  fprintf(output, "%0.9lf\n", parameters.parameterW);
  fprintf(output, "T\n");
  fprintf(output, "%d\n", parameters.parameterT);
  fprintf(output, "typeHT\n");
  fprintf(output, "%d\n", parameters.typeHT);
}

RNNParametersT readRNNParameters(FILE *input){
  ASSERT(input != NULL);
  RNNParametersT parameters;
  char s[1000];// TODO: possible buffer overflow

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  FSCANF_REAL(input, &parameters.parameterR);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  FSCANF_REAL(input, &parameters.successProbability);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.dimension);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  FSCANF_REAL(input, &parameters.parameterR2);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.useUfunctions);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.parameterK);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.parameterM);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.parameterL);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  FSCANF_REAL(input, &parameters.parameterW);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.parameterT);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.typeHT);

  return parameters;
}

// Creates the LSH hash functions for the R-near neighbor structure
// <nnStruct>. The functions fills in the corresponding field of
// <nnStruct>.
void initHashFunctions(PRNearNeighborStructT nnStruct){
  ASSERT(nnStruct != NULL);
  LSHFunctionT **lshFunctions;
  // allocate memory for the functions
  printf("initHashfunctions nnStruct=[%x]\n", nnStruct);
  printf("initHashfunctions nnStruct->nHFTuples=[%d], nnStruct->hfTuplesLength=[%d], size(LSHfunctionT*)=[%d]\n", nnStruct->nHFTuples, nnStruct->hfTuplesLength, sizeof(LSHFunctionT*));
  fflush(stdout);
  FAILIF(NULL == (lshFunctions = (LSHFunctionT**)MALLOC(nnStruct->nHFTuples * sizeof(LSHFunctionT*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (lshFunctions[i] = (LSHFunctionT*)MALLOC(nnStruct->hfTuplesLength * sizeof(LSHFunctionT))));
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      FAILIF(NULL == (lshFunctions[i][j].a = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));
    }
  }

  // initialize the LSH functions
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      // vector a
      for(IntT d = 0; d < nnStruct->dimension; d++){
#ifdef USE_L1_DISTANCE
	lshFunctions[i][j].a[d] = genCauchyRandom();
#else
	lshFunctions[i][j].a[d] = genGaussianRandom();
#endif
      }
      // b
      lshFunctions[i][j].b = genUniformRandom(0, nnStruct->parameterW);
     }
  }


  nnStruct->lshFunctions = lshFunctions;
/*
  for(IntT i = 0; i < 1; i++){
  	    for(IntT j = 0; j < 1; j++){
  	      // vector a
  	      for(IntT d = 0; d < 1; d++){
  	      printf("initHashFunctions lshFunctions[%d][%d].a[%d]=%f\n", i, j, d, nnStruct->lshFunctions[i][j].a[d]);
  	    }
  	      printf("initHashFunctions lshFunctions[%d][%d].b=%f\n", i, j, nnStruct->lshFunctions[i][j].b);
  	  }
  	  }
   fflush(stdout);
   */
}

PRNearNeighborStructT initPRNearNeighborStructT(double parameterR, double parameterR2, int useUfunctions, int parameterK, int parameterL, int parameterM, int parameterT, int dimension, double parameterW ){

	PRNearNeighborStructT nnStruct;
	FAILIF(NULL == (nnStruct = (PRNearNeighborStructT)MALLOC(sizeof(RNearNeighborStructT))));

	/*
	nnStruct->parameterR = 0.53;
	//nnStruct->successProbability = 0.9;
	nnStruct->dimension = 784;
	nnStruct->parameterR2= 0.280899972;
	nnStruct->useUfunctions = 1;
	nnStruct->parameterK = 20; // parameter K of the algorithm.
	//nnStruct->parameterM = 35;
	nnStruct->parameterL = 1; //595; // parameter L of the algorithm.
	nnStruct->parameterW = 4.000000000; // parameter W of the algorithm.
	nnStruct->parameterT = 9991; // parameter T of the algorithm.

	if (!nnStruct->useUfunctions) {
		// Use normal <g> functions.
		nnStruct->nHFTuples = nnStruct->parameterL;
		nnStruct->hfTuplesLength = nnStruct->parameterK;
	}else{
		// Use <u> hash functions; a <g> function is a pair of 2 <u> functions.
		nnStruct->nHFTuples = 35;
		nnStruct->hfTuplesLength = nnStruct->parameterK / 2;
	}
	*/


	nnStruct->parameterR = parameterR;
	nnStruct->parameterR2 = parameterR2;
	nnStruct->useUfunctions = useUfunctions;
	nnStruct->parameterK = parameterK;
	if (!useUfunctions) {
		// Use normal <g> functions.
		nnStruct->parameterL = parameterL;
		nnStruct->nHFTuples = parameterL;
		nnStruct->hfTuplesLength = parameterK;
	}else{
		// Use <u> hash functions; a <g> function is a pair of 2 <u> functions.
		nnStruct->parameterL = parameterL;
		nnStruct->nHFTuples = parameterM;
		nnStruct->hfTuplesLength = parameterK / 2;
	}
	nnStruct->parameterT = parameterT;
	nnStruct->dimension = dimension;
	nnStruct->parameterW = parameterW;

    //nnStruct->typeHT = 3;
	printf("initPRNearNeighborStructT nnStruct=[%x]\n", nnStruct);
	  printf("initPRNearNeighborStructT nnStruct->nHFTuples=[%d]\n", nnStruct->nHFTuples);
	  fflush(stdout);
  // create the hash functions
  initHashFunctions(nnStruct);

  // init fields that are used only in operations ("temporary" variables for operations).

  // init the vector <pointULSHVectors> and the vector
  // <precomputedHashesOfULSHs>
  FAILIF(NULL == (nnStruct->pointULSHVectors = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (nnStruct->pointULSHVectors[i] = (Uns32T*)MALLOC(nnStruct->hfTuplesLength * sizeof(Uns32T))));
  }
  FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs[i] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
  }
  // init the vector <reducedPoint>
  FAILIF(NULL == (nnStruct->reducedPoint = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));

  return nnStruct;
}

// Initializes the fields of a R-near neighbors data structure except
// the hash tables for storing the buckets.
PRNearNeighborStructT initializePRNearNeighborFields(RNNParametersT algParameters, Int32T nPointsEstimate){
  PRNearNeighborStructT nnStruct;
  FAILIF(NULL == (nnStruct = (PRNearNeighborStructT)MALLOC(sizeof(RNearNeighborStructT))));
  nnStruct->parameterR = algParameters.parameterR;
  nnStruct->parameterR2 = algParameters.parameterR2;
  nnStruct->useUfunctions = algParameters.useUfunctions;
  nnStruct->parameterK = algParameters.parameterK;
  if (!algParameters.useUfunctions) {
    // Use normal <g> functions.
    nnStruct->parameterL = algParameters.parameterL;
    nnStruct->nHFTuples = algParameters.parameterL;
    nnStruct->hfTuplesLength = algParameters.parameterK;
  }else{
    // Use <u> hash functions; a <g> function is a pair of 2 <u> functions.
    nnStruct->parameterL = algParameters.parameterL;
    nnStruct->nHFTuples = algParameters.parameterM;
    nnStruct->hfTuplesLength = algParameters.parameterK / 2;
  }
  nnStruct->parameterT = algParameters.parameterT;
  nnStruct->dimension = algParameters.dimension;
  nnStruct->parameterW = algParameters.parameterW;

  nnStruct->nPoints = 0;
  nnStruct->pointsArraySize = nPointsEstimate;

  FAILIF(NULL == (nnStruct->points = (PPointT*)MALLOC(nnStruct->pointsArraySize * sizeof(PPointT))));

  // create the hash functions
  initHashFunctions(nnStruct);

  // init fields that are used only in operations ("temporary" variables for operations).

  // init the vector <pointULSHVectors> and the vector
  // <precomputedHashesOfULSHs>
  FAILIF(NULL == (nnStruct->pointULSHVectors = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (nnStruct->pointULSHVectors[i] = (Uns32T*)MALLOC(nnStruct->hfTuplesLength * sizeof(Uns32T))));
  }
  FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs[i] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
  }
  // init the vector <reducedPoint>
  FAILIF(NULL == (nnStruct->reducedPoint = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));
  // init the vector <nearPoints>
  nnStruct->sizeMarkedPoints = nPointsEstimate;
  FAILIF(NULL == (nnStruct->markedPoints = (BooleanT*)MALLOC(nnStruct->sizeMarkedPoints * sizeof(BooleanT))));
  for(IntT i = 0; i < nnStruct->sizeMarkedPoints; i++){
    nnStruct->markedPoints[i] = FALSE;
  }
  // init the vector <nearPointsIndeces>
  FAILIF(NULL == (nnStruct->markedPointsIndeces = (Int32T*)MALLOC(nnStruct->sizeMarkedPoints * sizeof(Int32T))));

  nnStruct->reportingResult = TRUE;

  return nnStruct;
}

// Constructs a new empty R-near-neighbor data structure.
PRNearNeighborStructT initLSH(RNNParametersT algParameters, Int32T nPointsEstimate){
  ASSERT(algParameters.typeHT == HT_LINKED_LIST || algParameters.typeHT == HT_STATISTICS);
  PRNearNeighborStructT nnStruct = initializePRNearNeighborFields(algParameters, nPointsEstimate);

  // initialize second level hashing (bucket hashing)
  FAILIF(NULL == (nnStruct->hashedBuckets = (PUHashStructureT*)MALLOC(nnStruct->parameterL * sizeof(PUHashStructureT))));
  Uns32T *mainHashA = NULL, *controlHash1 = NULL;
  BooleanT uhashesComputedAlready = FALSE;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPointsEstimate, nnStruct->parameterK, uhashesComputedAlready, mainHashA, controlHash1, NULL);
    uhashesComputedAlready = TRUE;
  }

  return nnStruct;
}

/*
PRNearNeighborStructT initLSH(PRNearNeighborStructT nnStruct, Int32T nPointsEstimate){

  // initialize second level hashing (bucket hashing)
  FAILIF(NULL == (nnStruct->hashedBuckets = (PUHashStructureT*)MALLOC(nnStruct->parameterL * sizeof(PUHashStructureT))));
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    nnStruct->hashedBuckets[i] = newUHash(HT_LINKED_LIST, nPointsEstimate, nnStruct->parameterK, FALSE);
  }

  return nnStruct;
}
*/

void preparePointAdding(PRNearNeighborStructT nnStruct, PUHashStructureT uhash, PPointT point);
void preparePointAdding_rangeL(Uns32T Lindex_start, Uns32T Lindex_end, PRNearNeighborStructT nnStruct, PUHashStructureT uhash, PPointT point);

/*
// ADDED BY MIJUNG
// return Hash vectors for given a point (darr)
// this function is NOT USED for now and created for a later use
void getHashVector(PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, double* darr, int pointsDimension) {
	PPointT point;
	FAILIF(NULL == (point = (PPointT)MALLOC(sizeof(PointT))));
	FAILIF(NULL == (point->coordinates = (RealT*)MALLOC(pointsDimension * sizeof(RealT))));

	RealT sqrLength = 0;
	// read in the query point.
	for(IntT d = 0; d < pointsDimension; d++){
	  point->coordinates[d] = darr[d];
	  sqrLength += SQR(point->coordinates[d]);
	}
	point->sqrLength = sqrLength;

  //ASSERT(algParameters.typeHT == HT_HYBRID_CHAINS);
  //ASSERT(dataSet != NULL);
  ASSERT(USE_SAME_UHASH_FUNCTIONS);


	for(IntT d = 0; d < nnStruct->dimension; d++){
	  nnStruct->reducedPoint[d] = point->coordinates[d] / nnStruct->parameterR;
	  //printf("p[%d]=[%f]\n", d, nnStruct->reducedPoint[d]);
	}
	//fflush(stdout);

	// Compute all ULSH functions.
	for(IntT i = 0; i < nnStruct->nHFTuples; i++){
	  computeULSH(nnStruct, i, nnStruct->reducedPoint, nnStruct->pointULSHVectors[i]);
	  //printf("preparePointAdding nnStruct->pointULSHVectors[%d][0]=%u\n", i, nnStruct->pointULSHVectors[i][0]);
	  //printf("preparePointAdding nnStruct->pointULSHVectors[%d][1]=%u\n", i, nnStruct->pointULSHVectors[i][1]);
	}
}
*/


// ADDED BY MIJUNG
// n choose k
ULONG binomial(ULONG n, ULONG k)
{
	ULONG r = 1, d = n - k;

	/* choose the smaller of k and n - k */
	if (d > k) { k = d; d = n - k; }

	while (n > k) {
		if (r >= UINT_MAX / n) return 0; /* overflown */
		r *= n--;

		/* divide (n - k)! as soon as we can to delay overflows */
		while (d > 1 && !(r % d)) r /= d--;
	}
	return r;
}
// enumerate all combinations of n choose k
template <typename Iterator>
inline bool next_combination(const Iterator first, Iterator k, const Iterator last)
{
   if ((first == last) || (first == k) || (last == k))
      return false;
   Iterator itr1 = first;
   Iterator itr2 = last;
   ++itr1;
   if (last == itr1)
      return false;
   itr1 = last;
   --itr1;
   itr1 = k;
   --itr2;
   while (first != itr1)
   {
      if (*--itr1 < *itr2)
      {
         Iterator j = k;
         while (!(*itr1 < *j)) ++j;
         std::iter_swap(itr1,j);
         ++itr1;
         ++j;
         itr2 = k;
         std::rotate(itr1,j,last);
         while (last != j)
         {
            ++j;
            ++itr2;
         }
         std::rotate(k,itr2,last);
         return true;
      }
   }
   std::rotate(first,k,last);
   return false;
}
// ADDED BY MIJUNG
// given a point, create perturbation vectors and return h1 and h2 indices for each perturbation vector
void getPerturbationIndices(Uns32T* hIndex, Uns32T* control1, PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, double* darr, int pointsDimension, int npert) {
	PPointT point;
	FAILIF(NULL == (point = (PPointT)MALLOC(sizeof(PointT))));
	FAILIF(NULL == (point->coordinates = (RealT*)MALLOC(pointsDimension * sizeof(RealT))));

	RealT sqrLength = 0;
	// read in the query point.
	for(IntT d = 0; d < pointsDimension; d++){
	  point->coordinates[d] = darr[d];
	  sqrLength += SQR(point->coordinates[d]);
	}
	point->sqrLength = sqrLength;

  //ASSERT(algParameters.typeHT == HT_HYBRID_CHAINS);
  //ASSERT(dataSet != NULL);

	ASSERT(USE_SAME_UHASH_FUNCTIONS);

/*
	for(IntT d = 0; d < nnStruct->dimension; d++){
	  nnStruct->reducedPoint[d] = point->coordinates[d] / nnStruct->parameterR;
	  printf("p[%d]=[%f]\n", d, nnStruct->reducedPoint[d]);
	}
	fflush(stdout);
	*/

	// maximum count of perturbation
	int npert_max = nnStruct->hfTuplesLength;
	// k choose (# of pert)
	int nchoosek = binomial(npert_max, npert);

	std::pair<int,RealT> proj_values[nnStruct->nHFTuples][nnStruct->hfTuplesLength]; // projection vector

	Uns32T* H_values[nnStruct->nHFTuples][nchoosek];  // perturbation vectors
	for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		for(IntT j = 0; j < nchoosek; j++){
			H_values[i][j] = (Uns32T*)(MALLOC(nnStruct->hfTuplesLength*sizeof(Uns32T)));  // perturbation vectors
		}
	}
	std::pair<int,RealT> sorted_proj_values[nnStruct->nHFTuples][nnStruct->hfTuplesLength]; // sorted projection vector

	for(IntT i = 0; i < nnStruct->nHFTuples; i++){
	  // compute LSH for query point
	  computeULSH(nnStruct, i, nnStruct->reducedPoint, nnStruct->pointULSHVectors[i]);

	  // compute projection vector
	  RealT* tmp_proj_value = (RealT*)MALLOC(nnStruct->hfTuplesLength*sizeof(RealT));

	  /* for test purpose
	  tmp_proj_value[0]=2.8445;
	  tmp_proj_value[1]=2.8467;
	  tmp_proj_value[2]=0.8085;
	  tmp_proj_value[3]=0.1436;
*/
	  computeProjection(nnStruct, i, nnStruct->reducedPoint, tmp_proj_value);

	  // projection vector of pair of value and index
	  for(int j=0;j<nnStruct->hfTuplesLength;j++) {
		  RealT  absval = tmp_proj_value[j] - (RealT)(nnStruct->parameterW)/2;
		  if(absval < 0)
			  absval *= -1;
		  sorted_proj_values[i][j] = std::make_pair (j,absval);
		  proj_values[i][j] = std::make_pair (j,tmp_proj_value[j]);


		  //printf("proj:%d,", sorted_proj_values[i][j].first);
		  //printf("%f\n", sorted_proj_values[i][j].second);
	  }
	  free(tmp_proj_value);


	  //fflush(stdout);


	  // sort the projection vector
	  qsort(sorted_proj_values[i], nnStruct->hfTuplesLength, sizeof(std::pair<int,RealT>), comparePairs);

	  /*
	  for(int j=0;j<nnStruct->hfTuplesLength;j++) {
	  		  printf("sorted %d,", sorted_proj_values[i][j].first);
	  		  printf("%f\n", sorted_proj_values[i][j].second);
	  		printf("proj %d,", proj_values[i][j].first);
	  			  		  printf("%f\n", proj_values[i][j].second);
	  	  }
	  fflush(stdout);
*/
/*
	  int npert_max = 0; // = sum(proj_values > 0);
	  for(IntT j=0;j<nnStruct->hfTuplesLength;j++) {
		  if(proj_values[i][j].second > 0)
			  npert_max++;
	  }

	  npert = std::min(npert, npert_max);
*/
	  // generate perturbation vectors
	  Uns32T Zprime[npert];
	  Uns32T n[npert_max];
	  for(IntT j=0;j<npert_max;j++) {
		  n[j] = j;
	  }
	  std::vector<int> V(n, n+npert_max);
	  int count1 = 0;
	  do {

		    //for(int j=0;j<npert;j++) {
			//	std::cout << V[j];
			//}
			//std::cout << std::endl;

			  for(IntT j=0;j<npert;j++) {
				  Zprime[j] = sorted_proj_values[i][V[j]].first;
			  }
/*
			  for(IntT j=0;j<npert;j++) {
			  		  printf("Zprime[%d]=%d\n", j, Zprime[j]);
			  	  }

			  fflush(stdout);
*/
			  /* for test purpose
			  nnStruct->pointULSHVectors[i][0] = 0;
			  nnStruct->pointULSHVectors[i][1] = 0;
			  nnStruct->pointULSHVectors[i][2] = 0;
			  nnStruct->pointULSHVectors[i][3] = 1;
*/
			  memcpy((Uns32T*) H_values[i][count1], nnStruct->pointULSHVectors[i], nnStruct->hfTuplesLength*sizeof(Uns32T));

			  for(IntT j = 0; j<npert; j++) {
				  //printf("proj_values[%d][%d].second=%f, H_values[%d][%d][%d]=%d\n", i, Zprime[j], proj_values[i][Zprime[j]].second, i, count1, Zprime[j], H_values[i][count1][Zprime[j]] );

				  if (proj_values[i][Zprime[j]].second > nnStruct->parameterW/2)
					  H_values[i][count1][Zprime[j]] = (H_values[i][count1][Zprime[j]]+1);
				  else
					  H_values[i][count1][Zprime[j]] = (H_values[i][count1][Zprime[j]]-1);

				  //printf("after H_values[%d][%d][%d]=%d\n", i, count1, Zprime[j], H_values[i][count1][Zprime[j]] );
				  //fflush(stdout);
			  }

			  count1=count1+1;

	  } while(next_combination(V.begin(),V.begin() + npert,V.end()));
	}

	/*
	for(IntT i = 0; i < nnStruct->parameterL; i++){
	for(IntT j = 0; j < nchoosek; j++) {
		for(IntT k = 0; k < nnStruct->hfTuplesLength; k++) {
			//printf("H_values[%d][%d][%d]=%d\n", i,j,k,H_values[i][j][k]);
			printf("%u ", H_values[i][j][k]);

		}
		printf("\n");

	}
	}
	*/




	// generate hash indices (h1, h2) for perturbation vectors, similar to h1,h2 generation for the query point (copied from "getHashIndex" function)
	int cnt = 0; // perturbation hash indices count
	for(IntT j = 0; j < nchoosek; j++) {
		if (USE_SAME_UHASH_FUNCTIONS) {
			for(IntT i = 0; i < nnStruct->nHFTuples; i++){
				/*
				for(IntT k = 0; k < nnStruct->hfTuplesLength; k++) {
				printf("after H_values[%d][%d][%d]=%u\n", i,j,k,H_values[i][j][k]);
				printf("nnStruct->pointULSHVectors[%d][%d][%d]=%u\n", i,j,k,nnStruct->pointULSHVectors[i][k]);

				}
				*/

	    		precomputeUHFsForULSH(modelHT, H_values[i][j], nnStruct->hfTuplesLength, nnStruct->precomputedHashesOfULSHs[i]);

			}
		}

	  IntT firstUComp = 0;
	  IntT secondUComp = 1;
	  for(IntT i = 0; i < nnStruct->parameterL; i++){
		// build the model HT.
		//for(IntT p = 0; p < nPoints; p++){
		  // Add point <dataSet[p]> to modelHT.
		  if (!nnStruct->useUfunctions) {
		// Use usual <g> functions (truly independent; <g>s are precisly
		// <u>s).
			  getBucketIndex(hIndex[cnt], control1[cnt], modelHT, 1, nnStruct->precomputedHashesOfULSHs[i], NULL);
		  } else {
		// Use <u> functions (<g>s are pairs of <u> functions).
			  getBucketIndex(hIndex[cnt], control1[cnt], modelHT, 2, nnStruct->precomputedHashesOfULSHs[firstUComp], nnStruct->precomputedHashesOfULSHs[secondUComp]);
		  }
		  printf("hIndex[%d]=%u, control1[%d]=%u\n", cnt, hIndex[cnt], cnt, control1[cnt]);

		  cnt++;
		  //fflush(stdout);
		//}

		//ASSERT(nAllocatedGBuckets <= nPoints);
		//ASSERT(nAllocatedBEntries <= nPoints);

		// compute what is the next pair of <u> functions.
		secondUComp++;
		if (secondUComp == nnStruct->nHFTuples) {
		  firstUComp++;
		  secondUComp = firstUComp + 1;
		}

		// copy the model HT into the actual (packed) HT. copy the uhash function too.
		//nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPoints, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);

	   }
	}


	free(point->coordinates);
	free(point);


	for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		for(IntT j = 0; j < nchoosek; j++){
			free(H_values[i][j]);
		}
	}



}


// ADDED BY MIJUNG
// Returns hIndex and control1 given the data point (darr)
void getHashIndex(Uns32T* hIndex, Uns32T* control1, PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, RealT* darr, int pointsDimension){

	PPointT point;
	FAILIF(NULL == (point = (PPointT)MALLOC(sizeof(PointT))));
	FAILIF(NULL == (point->coordinates = (RealT*)MALLOC(pointsDimension * sizeof(RealT))));

	RealT sqrLength = 0;
	// read in the query point.
	for(IntT d = 0; d < pointsDimension; d++){
	  point->coordinates[d] = darr[d];
	  sqrLength += SQR(point->coordinates[d]);
	}
	point->sqrLength = sqrLength;

  //ASSERT(algParameters.typeHT == HT_HYBRID_CHAINS);
  //ASSERT(dataSet != NULL);
  ASSERT(USE_SAME_UHASH_FUNCTIONS);

/*
  Uns32T *(precomputedHashesOfULSHs[nnStruct->nHFTuples]);
    for(IntT l = 0; l < nnStruct->nHFTuples; l++){
      FAILIF(NULL == (precomputedHashesOfULSHs[l] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
    }
*/
  preparePointAdding(nnStruct, modelHT, point);
/*
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    for(IntT h = 0; h < N_PRECOMPUTED_HASHES_NEEDED; h++){
	precomputedHashesOfULSHs[l][h] = nnStruct->precomputedHashesOfULSHs[l][h];
    }
  }
  */
  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    // build the model HT.
    //for(IntT p = 0; p < nPoints; p++){
      // Add point <dataSet[p]> to modelHT.
      if (!nnStruct->useUfunctions) {
	// Use usual <g> functions (truly independent; <g>s are precisly
	// <u>s).
    	  //printf("precomputedHashesOfULSHs[%u][0]\n", nnStruct->precomputedHashesOfULSHs[i][0]);
    	  //printf("precomputedHashesOfULSHs[%u][1]\n", nnStruct->precomputedHashesOfULSHs[i][1]);
    	  getBucketIndex(hIndex[i], control1[i], modelHT, 1, nnStruct->precomputedHashesOfULSHs[i], NULL);
      } else {
	// Use <u> functions (<g>s are pairs of <u> functions).
    	  getBucketIndex(hIndex[i], control1[i], modelHT, 2, nnStruct->precomputedHashesOfULSHs[firstUComp], nnStruct->precomputedHashesOfULSHs[secondUComp]);
      }
      fflush(stdout);
      //printf("hIndex[%d]=%u, control1[%d]=%u\n", i, hIndex[i], i, control1[i]);
      //fflush(stdout);
    //}

    //ASSERT(nAllocatedGBuckets <= nPoints);
    //ASSERT(nAllocatedBEntries <= nPoints);

    // compute what is the next pair of <u> functions.
    secondUComp++;
    if (secondUComp == nnStruct->nHFTuples) {
      firstUComp++;
      secondUComp = firstUComp + 1;
    }

    // copy the model HT into the actual (packed) HT. copy the uhash function too.
    //nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPoints, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);

   }


  	free(point->coordinates);
	free(point);
}

void getHashIndex_rangeL(Uns32T Lindex_start, Uns32T Lindex_end, Uns32T* hIndex, Uns32T* control1, PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, PPointT point, int pointsDimension){

  ASSERT(USE_SAME_UHASH_FUNCTIONS);


  preparePointAdding_rangeL(Lindex_start, Lindex_end, nnStruct, modelHT, point);

  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;
  for(IntT i = Lindex_start; i <= Lindex_end; i++){
    // build the model HT.
    //for(IntT p = 0; p < nPoints; p++){
      // Add point <dataSet[p]> to modelHT.
      if (!nnStruct->useUfunctions) {
	// Use usual <g> functions (truly independent; <g>s are precisly
	// <u>s).
    	  //printf("precomputedHashesOfULSHs[%u][0]\n", nnStruct->precomputedHashesOfULSHs[i][0]);
    	  //printf("precomputedHashesOfULSHs[%u][1]\n", nnStruct->precomputedHashesOfULSHs[i][1]);
    	  getBucketIndex(hIndex[i-Lindex_start], control1[i-Lindex_start], modelHT, 1, nnStruct->precomputedHashesOfULSHs[i], NULL);
      } else {
	// Use <u> functions (<g>s are pairs of <u> functions).
    	  getBucketIndex(hIndex[i-Lindex_start], control1[i-Lindex_start], modelHT, 2, nnStruct->precomputedHashesOfULSHs[firstUComp], nnStruct->precomputedHashesOfULSHs[secondUComp]);
      }
      //fflush(stdout);
      //printf("hIndex[%d]=%u, control1[%d]=%u\n", i, hIndex[i-Lindex_start], i, control1[i-Lindex_start]);
      //fflush(stdout);
    //}

    //ASSERT(nAllocatedGBuckets <= nPoints);
    //ASSERT(nAllocatedBEntries <= nPoints);

    // compute what is the next pair of <u> functions.
    secondUComp++;
    if (secondUComp == nnStruct->nHFTuples) {
      firstUComp++;
      secondUComp = firstUComp + 1;
    }

    // copy the model HT into the actual (packed) HT. copy the uhash function too.
    //nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPoints, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);

   }
}
// ADDED BY MIJUNG
// Returns hIndex and control1 given the data point (darr) for a range of L values
time_t getHashIndex_rangeL(Uns32T Lindex_start, Uns32T Lindex_end, Uns32T* hIndex, Uns32T* control1, PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, RealT* darr, int pointsDimension){

	timeval begin,end;
		time_t delta;

		gettimeofday(&begin, nullptr);
	PPointT point;
	FAILIF(NULL == (point = (PPointT)MALLOC(sizeof(PointT))));
	FAILIF(NULL == (point->coordinates = (RealT*)MALLOC(pointsDimension * sizeof(RealT))));

	RealT sqrLength = 0;
	// read in the query point.
	for(IntT d = 0; d < pointsDimension; d++){
	  point->coordinates[d] = darr[d];
	  sqrLength += SQR(point->coordinates[d]);
	}
	point->sqrLength = sqrLength;

  //ASSERT(algParameters.typeHT == HT_HYBRID_CHAINS);
  //ASSERT(dataSet != NULL);
  ASSERT(USE_SAME_UHASH_FUNCTIONS);

/*
  Uns32T *(precomputedHashesOfULSHs[nnStruct->nHFTuples]);
    for(IntT l = 0; l < nnStruct->nHFTuples; l++){
      FAILIF(NULL == (precomputedHashesOfULSHs[l] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
    }
*/
  gettimeofday(&end, nullptr);
   delta = (end.tv_sec - begin.tv_sec)*1000000 + (end.tv_usec - begin.tv_usec);

  preparePointAdding_rangeL(Lindex_start, Lindex_end, nnStruct, modelHT, point);
/*
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    for(IntT h = 0; h < N_PRECOMPUTED_HASHES_NEEDED; h++){
	precomputedHashesOfULSHs[l][h] = nnStruct->precomputedHashesOfULSHs[l][h];
    }
  }
  */

  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;
  for(IntT i = Lindex_start; i <= Lindex_end; i++){
    // build the model HT.
    //for(IntT p = 0; p < nPoints; p++){
      // Add point <dataSet[p]> to modelHT.
      if (!nnStruct->useUfunctions) {
	// Use usual <g> functions (truly independent; <g>s are precisly
	// <u>s).
    	  //printf("precomputedHashesOfULSHs[%u][0]\n", nnStruct->precomputedHashesOfULSHs[i][0]);
    	  //printf("precomputedHashesOfULSHs[%u][1]\n", nnStruct->precomputedHashesOfULSHs[i][1]);
    	  getBucketIndex(hIndex[i-Lindex_start], control1[i-Lindex_start], modelHT, 1, nnStruct->precomputedHashesOfULSHs[i], NULL);
      } else {
	// Use <u> functions (<g>s are pairs of <u> functions).
    	  getBucketIndex(hIndex[i-Lindex_start], control1[i-Lindex_start], modelHT, 2, nnStruct->precomputedHashesOfULSHs[firstUComp], nnStruct->precomputedHashesOfULSHs[secondUComp]);
      }
      //fflush(stdout);
      //printf("hIndex[%d]=%u, control1[%d]=%u\n", i, hIndex[i-Lindex_start], i, control1[i-Lindex_start]);
      //fflush(stdout);
    //}

    //ASSERT(nAllocatedGBuckets <= nPoints);
    //ASSERT(nAllocatedBEntries <= nPoints);

    // compute what is the next pair of <u> functions.
    secondUComp++;
    if (secondUComp == nnStruct->nHFTuples) {
      firstUComp++;
      secondUComp = firstUComp + 1;
    }

    // copy the model HT into the actual (packed) HT. copy the uhash function too.
    //nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPoints, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);

   }


  	free(point->coordinates);
	free(point);
	return delta;
}



// Returns hIndex and control1 given the data point (darr) for a given Lindex
void getHashIndex(Uns32T Lindex, Uns32T& hIndex, Uns32T& control1, PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, double* darr, int pointsDimension){




	PPointT point;
	FAILIF(NULL == (point = (PPointT)MALLOC(sizeof(PointT))));
	FAILIF(NULL == (point->coordinates = (RealT*)MALLOC(pointsDimension * sizeof(RealT))));

	RealT sqrLength = 0;
	// read in the query point.
	for(IntT d = 0; d < pointsDimension; d++){
	  point->coordinates[d] = darr[d];
	  //printf("darr[%d]=[%f]\n", d,darr[d]);
	  sqrLength += SQR(point->coordinates[d]);
	}
	//fflush(stdout);
	point->sqrLength = sqrLength;

  //ASSERT(algParameters.typeHT == HT_HYBRID_CHAINS);
  //ASSERT(dataSet != NULL);
  ASSERT(USE_SAME_UHASH_FUNCTIONS);

/*
  Uns32T *(precomputedHashesOfULSHs[nnStruct->nHFTuples]);
    for(IntT l = 0; l < nnStruct->nHFTuples; l++){
      FAILIF(NULL == (precomputedHashesOfULSHs[l] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
    }
*/
  preparePointAdding(nnStruct, modelHT, point);

  /*
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    for(IntT h = 0; h < N_PRECOMPUTED_HASHES_NEEDED; h++){
	precomputedHashesOfULSHs[l][h] = nnStruct->precomputedHashesOfULSHs[l][h];
    }
  }
  */

  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;
  IntT i = Lindex;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    // build the model HT.
    //for(IntT p = 0; p < nPoints; p++){
      // Add point <dataSet[p]> to modelHT.
      if (!nnStruct->useUfunctions) {
	// Use usual <g> functions (truly independent; <g>s are precisly
	// <u>s).
    	  //printf("precomputedHashesOfULSHs[%u][0]\n", nnStruct->precomputedHashesOfULSHs[i][0]);
    	  //printf("precomputedHashesOfULSHs[%u][1]\n", nnStruct->precomputedHashesOfULSHs[i][1]);
    	  getBucketIndex(hIndex, control1, modelHT, 1, nnStruct->precomputedHashesOfULSHs[i], NULL);
      } else {
	// Use <u> functions (<g>s are pairs of <u> functions).
    	  getBucketIndex(hIndex, control1, modelHT, 2, nnStruct->precomputedHashesOfULSHs[firstUComp], nnStruct->precomputedHashesOfULSHs[secondUComp]);
      }

      //printf("hIndex[%d]=%u, control1[%d]=%u\n", i, hIndex, i, control1);
      //fflush(stdout);
      //fflush(stdout);
    //}

    //ASSERT(nAllocatedGBuckets <= nPoints);
    //ASSERT(nAllocatedBEntries <= nPoints);

    // compute what is the next pair of <u> functions.
    secondUComp++;
    if (secondUComp == nnStruct->nHFTuples) {
      firstUComp++;
      secondUComp = firstUComp + 1;
    }

    // copy the model HT into the actual (packed) HT. copy the uhash function too.
    //nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPoints, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);
  	}


  	free(point->coordinates);
	free(point);


}

// Construct PRNearNeighborStructT given the data set <dataSet> (all
// the points <dataSet> will be contained in the resulting DS).
// Currenly only type HT_HYBRID_CHAINS is supported for this
// operation.
PRNearNeighborStructT initLSH_WithDataSet(RNNParametersT algParameters, Int32T nPoints, PPointT *dataSet){
  ASSERT(algParameters.typeHT == HT_HYBRID_CHAINS);
  ASSERT(dataSet != NULL);
  ASSERT(USE_SAME_UHASH_FUNCTIONS);

  PRNearNeighborStructT nnStruct = initializePRNearNeighborFields(algParameters, nPoints);

  // Set the fields <nPoints> and <points>.
  nnStruct->nPoints = nPoints;
  for(Int32T i = 0; i < nPoints; i++){
    nnStruct->points[i] = dataSet[i];
  }
  
  // initialize second level hashing (bucket hashing)
  FAILIF(NULL == (nnStruct->hashedBuckets = (PUHashStructureT*)MALLOC(nnStruct->parameterL * sizeof(PUHashStructureT))));
  Uns32T *mainHashA = NULL, *controlHash1 = NULL;
  PUHashStructureT modelHT = newUHashStructure(HT_LINKED_LIST, nPoints, nnStruct->parameterK, FALSE, mainHashA, controlHash1, NULL);
  
  Uns32T **(precomputedHashesOfULSHs[nnStruct->nHFTuples]);
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    FAILIF(NULL == (precomputedHashesOfULSHs[l] = (Uns32T**)MALLOC(nPoints * sizeof(Uns32T*))));
    for(IntT i = 0; i < nPoints; i++){
      FAILIF(NULL == (precomputedHashesOfULSHs[l][i] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
    }
  }

  for(IntT i = 0; i < nPoints; i++){
    preparePointAdding(nnStruct, modelHT, dataSet[i]);
    for(IntT l = 0; l < nnStruct->nHFTuples; l++){
      for(IntT h = 0; h < N_PRECOMPUTED_HASHES_NEEDED; h++){
	precomputedHashesOfULSHs[l][i][h] = nnStruct->precomputedHashesOfULSHs[l][h];
      }
    }
  }

  //DPRINTF("Allocated memory(modelHT and precomputedHashesOfULSHs just a.): %lld\n", totalAllocatedMemory);

  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    // build the model HT.
    for(IntT p = 0; p < nPoints; p++){
      // Add point <dataSet[p]> to modelHT.
      if (!nnStruct->useUfunctions) {
	// Use usual <g> functions (truly independent; <g>s are precisly
	// <u>s).
	addBucketEntry(modelHT, 1, precomputedHashesOfULSHs[i][p], NULL, p);
      } else {
	// Use <u> functions (<g>s are pairs of <u> functions).
	addBucketEntry(modelHT, 2, precomputedHashesOfULSHs[firstUComp][p], precomputedHashesOfULSHs[secondUComp][p], p);
      }
    }

    //ASSERT(nAllocatedGBuckets <= nPoints);
    //ASSERT(nAllocatedBEntries <= nPoints);

    // compute what is the next pair of <u> functions.
    secondUComp++;
    if (secondUComp == nnStruct->nHFTuples) {
      firstUComp++;
      secondUComp = firstUComp + 1;
    }

    // copy the model HT into the actual (packed) HT. copy the uhash function too.
    nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPoints, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);

    // clear the model HT for the next iteration.
    clearUHashStructure(modelHT);
  }

  freeUHashStructure(modelHT, FALSE); // do not free the uhash functions since they are used by nnStruct->hashedBuckets[i]

  // freeing precomputedHashesOfULSHs
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    for(IntT i = 0; i < nPoints; i++){
      FREE(precomputedHashesOfULSHs[l][i]);
    }
    FREE(precomputedHashesOfULSHs[l]);
  }

  return nnStruct;
}



// // Packed version (static).
// PRNearNeighborStructT buildPackedLSH(RealT R, BooleanT useUfunctions, IntT k, IntT LorM, RealT successProbability, IntT dim, IntT T, Int32T nPoints, PPointT *points){
//   ASSERT(points != NULL);
//   PRNearNeighborStructT nnStruct = initializePRNearNeighborFields(R, useUfunctions, k, LorM, successProbability, dim, T, nPoints);

//   // initialize second level hashing (bucket hashing)
//   FAILIF(NULL == (nnStruct->hashedBuckets = (PUHashStructureT*)MALLOC(nnStruct->parameterL * sizeof(PUHashStructureT))));
//   Uns32T *mainHashA = NULL, *controlHash1 = NULL;
//   PUHashStructureT modelHT = newUHashStructure(HT_STATISTICS, nPoints, nnStruct->parameterK, FALSE, mainHashA, controlHash1, NULL);
//   for(IntT i = 0; i < nnStruct->parameterL; i++){
//     // build the model HT.
//     for(IntT p = 0; p < nPoints; p++){
//       // addBucketEntry(modelHT, );
//     }



//     // copy the model HT into the actual (packed) HT.
//     nnStruct->hashedBuckets[i] = newUHashStructure(HT_PACKED, nPointsEstimate, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);

//     // clear the model HT for the next iteration.
//     clearUHashStructure(modelHT);
//   }

//   return nnStruct;
// }


// Optimizes the nnStruct (non-agressively, i.e., without changing the
// parameters).
void optimizeLSH(PRNearNeighborStructT nnStruct){
  ASSERT(nnStruct != NULL);

  PointsListEntryT *auxList = NULL;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    optimizeUHashStructure(nnStruct->hashedBuckets[i], auxList);
  }
  FREE(auxList);
}

// Frees completely all the memory occupied by the <nnStruct>
// structure.
void freePRNearNeighborStruct(PRNearNeighborStructT nnStruct){
  if (nnStruct == NULL){
    return;
  }

  if (nnStruct->points != NULL) {
    free(nnStruct->points);
  }
  
  if (nnStruct->lshFunctions != NULL) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
	free(nnStruct->lshFunctions[i][j].a);
      }
      free(nnStruct->lshFunctions[i]);
    }
    free(nnStruct->lshFunctions);
  }
  
  if (nnStruct->precomputedHashesOfULSHs != NULL) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      free(nnStruct->precomputedHashesOfULSHs[i]);
    }
    free(nnStruct->precomputedHashesOfULSHs);
  }

  freeUHashStructure(nnStruct->hashedBuckets[0], TRUE);
  for(IntT i = 1; i < nnStruct->parameterL; i++){
    freeUHashStructure(nnStruct->hashedBuckets[i], FALSE);
  }
  free(nnStruct->hashedBuckets);

  if (nnStruct->pointULSHVectors != NULL){
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      free(nnStruct->pointULSHVectors[i]);
    }
    free(nnStruct->pointULSHVectors);
  }

  if (nnStruct->reducedPoint != NULL){
    free(nnStruct->reducedPoint);
  }

  if (nnStruct->markedPoints != NULL){
    free(nnStruct->markedPoints);
  }

  if (nnStruct->markedPointsIndeces != NULL){
    free(nnStruct->markedPointsIndeces);
  }
}

// Frees completely all the memory occupied by the <nnStruct>
// structure.
void freeLSH(PRNearNeighborStructT nnStruct){
  if (nnStruct == NULL){
    return;
  }

  if (nnStruct->lshFunctions != NULL) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
	free(nnStruct->lshFunctions[i][j].a);
      }
      free(nnStruct->lshFunctions[i]);
    }
    free(nnStruct->lshFunctions);
  }

  if (nnStruct->precomputedHashesOfULSHs != NULL) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      free(nnStruct->precomputedHashesOfULSHs[i]);
    }
    free(nnStruct->precomputedHashesOfULSHs);
  }

  if (nnStruct->pointULSHVectors != NULL){
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      free(nnStruct->pointULSHVectors[i]);
    }
    free(nnStruct->pointULSHVectors);
  }

  if (nnStruct->reducedPoint != NULL){
    free(nnStruct->reducedPoint);
  }
}

// If <reportingResult> == FALSe, no points are reported back in a
// <get> function. In particular any point that is found in the bucket
// is considered to be outside the R-ball of the query point.  If
// <reportingResult> == TRUE, then the structure behaves normally.
void setResultReporting(PRNearNeighborStructT nnStruct, BooleanT reportingResult){
  ASSERT(nnStruct != NULL);
  nnStruct->reportingResult = reportingResult;
}

// Compute the value of a hash function u=lshFunctions[gNumber] (a
// vector of <hfTuplesLength> LSH functions) in the point <point>. The
// result is stored in the vector <vectorValue>. <vectorValue> must be
// already allocated (and have space for <hfTuplesLength> Uns32T-words).
inline void computeULSH(PRNearNeighborStructT nnStruct, IntT gNumber, RealT *point, Uns32T *vectorValue){
  CR_ASSERT(nnStruct != NULL);
  CR_ASSERT(point != NULL);
  CR_ASSERT(vectorValue != NULL);

/*
  for(IntT i = 0; i < 1; i++){
 	    for(IntT j = 0; j < 1; j++){
 	      // vector a
 	      for(IntT d = 0; d < 1; d++){
 	      printf("computeULSH lshFunctions[%d][%d].a[%d]=%f\n", i, j, d, nnStruct->lshFunctions[i][j].a[d]);
 	    }
 	      printf("computeULSH lshFunctions[%d][%d].b=%f\n", i, j, nnStruct->lshFunctions[i][j].b);
 	  }
 	  }
  fflush(stdout);
  */


  for(IntT i = 0; i < nnStruct->hfTuplesLength; i++){
    RealT value = 0;
    for(IntT d = 0; d < nnStruct->dimension; d++){

      value += point[d] * nnStruct->lshFunctions[gNumber][i].a[d];

    }

    //printf("computeULSH value[%f]\n", value);

    vectorValue[i] = (Uns32T)(FLOOR_INT32((value + nnStruct->lshFunctions[gNumber][i].b) / nnStruct->parameterW) /* - MIN_INT32T*/);
    //printf("computeULSHnnStruct->lshFunctions[gNumber][i].b[%d]=[%u]\n", i, nnStruct->lshFunctions[gNumber][i].b);
    //printf("vectorValue[%d]=[%u]\n", i, vectorValue[i]);
  }
 // fflush(stdout);
}

// ADDED BY MIJUNG
// compute projection
inline void computeProjection(PRNearNeighborStructT nnStruct, IntT gNumber, RealT *point, RealT *vectorValue){
  CR_ASSERT(nnStruct != NULL);
  CR_ASSERT(point != NULL);
  CR_ASSERT(vectorValue != NULL);

/*
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
 	    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
 	      // vector a
 	      for(IntT d = 0; d < nnStruct->dimension ; d++){
 	      printf("computeULSH lshFunctions[%d][%d].a[%d]=%f\n", i, j, d, nnStruct->lshFunctions[i][j].a[d]);
 	    }
 	      printf("computeULSH lshFunctions[%d][%d].b=%f\n", i, j, nnStruct->lshFunctions[i][j].b);
 	  }
 	  }
  fflush(stdout);
  */



  for(IntT i = 0; i < nnStruct->hfTuplesLength; i++){
    RealT value = 0;
    for(IntT d = 0; d < nnStruct->dimension; d++){

      value += point[d] * nnStruct->lshFunctions[gNumber][i].a[d];
    }


    vectorValue[i] = value + nnStruct->lshFunctions[gNumber][i].b - FLOOR_INT32((value + nnStruct->lshFunctions[gNumber][i].b) / nnStruct->parameterW)*nnStruct->parameterW;
    printf("computeProjection vectorValue[%d]=[%f]\n", i, vectorValue[i]);
  }
}

inline void preparePointAdding_rangeL(Uns32T Lindex_start, Uns32T Lindex_end, PRNearNeighborStructT nnStruct, PUHashStructureT uhash, PPointT point){
  ASSERT(nnStruct != NULL);
  ASSERT(uhash != NULL);
  ASSERT(point != NULL);

  TIMEV_START(timeComputeULSH);
  for(IntT d = 0; d < nnStruct->dimension; d++){
	 //Change:
	 //division by R removed because this operation happends in LSHHahfunctions::getIndex in order
	 //to support precomputation method.
	 //Original line: nnStruct->reducedPoint[d] = point->coordinates[d] / nnStruct->parameterR;
	 //Done by Tere April 9, 2014
	 nnStruct->reducedPoint[d] = point->coordinates[d];// / nnStruct->parameterR;

    //printf("p[%d]=[%f]\n", d, nnStruct->reducedPoint[d]);
  }
  //fflush(stdout);

  // Compute all ULSH functions.
  for(IntT i = Lindex_start; i <= Lindex_end; i++){
    computeULSH(nnStruct, i, nnStruct->reducedPoint, nnStruct->pointULSHVectors[i]);
    //printf("preparePointAdding nnStruct->pointULSHVectors[%d][0]=%u\n", i, nnStruct->pointULSHVectors[i][0]);
    //printf("preparePointAdding nnStruct->pointULSHVectors[%d][1]=%u\n", i, nnStruct->pointULSHVectors[i][1]);
  }

  // Compute data for <precomputedHashesOfULSHs>.
  if (USE_SAME_UHASH_FUNCTIONS) {
	  for(IntT i = Lindex_start; i <= Lindex_end; i++){
	     precomputeUHFsForULSH(uhash, nnStruct->pointULSHVectors[i], nnStruct->hfTuplesLength, nnStruct->precomputedHashesOfULSHs[i]);
    }
  }

  TIMEV_END(timeComputeULSH);
}

inline void preparePointAdding(PRNearNeighborStructT nnStruct, PUHashStructureT uhash, PPointT point){
  ASSERT(nnStruct != NULL);
  ASSERT(uhash != NULL);
  ASSERT(point != NULL);

  TIMEV_START(timeComputeULSH);
  for(IntT d = 0; d < nnStruct->dimension; d++){
    nnStruct->reducedPoint[d] = point->coordinates[d] / nnStruct->parameterR;
    //printf("p[%d]=[%f]\n", d, nnStruct->reducedPoint[d]);
  }
  //fflush(stdout);

  // Compute all ULSH functions.
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    computeULSH(nnStruct, i, nnStruct->reducedPoint, nnStruct->pointULSHVectors[i]);
    //printf("preparePointAdding nnStruct->pointULSHVectors[%d][0]=%u\n", i, nnStruct->pointULSHVectors[i][0]);
    //printf("preparePointAdding nnStruct->pointULSHVectors[%d][1]=%u\n", i, nnStruct->pointULSHVectors[i][1]);
  }

  // Compute data for <precomputedHashesOfULSHs>.
  if (USE_SAME_UHASH_FUNCTIONS) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      precomputeUHFsForULSH(uhash, nnStruct->pointULSHVectors[i], nnStruct->hfTuplesLength, nnStruct->precomputedHashesOfULSHs[i]);
    }
  }

  TIMEV_END(timeComputeULSH);
}

inline void batchAddRequest(PRNearNeighborStructT nnStruct, IntT i, IntT &firstIndex, IntT &secondIndex, PPointT point){
//   Uns32T *(gVector[4]);
//   if (!nnStruct->useUfunctions) {
//     // Use usual <g> functions (truly independent).
//     gVector[0] = nnStruct->pointULSHVectors[i];
//     gVector[1] = nnStruct->precomputedHashesOfULSHs[i];
//     addBucketEntry(nnStruct->hashedBuckets[firstIndex], gVector, 1, point);
//   } else {
//     // Use <u> functions (<g>s are pairs of <u> functions).
//     gVector[0] = nnStruct->pointULSHVectors[firstIndex];
//     gVector[1] = nnStruct->pointULSHVectors[secondIndex];
//     gVector[2] = nnStruct->precomputedHashesOfULSHs[firstIndex];
//     gVector[3] = nnStruct->precomputedHashesOfULSHs[secondIndex];
    
//     // compute what is the next pair of <u> functions.
//     secondIndex++;
//     if (secondIndex == nnStruct->nHFTuples) {
//       firstIndex++;
//       secondIndex = firstIndex + 1;
//     }
    
//     addBucketEntry(nnStruct->hashedBuckets[i], gVector, 2, point);
//   }
  ASSERT(1 == 0);
}

// Adds a new point to the LSH data structure, that is for each
// i=0..parameterL-1, the point is added to the bucket defined by
// function g_i=lshFunctions[i].
void addNewPointToPRNearNeighborStruct(PRNearNeighborStructT nnStruct, PPointT point){
  ASSERT(nnStruct != NULL);
  ASSERT(point != NULL);
  ASSERT(nnStruct->reducedPoint != NULL);
  ASSERT(!nnStruct->useUfunctions || nnStruct->pointULSHVectors != NULL);
  ASSERT(nnStruct->hashedBuckets[0]->typeHT == HT_LINKED_LIST || nnStruct->hashedBuckets[0]->typeHT == HT_STATISTICS);

  nnStruct->points[nnStruct->nPoints] = point;
  nnStruct->nPoints++;

  preparePointAdding(nnStruct, nnStruct->hashedBuckets[0], point);

  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;

  TIMEV_START(timeBucketIntoUH);
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    if (!nnStruct->useUfunctions) {
      // Use usual <g> functions (truly independent; <g>s are precisly
      // <u>s).
      addBucketEntry(nnStruct->hashedBuckets[i], 1, nnStruct->precomputedHashesOfULSHs[i], NULL, nnStruct->nPoints - 1);
    } else {
      // Use <u> functions (<g>s are pairs of <u> functions).
      addBucketEntry(nnStruct->hashedBuckets[i], 2, nnStruct->precomputedHashesOfULSHs[firstUComp], nnStruct->precomputedHashesOfULSHs[secondUComp], nnStruct->nPoints - 1);

      // compute what is the next pair of <u> functions.
      secondUComp++;
      if (secondUComp == nnStruct->nHFTuples) {
	firstUComp++;
	secondUComp = firstUComp + 1;
      }
    }
    //batchAddRequest(nnStruct, i, firstUComp, secondUComp, point);
  }
  TIMEV_END(timeBucketIntoUH);

  // Check whether the vectors <nearPoints> & <nearPointsIndeces> is still big enough.
  if (nnStruct->nPoints > nnStruct->sizeMarkedPoints) {
    nnStruct->sizeMarkedPoints = 2 * nnStruct->nPoints;
    FAILIF(NULL == (nnStruct->markedPoints = (BooleanT*)REALLOC(nnStruct->markedPoints, nnStruct->sizeMarkedPoints * sizeof(BooleanT))));
    for(IntT i = 0; i < nnStruct->sizeMarkedPoints; i++){
      nnStruct->markedPoints[i] = FALSE;
    }
    FAILIF(NULL == (nnStruct->markedPointsIndeces = (Int32T*)REALLOC(nnStruct->markedPointsIndeces, nnStruct->sizeMarkedPoints * sizeof(Int32T))));
  }
}

// Returns TRUE iff |p1-p2|_2^2 <= threshold
inline BooleanT isDistanceSqrLeq(IntT dimension, PPointT p1, PPointT p2, RealT threshold){
  RealT result = 0;
  nOfDistComps++;

  TIMEV_START(timeDistanceComputation);
  for (IntT i = 0; i < dimension; i++){
    RealT temp = p1->coordinates[i] - p2->coordinates[i];
#ifdef USE_L1_DISTANCE
    result += ABS(temp);
#else
    result += SQR(temp);
#endif
    if (result > threshold){
      // TIMEV_END(timeDistanceComputation);
      return 0;
    }
  }
  TIMEV_END(timeDistanceComputation);

  //return result <= threshold;
  return 1;
}

// // Returns TRUE iff |p1-p2|_2^2 <= threshold
// inline BooleanT isDistanceSqrLeq(IntT dimension, PPointT p1, PPointT p2, RealT threshold){
//   RealT result = 0;
//   nOfDistComps++;

//   //TIMEV_START(timeDistanceComputation);
//   for (IntT i = 0; i < dimension; i++){
//     result += p1->coordinates[i] * p2->coordinates[i];
//   }
//   //TIMEV_END(timeDistanceComputation);

//   return p1->sqrLength + p2->sqrLength - 2 * result <= threshold;
// }

// Returns the list of near neighbors of the point <point> (with a
// certain success probability). Near neighbor is defined as being a
// point within distance <parameterR>. Each near neighbor from the
// data set is returned is returned with a certain probability,
// dependent on <parameterK>, <parameterL>, and <parameterT>. The
// returned points are kept in the array <result>. If result is not
// allocated, it will be allocated to at least some minimum size
// (RESULT_INIT_SIZE). If number of returned points is bigger than the
// size of <result>, then the <result> is resized (to up to twice the
// number of returned points). The return value is the number of
// points found.
Int32T getNearNeighborsFromPRNearNeighborStruct(PRNearNeighborStructT nnStruct, PPointT query, PPointT *(&result), Int32T &resultSize){
  ASSERT(nnStruct != NULL);
  ASSERT(query != NULL);
  ASSERT(nnStruct->reducedPoint != NULL);
  ASSERT(!nnStruct->useUfunctions || nnStruct->pointULSHVectors != NULL);

  PPointT point = query;

  if (result == NULL){
    resultSize = RESULT_INIT_SIZE;
    FAILIF(NULL == (result = (PPointT*)MALLOC(resultSize * sizeof(PPointT))));
  }
  
  preparePointAdding(nnStruct, nnStruct->hashedBuckets[0], point);

  Uns32T precomputedHashesOfULSHs[nnStruct->nHFTuples][N_PRECOMPUTED_HASHES_NEEDED];
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < N_PRECOMPUTED_HASHES_NEEDED; j++){
      precomputedHashesOfULSHs[i][j] = nnStruct->precomputedHashesOfULSHs[i][j];
    }
  }
  TIMEV_START(timeTotalBuckets);

  BooleanT oldTimingOn = timingOn;
  if (noExpensiveTiming) {
    timingOn = FALSE;
  }
  
  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;

  Int32T nNeighbors = 0;// the number of near neighbors found so far.
  Int32T nMarkedPoints = 0;// the number of marked points
  for(IntT i = 0; i < nnStruct->parameterL; i++){ 
    TIMEV_START(timeGetBucket);
    GeneralizedPGBucket gbucket;
    if (!nnStruct->useUfunctions) {
      // Use usual <g> functions (truly independent; <g>s are precisly
      // <u>s).
      gbucket = getGBucket(nnStruct->hashedBuckets[i], 1, precomputedHashesOfULSHs[i], NULL);
    } else {
      // Use <u> functions (<g>s are pairs of <u> functions).
      gbucket = getGBucket(nnStruct->hashedBuckets[i], 2, precomputedHashesOfULSHs[firstUComp], precomputedHashesOfULSHs[secondUComp]);

      // compute what is the next pair of <u> functions.
      secondUComp++;
      if (secondUComp == nnStruct->nHFTuples) {
	firstUComp++;
	secondUComp = firstUComp + 1;
      }
    }
    TIMEV_END(timeGetBucket);

    PGBucketT bucket;

    TIMEV_START(timeCycleBucket);
    switch (nnStruct->hashedBuckets[i]->typeHT){
    case HT_LINKED_LIST:
      bucket = gbucket.llGBucket;
      if (bucket != NULL){
	// circle through the bucket and add to <result> the points that are near.
	PBucketEntryT bucketEntry = &(bucket->firstEntry);
	//TIMEV_START(timeCycleProc);
	while (bucketEntry != NULL){
	  //TIMEV_END(timeCycleProc);
	  //ASSERT(bucketEntry->point != NULL);
	  //TIMEV_START(timeDistanceComputation);
	  Int32T candidatePIndex = bucketEntry->pointIndex;
	  PPointT candidatePoint = nnStruct->points[candidatePIndex];
	  if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
	    //TIMEV_END(timeDistanceComputation);
	    if (nnStruct->markedPoints[candidatePIndex] == FALSE) {
	      //TIMEV_START(timeResultStoring);
	      // a new R-NN point was found (not yet in <result>).
	      if (nNeighbors >= resultSize){
		// run out of space => resize the <result> array.
		resultSize = 2 * resultSize;
		result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
	      }
	      result[nNeighbors] = candidatePoint;
	      nNeighbors++;
	      nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	      nnStruct->markedPoints[candidatePIndex] = TRUE; // do not include more points with the same index
	      nMarkedPoints++;
	      //TIMEV_END(timeResultStoring);
	    }
	  }else{
	    //TIMEV_END(timeDistanceComputation);
	  }
	  //TIMEV_START(timeCycleProc);
	  bucketEntry = bucketEntry->nextEntry;
	}
	//TIMEV_END(timeCycleProc);
      }
      break;
    case HT_STATISTICS:
      ASSERT(FALSE); // HT_STATISTICS not supported anymore

//       if (gbucket.linkGBucket != NULL && gbucket.linkGBucket->indexStart != INDEX_START_EMPTY){
// 	Int32T position;
// 	PointsListEntryT *pointsList = nnStruct->hashedBuckets[i]->bucketPoints.pointsList;
// 	position = gbucket.linkGBucket->indexStart;
// 	// circle through the bucket and add to <result> the points that are near.
// 	while (position != INDEX_START_EMPTY){
// 	  PPointT candidatePoint = pointsList[position].point;
// 	  if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
// 	    if (nnStruct->nearPoints[candidatePoint->index] == FALSE) {
// 	      // a new R-NN point was found (not yet in <result>).
// 	      if (nNeighbors >= resultSize){
// 		// run out of space => resize the <result> array.
// 		resultSize = 2 * resultSize;
// 		result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
// 	      }
// 	      result[nNeighbors] = candidatePoint;
// 	      nNeighbors++;
// 	      nnStruct->nearPoints[candidatePoint->index] = TRUE; // do not include more points with the same index
// 	    }
// 	  }
// 	  // Int32T oldP = position;
// 	  position = pointsList[position].nextPoint;
// 	  // ASSERT(position == INDEX_START_EMPTY || position == oldP + 1);
// 	}
//       }
      break;
    case HT_HYBRID_CHAINS:
      if (gbucket.hybridGBucket != NULL){
	PHybridChainEntryT hybridPoint = gbucket.hybridGBucket;
	Uns32T offset = 0;
	if (hybridPoint->point.bucketLength == 0){
	  // there are overflow points in this bucket.
	  offset = 0;
	  for(IntT j = 0; j < N_FIELDS_PER_INDEX_OF_OVERFLOW; j++){
	    offset += ((Uns32T)((hybridPoint + 1 + j)->point.bucketLength) << (j * N_BITS_FOR_BUCKET_LENGTH));
	  }
	}
	Uns32T index = 0;
	BooleanT done = FALSE;
	while(!done){
	  if (index == MAX_NONOVERFLOW_POINTS_PER_BUCKET){
	    //CR_ASSERT(hybridPoint->point.bucketLength == 0);
	    index = index + offset;
	  }
	  Int32T candidatePIndex = (hybridPoint + index)->point.pointIndex;
	  CR_ASSERT(candidatePIndex >= 0 && candidatePIndex < nnStruct->nPoints);
	  done = (hybridPoint + index)->point.isLastPoint == 1 ? TRUE : FALSE;
	  index++;
	  if (nnStruct->markedPoints[candidatePIndex] == FALSE){
	    // mark the point first.
	    nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	    nnStruct->markedPoints[candidatePIndex] = TRUE; // do not include more points with the same index
	    nMarkedPoints++;

	    PPointT candidatePoint = nnStruct->points[candidatePIndex];
	    if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
	      //if (nnStruct->markedPoints[candidatePIndex] == FALSE) {
	      // a new R-NN point was found (not yet in <result>).
	      //TIMEV_START(timeResultStoring);
	      if (nNeighbors >= resultSize){
		// run out of space => resize the <result> array.
		resultSize = 2 * resultSize;
		result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
	      }
	      result[nNeighbors] = candidatePoint;
	      nNeighbors++;
	      //TIMEV_END(timeResultStoring);
	      //nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	      //nnStruct->markedPoints[candidatePIndex] = TRUE; // do not include more points with the same index
	      //nMarkedPoints++;
	      //}
	    }
	  }else{
	    // the point was already marked (& examined)
	  }
	}
      }
      break;
    default:
      ASSERT(FALSE);
    }
    TIMEV_END(timeCycleBucket);
    
  }

  timingOn = oldTimingOn;
  TIMEV_END(timeTotalBuckets);

  // we need to clear the array nnStruct->nearPoints for the next query.
  for(Int32T i = 0; i < nMarkedPoints; i++){
    ASSERT(nnStruct->markedPoints[nnStruct->markedPointsIndeces[i]] == TRUE);
    nnStruct->markedPoints[nnStruct->markedPointsIndeces[i]] = FALSE;
  }
  DPRINTF("nMarkedPoints: %d\n", nMarkedPoints);

  return nNeighbors;
}

/*
PRNearNeighborStructT deserializePRNearNeighborStructT(int fd) {

		PRNearNeighborStructT nnStruct;
		FAILIF(NULL == (nnStruct = (PRNearNeighborStructT)MALLOC(sizeof(RNearNeighborStructT))));

		if(read (fd, &(nnStruct->parameterR), sizeof(RealT)) <= 0) return NULL;
		if(read (fd, &(nnStruct->dimension), sizeof(IntT)) <= 0) return NULL;
		if(read (fd, &(nnStruct->parameterR2), sizeof(RealT)) <= 0) return NULL;
		if(read (fd, &(nnStruct->useUfunctions), sizeof(BooleanT)) <= 0) return NULL;
		if(read (fd, &(nnStruct->parameterK), sizeof(IntT)) <= 0) return NULL;
		if(read (fd, &(nnStruct->parameterL), sizeof(IntT)) <= 0) return NULL;
		if(read (fd, &(nnStruct->parameterW), sizeof(RealT)) <= 0) return NULL;
		if(read (fd, &(nnStruct->parameterT), sizeof(IntT)) <= 0) return NULL;
		if(read (fd, &(nnStruct->nHFTuples), sizeof(IntT)) <= 0) return NULL;
		if(read (fd, &(nnStruct->hfTuplesLength), sizeof(IntT)) <= 0) return NULL;

		LSHFunctionT **lshFunctions;
		FAILIF(NULL == (lshFunctions = (LSHFunctionT**)MALLOC(nnStruct->nHFTuples * sizeof(LSHFunctionT*))));
		for(IntT i = 0; i < nnStruct->nHFTuples; i++){
			FAILIF(NULL == (lshFunctions[i] = (LSHFunctionT*)MALLOC(nnStruct->hfTuplesLength * sizeof(LSHFunctionT))));
			for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
				FAILIF(NULL == (lshFunctions[i][j].a = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));
			}
		}
		FILE * pFile;
		pFile = fopen ("deserial.out","w");

		for(IntT i = 0; i < nnStruct->nHFTuples; i++){
			for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
				  // vector a
				for(IntT d = 0; d < nnStruct->dimension; d++){
					if(read (fd, &(lshFunctions[i][j].a[d]), sizeof(RealT)) <= 0) return NULL;
					fprintf(pFile, "lshFunctions[%d][%d].a[%d]=%f\n", i, j, d, lshFunctions[i][j].a[d]);

				}
				if(read (fd, &(lshFunctions[i][j].b), sizeof(RealT)) <= 0) return NULL;
				fprintf(pFile, "lshFunctions[%d][%d].b=%f\n", i, j, lshFunctions[i][j].b);

			}
		}


		fclose(pFile);
		nnStruct->lshFunctions = lshFunctions;
		close(fd);

		FAILIF(NULL == (nnStruct->pointULSHVectors = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
		  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		    FAILIF(NULL == (nnStruct->pointULSHVectors[i] = (Uns32T*)MALLOC(nnStruct->hfTuplesLength * sizeof(Uns32T))));
		  }
		  FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
		  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		    FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs[i] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
		  }
		  // init the vector <reducedPoint>
		  FAILIF(NULL == (nnStruct->reducedPoint = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));


		return nnStruct;
}
*/

PRNearNeighborStructT deserializePRNearNeighborStructT(int sz, int fd) {

		void* data = malloc(sz);
		void* cpdata = data;

		if(read (fd, data, sz) <= 0) return NULL;

		PRNearNeighborStructT nnStruct;
		FAILIF(NULL == (nnStruct = (PRNearNeighborStructT)MALLOC(sizeof(RNearNeighborStructT))));

		memcpy(&(nnStruct->parameterR), data, sizeof(RealT));
		data = (void*)((long)data + sizeof(RealT));
		memcpy(&(nnStruct->dimension), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->parameterR2), data, sizeof(RealT));
		data = (void*)((long)data + sizeof(RealT));
		memcpy(&(nnStruct->useUfunctions), data, sizeof(BooleanT));
		data = (void*)((long)data + sizeof(BooleanT));
		memcpy(&(nnStruct->parameterK), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->parameterL), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->parameterW), data, sizeof(RealT));
		data = (void*)((long)data + sizeof(RealT));
		memcpy(&(nnStruct->parameterT), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->nHFTuples), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->hfTuplesLength), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));

		LSHFunctionT **lshFunctions;
		FAILIF(NULL == (lshFunctions = (LSHFunctionT**)MALLOC(nnStruct->nHFTuples * sizeof(LSHFunctionT*))));
		for(IntT i = 0; i < nnStruct->nHFTuples; i++){
			FAILIF(NULL == (lshFunctions[i] = (LSHFunctionT*)MALLOC(nnStruct->hfTuplesLength * sizeof(LSHFunctionT))));
			for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
				FAILIF(NULL == (lshFunctions[i][j].a = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));
			}
		}
		for(IntT i = 0; i < nnStruct->nHFTuples; i++){
			for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
				  // vector a
				memcpy(lshFunctions[i][j].a, data, sizeof(RealT)*nnStruct->dimension);
				data = (void*)((long)data + sizeof(RealT)*nnStruct->dimension);

				memcpy(&(lshFunctions[i][j].b), data, sizeof(RealT));
				data = (void*)((long)data + sizeof(RealT));
			}
		}

		nnStruct->lshFunctions = lshFunctions;

		free(cpdata);
		close(fd);

		FAILIF(NULL == (nnStruct->pointULSHVectors = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
		  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		    FAILIF(NULL == (nnStruct->pointULSHVectors[i] = (Uns32T*)MALLOC(nnStruct->hfTuplesLength * sizeof(Uns32T))));
		  }
		  FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
		  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		    FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs[i] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
		  }
		  // init the vector <reducedPoint>
		  FAILIF(NULL == (nnStruct->reducedPoint = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));


		return nnStruct;
}

PRNearNeighborStructT deserializePRNearNeighborStructT(void* data) {

		PRNearNeighborStructT nnStruct;
		FAILIF(NULL == (nnStruct = (PRNearNeighborStructT)MALLOC(sizeof(RNearNeighborStructT))));

		memcpy(&(nnStruct->parameterR), data, sizeof(RealT));
		data = (void*)((long)data + sizeof(RealT));
		memcpy(&(nnStruct->dimension), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->parameterR2), data, sizeof(RealT));
		data = (void*)((long)data + sizeof(RealT));
		memcpy(&(nnStruct->useUfunctions), data, sizeof(BooleanT));
		data = (void*)((long)data + sizeof(BooleanT));
		memcpy(&(nnStruct->parameterK), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->parameterL), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->parameterW), data, sizeof(RealT));
		data = (void*)((long)data + sizeof(RealT));
		memcpy(&(nnStruct->parameterT), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->nHFTuples), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));
		memcpy(&(nnStruct->hfTuplesLength), data, sizeof(IntT));
		data = (void*)((long)data + sizeof(IntT));

		LSHFunctionT **lshFunctions;
		FAILIF(NULL == (lshFunctions = (LSHFunctionT**)MALLOC(nnStruct->nHFTuples * sizeof(LSHFunctionT*))));
		for(IntT i = 0; i < nnStruct->nHFTuples; i++){
			FAILIF(NULL == (lshFunctions[i] = (LSHFunctionT*)MALLOC(nnStruct->hfTuplesLength * sizeof(LSHFunctionT))));
			for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
				FAILIF(NULL == (lshFunctions[i][j].a = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));
			}
		}
		for(IntT i = 0; i < nnStruct->nHFTuples; i++){
			for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
				  // vector a
				memcpy(lshFunctions[i][j].a, data, sizeof(RealT)*nnStruct->dimension);
				data = (void*)((long)data + sizeof(RealT)*nnStruct->dimension);

				memcpy(&(lshFunctions[i][j].b), data, sizeof(RealT));
				data = (void*)((long)data + sizeof(RealT));
			}
		}

		nnStruct->lshFunctions = lshFunctions;

		FAILIF(NULL == (nnStruct->pointULSHVectors = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
		  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		    FAILIF(NULL == (nnStruct->pointULSHVectors[i] = (Uns32T*)MALLOC(nnStruct->hfTuplesLength * sizeof(Uns32T))));
		  }
		  FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
		  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		    FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs[i] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
		  }
		  // init the vector <reducedPoint>
		  FAILIF(NULL == (nnStruct->reducedPoint = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));


		return nnStruct;
}

int serializePRNearNeighborStructT(PRNearNeighborStructT nnStruct, char* filename) {

	int sz = 0;
	FILE * f;
	f = fopen (filename,"a");
	sz += sizeof(RealT);
	sz += sizeof(IntT);
	sz += sizeof(RealT);
	sz += sizeof(BooleanT);
	sz += sizeof(IntT);
	sz += sizeof(IntT);
	sz += sizeof(RealT);
	sz += sizeof(IntT);
	sz += sizeof(IntT);
	sz += sizeof(IntT);

	for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
			sz += sizeof(RealT)*nnStruct->dimension;
			sz += sizeof(RealT);
		}
	}
	printf("serializePRNearNeighborStructT size=[%d]\n", sz);
	fflush(stdout);

	//if(fwrite (&(sz), sizeof(int), 1, f) != 1) return -1;

	if(fwrite (&(nnStruct->parameterR), sizeof(RealT), 1, f) != 1) return -1;
	if(fwrite (&(nnStruct->dimension), sizeof(IntT), 1, f) != 1) return -1;
	if(fwrite (&(nnStruct->parameterR2), sizeof(RealT), 1, f) != 1) return -1;
	if(fwrite (&(nnStruct->useUfunctions), sizeof(BooleanT), 1, f) != 1) return -1;
	if(fwrite (&(nnStruct->parameterK), sizeof(IntT), 1, f) != 1) return -1;
	if(fwrite (&(nnStruct->parameterL), sizeof(IntT), 1, f) != 1) return -1;
	if(fwrite (&(nnStruct->parameterW), sizeof(RealT), 1, f) != 1) return -1;
	if(fwrite (&(nnStruct->parameterT), sizeof(IntT), 1, f) != 1) return -1;
	if(fwrite (&(nnStruct->nHFTuples), sizeof(IntT), 1, f) != 1) return -1;
	if(fwrite (&(nnStruct->hfTuplesLength), sizeof(IntT), 1, f) != 1) return -1;

	for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
			if(fwrite (nnStruct->lshFunctions[i][j].a, sizeof(RealT)*nnStruct->dimension, 1, f) != 1) return -1;
			if(fwrite (&(nnStruct->lshFunctions[i][j].b), sizeof(RealT), 1, f) != 1) return -1;
		}
	}

	fclose(f);

	return sz;
}

int getSizeSerializePRNearNeighborStructT(PRNearNeighborStructT nnStruct) {

	int sz = 0;
	sz += sizeof(RealT);
	sz += sizeof(IntT);
	sz += sizeof(RealT);
	sz += sizeof(BooleanT);
	sz += sizeof(IntT);
	sz += sizeof(IntT);
	sz += sizeof(RealT);
	sz += sizeof(IntT);
	sz += sizeof(IntT);
	sz += sizeof(IntT);

	for(IntT i = 0; i < nnStruct->nHFTuples; i++){
		for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
			sz += sizeof(RealT)*nnStruct->dimension;
			sz += sizeof(RealT);
		}
	}
	printf("serializePRNearNeighborStructT size=[%d]\n", sz);
	fflush(stdout);

	return sz;
}
