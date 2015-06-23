//=================================================================================================
/*!
//  \file blaze/config/Thresholds.h
//  \brief Configuration of the thresholds for matrix/vector and matrix/matrix multiplications
//
//  Copyright (C) 2013 Klaus Iglberger - All Rights Reserved
//
//  This file is part of the Blaze library. You can redistribute it and/or modify it under
//  the terms of the New (Revised) BSD License. Redistribution and use in source and binary
//  forms, with or without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice, this list of
//     conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//     of conditions and the following disclaimer in the documentation and/or other materials
//     provided with the distribution.
//  3. Neither the names of the Blaze development group nor the names of its contributors
//     may be used to endorse or promote products derived from this software without specific
//     prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
*/
//=================================================================================================


namespace blaze {

//=================================================================================================
//
//  BLAS THRESHOLDS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Row-major dense matrix/dense vector multiplication threshold.
// \ingroup config
//
// This setting specifies the threshold between the application of the custom Blaze kernels
// and the BLAS kernels for the row-major dense matrix/dense vector multiplication. In case
// the number of elements in the dense matrix is equal or higher than this value, the BLAS
// kernels are prefered over the custom Blaze kernels. In case the number of elements in the
// dense matrix is smaller, the Blaze kernels are used.
//
// The default setting for this threshold is 10000 (which for instance corresponds to a matrix
// size of \f$ 100 \times 100 \f$).
*/
const size_t DMATDVECMULT_THRESHOLD = 10000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Column-major dense matrix/dense vector multiplication threshold.
// \ingroup config
//
// This setting specifies the threshold between the application of the custom Blaze kernels
// and the BLAS kernels for the column-major dense matrix/dense vector multiplication. In case
// the number of elements in the dense matrix is equal or higher than this value, the BLAS
// kernels are prefered over the custom Blaze kernels. In case the number of elements in the
// dense matrix is smaller, the Blaze kernels are used.
//
// The default setting for this threshold is 10000 (which for instance corresponds to a matrix
// size of \f$ 100 \times 100 \f$).
*/
const size_t TDMATDVECMULT_THRESHOLD = 10000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Dense Vector/row-major dense matrix multiplication threshold.
// \ingroup config
//
// This setting specifies the threshold between the application of the custom Blaze kernels
// and the BLAS kernels for the dense vector/row-major dense matrix multiplication. In case
// the number of elements in the dense matrix is equal or higher than this value, the BLAS
// kernels are prefered over the custom Blaze kernels. In case the number of elements in the
// dense matrix is smaller, the Blaze kernels are used.
//
// The default setting for this threshold is 10000 (which for instance corresponds to a matrix
// size of \f$ 100 \times 100 \f$).
*/
const size_t TDVECDMATMULT_THRESHOLD = 10000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Dense Vector/column-major dense matrix multiplication threshold.
// \ingroup config
//
// This setting specifies the threshold between the application of the custom Blaze kernels
// and the BLAS kernels for the dense vector/column-major dense matrix multiplication. In case
// the number of elements in the dense matrix is equal or higher than this value, the BLAS
// kernels are prefered over the custom Blaze kernels. In case the number of elements in the
// dense matrix is smaller, the Blaze kernels are used.
//
// The default setting for this threshold is 10000 (which for instance corresponds to a matrix
// size of \f$ 100 \times 100 \f$).
*/
const size_t TDVECTDMATMULT_THRESHOLD = 10000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Row-major dense matrix/row-major dense matrix multiplication threshold.
// \ingroup config
//
// This setting specifies the threshold between the application of the custom Blaze kernels and
// the BLAS kernels for the row-major dense matrix/row-major dense matrix multiplication. In
// case the number of elements of the target matrix is equal or higher than this value, the
// BLAS kernels are prefered over the custom Blaze kernels. In case the number of elements in
// the target matrix is smaller, the Blaze kernels are used.
//
// The default setting for this threshold is 10000 (which for instance corresponds to a matrix
// size of \f$ 100 \times 100 \f$).
*/
const size_t DMATDMATMULT_THRESHOLD = 10000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Row-major dense matrix/column-major dense matrix multiplication threshold.
// \ingroup config
//
// This setting specifies the threshold between the application of the custom Blaze kernels and
// the BLAS kernels for the row-major dense matrix/column-major dense matrix multiplication. In
// case the number of elements of the target matrix is equal or higher than this value, the
// BLAS kernels are prefered over the custom Blaze kernels. In case the number of elements in
// the target matrix is smaller, the Blaze kernels are used.
//
// The default setting for this threshold is 10000 (which for instance corresponds to a matrix
// size of \f$ 100 \times 100 \f$).
*/
const size_t DMATTDMATMULT_THRESHOLD = 10000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Column-major dense matrix/row-major dense matrix multiplication threshold.
// \ingroup config
//
// This setting specifies the threshold between the application of the custom Blaze kernels and
// the BLAS kernels for the column-major dense matrix/row-major dense matrix multiplication. In
// case the number of elements of the target matrix is equal or higher than this value, the
// BLAS kernels are prefered over the custom Blaze kernels. In case the number of elements in
// the target matrix is smaller, the Blaze kernels are used.
//
// The default setting for this threshold is 10000 (which for instance corresponds to a matrix
// size of \f$ 100 \times 100 \f$).
*/
const size_t TDMATDMATMULT_THRESHOLD = 10000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Column-major dense matrix/column-major dense matrix multiplication threshold.
// \ingroup config
//
// This setting specifies the threshold between the application of the custom Blaze kernels and
// the BLAS kernels for the column-major dense matrix/column-major dense matrix multiplication.
// In case the number of elements of the target matrix is equal or higher than this value, the
// BLAS kernels are prefered over the custom Blaze kernels. In case the number of elements in
// the target matrix is smaller, the Blaze kernels are used.
//
// The default setting for this threshold is 10000 (which for instance corresponds to a matrix
// size of \f$ 100 \times 100 \f$).
*/
const size_t TDMATTDMATMULT_THRESHOLD = 10000UL;
//*************************************************************************************************




//=================================================================================================
//
//  OPENMP THRESHOLDS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief SMP dense vector assignment threshold.
// \ingroup config
//
// This threshold specifies when an assignment of a plain dense vector can be executed in
// parallel. In case the number of elements of the target vector is larger or equal to this
// threshold, the operation is executed in parallel. If the number of elements is below this
// threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 38000. In case the threshold is set to 0, the
// operation is unconditionally executed in parallel.
*/
const size_t SMP_DVECASSIGN_THRESHOLD = 38000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense vector/dense vector addition threshold.
// \ingroup config
//
// This threshold specifies when a dense vector/dense vector addition can be executed in parallel.
// In case the number of elements of the target vector is larger or equal to this threshold, the
// operation is executed in parallel. If the number of elements is below this threshold the
// operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 38000. In case the threshold is set to 0, the
// operation is unconditionally executed in parallel.
*/
const size_t SMP_DVECDVECADD_THRESHOLD = 38000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense vector/dense vector subtraction threshold.
// \ingroup config
//
// This threshold specifies when a dense vector/dense vector subtraction can be executed in
// parallel. In case the number of elements of the target vector is larger or equal to this
// threshold, the operation is executed in parallel. If the number of elements is below this
// threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 38000. In case the threshold is set to 0, the
// operation is unconditionally executed in parallel.
*/
const size_t SMP_DVECDVECSUB_THRESHOLD = 38000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense vector/dense vector multiplication threshold.
// \ingroup config
//
// This threshold specifies when a dense vector/dense vector multiplication can be executed
// in parallel. In case the number of elements of the target vector is larger or equal to this
// threshold, the operation is executed in parallel. If the number of elements is below this
// threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 38000. In case the threshold is set to 0, the
// operation is unconditionally executed in parallel.
*/
const size_t SMP_DVECDVECMULT_THRESHOLD = 38000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense vector/scalar multiplication/division threshold.
// \ingroup config
//
// This threshold specifies when a dense vector/scalar multiplication/division can be executed
// in parallel. In case the number of elements of the target vector is larger or equal to this
// threshold, the operation is executed in parallel. If the number of elements is below this
// threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 51000. In case the threshold is set to 0, the
// operation is unconditionally executed in parallel.
*/
const size_t SMP_DVECSCALARMULT_THRESHOLD = 51000UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/dense vector multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/dense vector multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 330. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATDVECMULT_THRESHOLD = 330UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major dense matrix/dense vector multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major dense matrix/dense vector multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 360. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDMATDVECMULT_THRESHOLD = 360UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense vector/row-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a dense vector/row-major dense matrix multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 370. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDVECDMATMULT_THRESHOLD = 370UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense vector/column-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a dense vector/column-major dense matrix multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 340. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDVECTDMATMULT_THRESHOLD = 340UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/sparse vector multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/sparse vector multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 480. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATSVECMULT_THRESHOLD = 480UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major dense matrix/sparse vector multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major dense matrix/sparse vector multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 910. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDMATSVECMULT_THRESHOLD = 910UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP sparse vector/row-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a sparse vector/row-major dense matrix multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 910. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSVECDMATMULT_THRESHOLD = 910UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP sparse vector/column-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a sparse vector/column-major dense matrix multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 480. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSVECTDMATMULT_THRESHOLD = 480UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major sparse matrix/dense vector multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major sparse matrix/dense vector multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 600. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_SMATDVECMULT_THRESHOLD = 600UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major sparse matrix/dense vector multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major sparse matrix/dense vector multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 1250. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSMATDVECMULT_THRESHOLD = 1250UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense vector/row-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a dense vector/row-major sparse matrix multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 1190. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDVECSMATMULT_THRESHOLD = 1190UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense vector/column-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a dense vector/column-major sparse matrix multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 530. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDVECTSMATMULT_THRESHOLD = 530UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major sparse matrix/sparse vector multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major sparse matrix/sparse vector multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 260. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_SMATSVECMULT_THRESHOLD = 260UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major sparse matrix/sparse vector multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major sparse matrix/sparse vector multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 2160. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSMATSVECMULT_THRESHOLD = 2160UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP sparse vector/row-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a sparse vector/row-major sparse matrix multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 2160. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSVECSMATMULT_THRESHOLD = 2160UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP sparse vector/column-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a sparse vector/column-major sparse matrix multiplication can be
// executed in parallel. In case the number of elements of the target vector is larger or equal
// to this threshold, the operation is executed in parallel. If the number of elements is below
// this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 260. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSVECTSMATMULT_THRESHOLD = 260UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense matrix assignment threshold.
// \ingroup config
//
// This threshold specifies when an assignment with a plain dense matrix can be executed in
// parallel. In case the number of rows/columns of the target matrix is larger or equal to this
// threshold, the operation is executed in parallel. If the number of rows/columns is below this
// threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 220. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATASSIGN_THRESHOLD = 220UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/row-major dense matrix addition threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/row-major dense matrix addition can
// be executed in parallel. This threshold affects both additions between two row-major matrices
// or two column-major dense matrices. In case the number of rows/columns of the target matrix
// is larger or equal to this threshold, the operation is executed in parallel. If the number of
// rows/columns is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 190. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATDMATADD_THRESHOLD = 190UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/column-major dense matrix addition threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/column-major dense matrix addition can
// be executed in parallel. This threshold affects both additions between a row-major matrix and
// a column-major matrix and a column-major matrix and a row-major matrix. In case the number of
// rows/columns of the target matrix is larger or equal to this threshold, the operation is
// executed in parallel. If the number of rows/columns is below this threshold the operation is
// executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 175. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATTDMATADD_THRESHOLD = 175UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/row-major dense matrix subtraction threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/row-major dense matrix subtraction
// can be executed in parallel. This threshold affects both subtractions between two row-major
// matrices or two column-major dense matrices. In case the number of rows/columns of the target
// matrix is larger or equal to this threshold, the operation is executed in parallel. If the
// number of rows/columns is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 190. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATDMATSUB_THRESHOLD = 190UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/column-major dense matrix subtraction threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/column-major dense matrix subtraction
// can be executed in parallel. This threshold affects both subtractions between a row-major
// matrix and a column-major matrix and a column-major matrix and a row-major matrix. In case
// the number of rows/columns of the target matrix is larger or equal to this threshold, the
// operation is executed in parallel. If the number of rows/columns is below this threshold
// the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 175. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATTDMATSUB_THRESHOLD = 175UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense matrix/scalar multiplication/division threshold.
// \ingroup config
//
// This threshold specifies when a dense matrix/scalar multiplication or division can be executed
// in parallel. In case the number of rows/columns of the target matrix is larger or equal to this
// threshold, the operation is executed in parallel. If the number of rows/columns is below this
// threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 220. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATSCALARMULT_THRESHOLD = 220UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/row-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/row-major dense matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 55. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATDMATMULT_THRESHOLD = 55UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/column-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/column-major dense matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 55. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATTDMATMULT_THRESHOLD = 55UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major dense matrix/row-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major dense matrix/row-major dense matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 55. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDMATDMATMULT_THRESHOLD = 55UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major dense matrix/column-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major dense matrix/column-major dense matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 55. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDMATTDMATMULT_THRESHOLD = 55UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/row-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/row-major sparse matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 64. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATSMATMULT_THRESHOLD = 64UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major dense matrix/column-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major dense matrix/column-major sparse matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 68. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DMATTSMATMULT_THRESHOLD = 68UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major dense matrix/row-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major dense matrix/row-major sparse matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 90. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDMATSMATMULT_THRESHOLD = 90UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major dense matrix/column-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major dense matrix/column-major sparse matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 90. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TDMATTSMATMULT_THRESHOLD = 90UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major sparse matrix/row-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major sparse matrix/row-major dense matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 88. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_SMATDMATMULT_THRESHOLD = 88UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major sparse matrix/column-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major sparse matrix/column-major dense matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 72. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_SMATTDMATMULT_THRESHOLD = 72UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major sparse matrix/row-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major sparse matrix/row-major dense matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 66. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSMATDMATMULT_THRESHOLD = 66UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major sparse matrix/column-major dense matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major sparse matrix/column-major dense matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 66. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSMATTDMATMULT_THRESHOLD = 66UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major sparse matrix/row-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major sparse matrix/row-major sparse matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 150. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_SMATSMATMULT_THRESHOLD = 150UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP row-major sparse matrix/column-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a row-major sparse matrix/column-major sparse matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 140. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_SMATTSMATMULT_THRESHOLD = 140UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major sparse matrix/row-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major sparse matrix/row-major sparse matrix multiplication
// can be executed in parallel. In case the number of rows/columns of the target matrix is larger
// or equal to this threshold, the operation is executed in parallel. If the number of rows/columns
// is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 140. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSMATSMATMULT_THRESHOLD = 140UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP column-major sparse matrix/column-major sparse matrix multiplication threshold.
// \ingroup config
//
// This threshold specifies when a column-major sparse matrix/column-major sparse matrix
// multiplication can be executed in parallel. In case the number of rows/columns of the target
// matrix is larger or equal to this threshold, the operation is executed in parallel. If the
// number of rows/columns is below this threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 150. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_TSMATTSMATMULT_THRESHOLD = 150UL;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief SMP dense vector/dense vector outer product threshold.
// \ingroup config
//
// This threshold specifies when a dense vector/dense vector outer product can be executed in
// parallel. In case the number of rows/columns of the target matrix is larger or equal to this
// threshold, the operation is executed in parallel. If the number of rows/columns is below this
// threshold the operation is executed single-threaded.
//
// Please note that this threshold is highly sensitiv to the used system architecture and the
// shared memory parallelization technique. Therefore the default value cannot guarantee maximum
// performance for all possible situations and configurations. It merely provides a reasonable
// standard for the current generation of CPUs. Also note that the provided default has been
// determined using the OpenMP parallelization and requires individual adaption for the C++11
// and Boost thread parallelization.
//
// The default setting for this threshold is 290. In case the threshold is set to 0, the operation
// is unconditionally executed in parallel.
*/
const size_t SMP_DVECTDVECMULT_THRESHOLD = 290UL;
//*************************************************************************************************

} // namespace blaze
