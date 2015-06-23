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

#ifndef NEARNEIGHBORS_INCLUDED
#define NEARNEIGHBORS_INCLUDED

void initializeLSHGlobal();

PRNearNeighborStructT initSelfTunedRNearNeighborWithDataSet(RealT thresholdR, RealT successProbability, Int32T nPoints, IntT dimension, PPointT *dataSet, IntT nSampleQueries, PPointT *sampleQueries, MemVarT memoryUpperBound);

Int32T getRNearNeighbors(PRNearNeighborStructT nnStruct, PPointT queryPoint, PPointT *(&result), Int32T &resultSize);

#endif
