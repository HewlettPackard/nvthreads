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

#include <iostream>
#include <vector>
#include "headers.h"

// SqrDistanceT computeSqrDistance(PointT p1, PointT p2){
//   SqrDistanceT result = 0;
//   IntT n = MIN(p1.dimension, p2.dimension);

//   for (IntT i = 0; i < n; i++)
//     result += SQR(p1.coordinates[i] - p2.coordinates[i]);

//   return result;
// }


// Compares according to the field "real" of the struct.
int comparePPointAndRealTStructT(const void *a, const void *b){
  PPointAndRealTStructT *x = (PPointAndRealTStructT*)a;
  PPointAndRealTStructT *y = (PPointAndRealTStructT*)b;
  return (x->real > y->real) - (x->real < y->real);
}

// ADDED BY MIJUNG. Compares pairs of value and index
int comparePairs(const void *p1, const void *p2) {
  std::pair<int,RealT> a = *((std::pair<int,RealT>*)p1);
  std::pair<int,RealT> b = *((std::pair<int,RealT>*)p2);

  return (a.second > b.second) - (a.second < b.second);
}

#ifdef USE_L1_DISTANCE
// Returns the L1 distance from point <p1> to <p2>.
RealT distance(IntT dimension, PPointT p1, PPointT p2){
  RealT result = 0;

  for (IntT i = 0; i < dimension; i++){
    result += ABS(p1->coordinates[i] - p2->coordinates[i]);
  }

  return result;
}
#else
// Returns the Euclidean distance from point <p1> to <p2>.
RealT distance(IntT dimension, PPointT p1, PPointT p2){
  RealT result = 0;

  for (IntT i = 0; i < dimension; i++){
    result += SQR(p1->coordinates[i] - p2->coordinates[i]);
  }

  return SQRT(result);
}
#endif
