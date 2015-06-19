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
 
#ifndef BASICDEFINITIONS_INCLUDED
#define BASICDEFINITIONS_INCLUDED

// Log of the maximum number of points in the database
#define N_BITS_PER_POINT_INDEX 20
// Maximum number of points = (2^N_BITS_PER_POINT_INDEX-2)
#define MAX_N_POINTS ((1U << N_BITS_PER_POINT_INDEX) - 1)
// Maxumum number of points to be reported (i.e., the "k" if interested in "k-NN")
#define MAX_REPORTED_POINTS 10

#define IntT int
#define LongUns64T long long unsigned
#define Uns32T unsigned
#define Int32T int
#define BooleanT int
#define TRUE 1
#define FALSE 0

// TODO: this is the max available memory. Set for a particular
// machine at the moment. It is used in the function util.h/getAvailableMemory
#define DEFAULT_MEMORY_MAX_AVAILABLE 1000000000U

// The min value of 32-bits wide int. It should be actually
// (-2147483648), but the compiler will make it unsigned, and the
// problems (warnings) will begin.
#define MIN_INT32T (-2147483647)

//#define DEBUG

// Attention: if this macro is defined, the program might be less
// portable.
#define DEBUG_MEM

// Attention: with this macro is defined, the program might be less
// portable. (However, if disabled, the self-tuning part will not
// work.)
#define DEBUG_TIMINGS

#define DEBUG_PROFILE_TIMING FALSE

// Note that if any of these flags is set to 'stdout', then when the
// main module outputs the computed parameters, the error/debug
// messages might interwine.
#define ERROR_OUTPUT stderr
#define DEBUG_OUTPUT stderr

// Activate this define when the L1 distance instead of L2 distance.
//  NEVER TESTED!
//#define USE_L1_DISTANCE

// One of these three settings should be set externally (by the compiler).
//#define REAL_LONG_DOUBLE
//#define REAL_DOUBLE
//#define REAL_FLOAT

#ifdef REAL_LONG_DOUBLE
#define RealT long double
#define SQRT sqrtl
#define ABS fabsl
#define LOG logl
#define COS cosl
#define FLOOR_INT32(x) ((Int32T)(floorl(x)))
#define CEIL(x) ((int)(ceill(x)))
#define POW(x, y) (powl(x, y))
#define FPRINTF_REAL(file, value) {fprintf(file, "%0.3Lf", value);}
#define FSCANF_REAL(file, value) {fscanf(file, "%Lf", value);}
#endif

#ifdef REAL_DOUBLE
#define RealT double
#define SQRT sqrt
#define ABS fabs
#define LOG log
#define COS cos
#define FLOOR_INT32(x) ((Int32T)(floor(x)))
#define CEIL(x) ((int)(ceil(x)))
#define POW(x, y) (pow(x, y))
#define FPRINTF_REAL(file, value) {fprintf(file, "%0.3lf", value);}
#define FSCANF_REAL(file, value) {fscanf(file, "%lf", value);}
#define EXP exp
#define ERF erf
#define ERFC erfc
#endif

#ifdef REAL_FLOAT
#define RealT float
#define SQRT sqrtf
#define ABS fabsf
#define LOG logf
#define COS cosf
#define FLOOR_INT32(x) ((Int32T)(floorf(x)))
#define CEIL(x) ((int)(ceilf(x)))
#define POW(x, y) (powf(x, y))
#define FPRINTF_REAL(file, value) {fprintf(file, "%0.3f", value);}
#define FSCANF_REAL(file, value) {fscanf(file, "%f", value);}
#define EXP expf
#define ERF erf
#define ERFC erfc
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SQR(a) ((a) * (a))

#define FAILIF(b) {if (b) {fprintf(ERROR_OUTPUT, "FAILIF triggered on line %d, file %s. Memory allocated: %lld\n", __LINE__, __FILE__, totalAllocatedMemory); exit(1);}}
#define FAILIFWR(b, s) {if (b) {fprintf(ERROR_OUTPUT, "FAILIF triggered on line %d, file %s. Memory allocated: %lld\nReason: %s\n", __LINE__, __FILE__, totalAllocatedMemory, s); exit(1);}}

#define ASSERT(b) {if (!(b)) {fprintf(ERROR_OUTPUT, "ASSERT failed on line %d, file %s.\n", __LINE__, __FILE__); exit(1);}}

