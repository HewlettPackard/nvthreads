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

#ifndef LOCALITYSENSITIVEHASHING_INCLUDED
#define LOCALITYSENSITIVEHASHING_INCLUDED

#include <sys/time.h>

// The default value for algorithm parameter W.
#define PARAMETER_W_DEFAULT 4.0

// The probability p(1) -- a function of W.
// #define PROBABILITY_P1 0.8005

// The size of the initial result array.
#define RESULT_INIT_SIZE 8

// A function drawn from the locality-sensitive family of hash functions.
typedef struct _LSHFunctionT {
  RealT *a;
  RealT b;
} LSHFunctionT, *PLSHFunctionT;


typedef struct _RNNParametersT {
  RealT parameterR; // parameter R of the algorithm.
  RealT successProbability; // the success probability 1-\delta
  IntT dimension; // dimension of points.
  RealT parameterR2; // = parameterR^2

  // Whether to use <u> hash functions instead of usual <g>
  // functions. When this flag is set to TRUE, <u> functions are
  // generated (which are roughly k/2-tuples of LSH), and a <g>
  // function is a pair of 2 different <u> functions.
  BooleanT useUfunctions;

  IntT parameterK; // parameter K of the algorithm.
  
  // parameter M (# of independent tuples of LSH functions)
  // if useUfunctions==TRUE, parameterL = parameterM * (parameterM - 1) / 2
  // if useUfunctions==FALSE, parameterL = parameterM
  IntT parameterM;

  IntT parameterL; // parameter L of the algorithm.
  RealT parameterW; // parameter W of the algorithm.
  IntT parameterT; // parameter T of the algorithm.

  // The type of the hash table used for storing the buckets (of the
  // same <g> function).
  IntT typeHT;
} RNNParametersT, *PRNNParametersT;

typedef struct _RNearNeighborStructT {
  IntT dimension; // dimension of points.
  IntT parameterK; // parameter K of the algorithm.
  IntT parameterL; // parameter L of the algorithm.
  RealT parameterW; // parameter W of the algorithm.
  IntT parameterT; // parameter T of the algorithm.
  RealT parameterR; // parameter R of the algorithm.
  RealT parameterR2; // = parameterR^2

  // Whether to use <u> hash functions instead of usual <g>
  // functions. When this flag is set to TRUE, <u> functions are
  // generated (which are roughly k/2-tuples of LSH), and a <g>
  // function is a pair of 2 different <u> functions.
  BooleanT useUfunctions;

  // the number of tuples of hash functions used (= # of rows of
  // <lshFunctions>). When useUfunctions == FALSE, this field is equal
  // to parameterL, otherwise, to <m>, the number of <u> hash
  // functions (in this case, parameterL = m*(m-1)/2 = nHFTuples*(nHFTuples-1)/2
  IntT nHFTuples;
  // How many LSH functions each of the tuple has (it is <k> when
  // useUfunctions == FALSE, and <k/2> when useUfunctions == TRUE).
  IntT hfTuplesLength;

  // number of points in the data set
  Int32T nPoints;

  // The array of pointers to the points that are contained in the
  // structure. Some types of this structure (of UHashStructureT,
  // actually) use indeces in this array to refer to points (as
  // opposed to using pointers).
  PPointT *points;

  // The size of the array <points>
  Int32T pointsArraySize;

  // If <reportingResult> == FALSE, no points are reported back in a
  // <get*> function. In particular any point that is found in the
  // bucket is considered to be outside the R-ball of the query point
  // (the distance is still computed).  If <reportingResult> == TRUE,
  // then the structure behaves normally.
  BooleanT reportingResult;
  
  // This table stores the LSH functions. There are <nHFTuples> rows
  // of <hfTuplesLength> LSH functions.
  LSHFunctionT **lshFunctions;

  // Precomputed hashes of each of the <nHFTuples> of <u> functions
  // (to be used by the bucket hashing module).
  Uns32T **precomputedHashesOfULSHs;

  // The set of non-empty buckets (which are hashed using
  // PUHashStructureT).
  PUHashStructureT *hashedBuckets;

  // ***
  // The following vectors are used only for temporary operations
  // within this R-NN structure during a query operation.
  // ***

  // This vector is used to store the values of hash functions <u>
  // (<hfTuplesLength>-tuple of LSH fuctions). One <g> function is a concatenation
  // of <nHFTuples> of <u> LSH functions.
  Uns32T **pointULSHVectors;
  
  // A vector of length <dimension> to store the reduced point (point
  // with coordinates divided by <parameterR>).
  RealT *reducedPoint;

  // This vector is used for storing marked points in a query
  // operation (for computing distances to a point at most once). If
  // markedPoints[i]=TRUE then point <i> was examined already.
  BooleanT *markedPoints;
  // This vector stored the indeces in the vector <markedPoints> of all
  // TRUE entries.
  Int32T *markedPointsIndeces;
  // the size of <markedPoints> and of <markedPointsIndeces>
  IntT sizeMarkedPoints;
} RNearNeighborStructT, *PRNearNeighborStructT;

void printRNNParameters(FILE *output, RNNParametersT parameters);

RNNParametersT readRNNParameters(FILE *input);

PRNearNeighborStructT initLSH(RNNParametersT algParameters, Int32T nPointsEstimate);

void getHashIndex(Uns32T* hIndex, Uns32T* control1, PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, RealT* darr, int pointsDimension);

void getHashIndex_rangeL(Uns32T Lindex_start, Uns32T Lindex_end, Uns32T* hIndex, Uns32T* control1, PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, PPointT point, int pointsDimension);

time_t getHashIndex_rangeL(Uns32T Lindex_start, Uns32T Lindex_end, Uns32T* hIndex, Uns32T* control1, PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, RealT* darr, int pointsDimension);

void initHashFunctions(PRNearNeighborStructT nnStruct);

PRNearNeighborStructT initPRNearNeighborStructT(double parameterR, double parameterR2, int useUfunctions, int parameterK, int parameterL, int parameterM, int parameterT, int dimension, double parameterW );

PRNearNeighborStructT initLSH_WithDataSet(RNNParametersT algParameters, Int32T nPoints, PPointT *dataSet);

PRNearNeighborStructT initializePRNearNeighborFields(RNNParametersT algParameters, Int32T nPointsEstimate);

//void optimizeLSH(PRNearNeighborStructT nnStruct);

void freePRNearNeighborStruct(PRNearNeighborStructT nnStruct);

void freeLSH(PRNearNeighborStructT nnStruct);

void setResultReporting(PRNearNeighborStructT nnStruct, BooleanT reportingStopped);

void addNewPointToPRNearNeighborStruct(PRNearNeighborStructT nnStruct, PPointT point);

Int32T getNearNeighborsFromPRNearNeighborStruct(PRNearNeighborStructT nnStruct, PPointT query, PPointT *(&result), IntT &resultSize);

PRNearNeighborStructT deserializePRNearNeighborStructT(int sz, int fd);

PRNearNeighborStructT deserializePRNearNeighborStructT(void* data);

int serializePRNearNeighborStructT(PRNearNeighborStructT nnStruct, char* filename);
int getSizeSerializePRNearNeighborStructT(PRNearNeighborStructT nnStruct);

void getPerturbationIndices(Uns32T* hIndex, Uns32T* control1, PRNearNeighborStructT nnStruct, PUHashStructureT modelHT, double* darr, int pointsDimension, int npert);

inline void computeULSH(PRNearNeighborStructT nnStruct, IntT gNumber, RealT *point, Uns32T *vectorValue);
inline void computeProjection(PRNearNeighborStructT nnStruct, IntT gNumber, RealT *point, RealT *vectorValue);

typedef unsigned long ULONG;
ULONG binomial(ULONG n, ULONG k);

#endif