// Critical ASSERT -- it is off when not debugging
#ifdef DEBUG
#define CR_ASSERT(b) {if (!(b)) {fprintf(ERROR_OUTPUT, "ASSERT failed on line %d, file %s.\n", __LINE__, __FILE__); exit(1);}}
#define CR_ASSERTWR(b, reason) {if (!(b)) {fprintf(ERROR_OUTPUT, "ASSERT failed on line %d, file %s.\nReason: %s.\n", __LINE__, __FILE__, reason); exit(1);}}
#else
#define CR_ASSERT(b)
#define CR_ASSERTWR(b, reason)
#endif

#ifdef DEBUG 
#define DC {fprintf(DEBUG_OUTPUT, "Debug checkpoint. Line %d, file %s.\n", __LINE__, __FILE__);}
#define DPRINTF1(p1) {fprintf(DEBUG_OUTPUT, p1);}
#define DPRINTF(p1, p2) {fprintf(DEBUG_OUTPUT, p1, p2);}
#define DPRINTF3(p1, p2, p3) {fprintf(DEBUG_OUTPUT, p1, p2, p3);}
#define DPRINTF4(p1, p2, p3, p4) {fprintf(DEBUG_OUTPUT, p1, p2, p3, p4);}
#else
#define DC
#define DPRINTF1(p1)
#define DPRINTF(p1, p2)
#define DPRINTF3(p1, p2, p3)
#define DPRINTF4(p1, p2, p3, p4)
#endif

#define TimeVarT double
#define MemVarT long long int

#ifdef DEBUG_TIMINGS
#define TIMEV_START(timeVar) { \
  if (timingOn) { \
    /*CR_ASSERTWR(timeVar >= 0, "timevar<0.");*/ \
    timeval _timev; \
    struct timezone _timez; \
    gettimeofday(&_timev, &_timez); \
    double timeInSecs = _timev.tv_sec + (double)_timev.tv_usec / 1000000.0; \
    timeVar -= timeInSecs; \
    /*int _b = (currentTime <= timeInSecs * 1.0000001);*/ \
    /*if (!_b) {*/ \
      /*printf("currentTime: %lf\n", currentTime);*/ \
      /*printf("timeInSecs: %lf\n", timeInSecs);*/ \
      /*printf("currentTime <= timeInSecs: %d\n", _b);*/ \
    /*}*/ \
    CR_ASSERTWR((currentTime <= timeInSecs * 1.0000001), "currentTime not increasing."); \
    CR_ASSERTWR(((currentTime = timeInSecs) >= 0), "timeInSecs < 0"); \
    /*CR_ASSERTWR(timeVar < 0, "timevar>=0");*/ \
  } \
}

#define TIMEV_END(timeVar) { \
  if (timingOn) { \
    /*CR_ASSERTWR(timeVar < 0, "timevar >=0");*/ \
    timeval _timev; \
    struct timezone _timez; \
    gettimeofday(&_timev, &_timez); \
    double timeInSecs = _timev.tv_sec + (double)_timev.tv_usec / 1000000.0; \
    timeVar += timeInSecs - timevSpeed; \
    if (timeVar < 0) { timeVar = 0;}; \
    /*int _b = (currentTime <= timeInSecs * 1.0000001);*/ \
    /*if (!_b) {*/ \
      /*printf("currentTime: %lf\n", currentTime);*/ \
      /*printf("timeInSecs: %lf\n", timeInSecs);*/ \
      /*printf("currentTime <= timeInSecs: %d\n", _b);*/ \
    /*}*/ \
    CR_ASSERTWR((currentTime <= timeInSecs * 1.0000001), "currentTime not increasing."); \
    /*CR_ASSERTWR(((currentTime = timeInSecs) >= 0), "timeInSecs < 0");*/ \
    /*CR_ASSERTWR(timeVar >= -0.0000001, "timevar <0")*/; \
  } \
}

#else
#define TIMEV_START(timeVar)
#define TIMEV_END(timeVar)
#endif

#define MALLOC(amount) ((amount > 0) ? totalAllocatedMemory += amount, malloc(amount) : NULL)

#define REALLOC(oldPointer, amount) ((oldPointer != NULL) ? \
 totalAllocatedMemory += amount / 3, \
 realloc(oldPointer, amount) : \
 MALLOC(amount))

#define FREE(pointer) {if (pointer != NULL) {free(pointer);} pointer = NULL; }

#endif
