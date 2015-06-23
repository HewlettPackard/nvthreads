//=================================================================================================
/*!
//  \file blaze/math/smp/threads/DenseMatrix.h
//  \brief Header file for the C++11/Boost thread-based dense matrix SMP implementation
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

#ifndef _BLAZE_MATH_SMP_THREADS_DENSEMATRIX_H_
#define _BLAZE_MATH_SMP_THREADS_DENSEMATRIX_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <blaze/math/constraints/SMPAssignable.h>
#include <blaze/math/DenseSubmatrix.h>
#include <blaze/math/expressions/DenseMatrix.h>
#include <blaze/math/expressions/SparseMatrix.h>
#include <blaze/math/Functions.h>
#include <blaze/math/intrinsics/IntrinsicTrait.h>
#include <blaze/math/smp/ParallelSection.h>
#include <blaze/math/smp/SerialSection.h>
#include <blaze/math/smp/threads/ThreadBackend.h>
#include <blaze/math/SparseSubmatrix.h>
#include <blaze/math/StorageOrder.h>
#include <blaze/math/traits/SubmatrixExprTrait.h>
#include <blaze/math/typetraits/IsSMPAssignable.h>
#include <blaze/system/SMP.h>
#include <blaze/util/Assert.h>
#include <blaze/util/DisableIf.h>
#include <blaze/util/EnableIf.h>
#include <blaze/util/logging/FunctionTrace.h>
#include <blaze/util/mpl/And.h>
#include <blaze/util/StaticAssert.h>
#include <blaze/util/typetraits/IsSame.h>


namespace blaze {

//=================================================================================================
//
//  PLAIN ASSIGNMENT
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP assignment of a row-major dense matrix to a
//        dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side row-major dense matrix to be assigned.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP assignment
// of a row-major dense matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side dense matrix
void smpAssign_backend( DenseMatrix<MT1,SO>& lhs, const DenseMatrix<MT2,rowMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef IntrinsicTrait<typename MT1::ElementType>         IT;
   typedef typename SubmatrixExprTrait<MT1,aligned>::Type    AlignedTarget;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const bool vectorizable( MT1::vectorizable && MT2::vectorizable && IsSame<ET1,ET2>::value );
   const bool lhsAligned  ( (~lhs).isAligned() );
   const bool rhsAligned  ( (~rhs).isAligned() );

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).rows() % threads ) != 0UL )? 1UL : 0UL );
   const size_t equalShare   ( (~lhs).rows() / threads + addon );
   const size_t rest         ( equalShare & ( IT::size - 1UL ) );
   const size_t rowsPerThread( ( vectorizable && rest )?( equalShare - rest + IT::size ):( equalShare ) );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t row( i*rowsPerThread );

      if( row >= (~lhs).rows() )
         continue;

      const size_t m( min( rowsPerThread, (~lhs).rows() - row ) );

      if( vectorizable && lhsAligned && rhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleAssign( target, submatrix<aligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
      else if( vectorizable && lhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleAssign( target, submatrix<unaligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
      else if( vectorizable && rhsAligned ) {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleAssign( target, submatrix<aligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
      else {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleAssign( target, submatrix<unaligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP assignment of a column-major dense matrix
//        to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side column-major dense matrix to be assigned.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP assignment
// of a column-major dense matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side dense matrix
void smpAssign_backend( DenseMatrix<MT1,SO>& lhs, const DenseMatrix<MT2,columnMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef IntrinsicTrait<typename MT1::ElementType>         IT;
   typedef typename SubmatrixExprTrait<MT1,aligned>::Type    AlignedTarget;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const bool vectorizable( MT1::vectorizable && MT2::vectorizable && IsSame<ET1,ET2>::value );
   const bool lhsAligned  ( (~lhs).isAligned() );
   const bool rhsAligned  ( (~rhs).isAligned() );

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).columns() % threads ) != 0UL )? 1UL : 0UL );
   const size_t equalShare   ( (~lhs).columns() / threads + addon );
   const size_t rest         ( equalShare & ( IT::size - 1UL ) );
   const size_t colsPerThread( ( vectorizable && rest )?( equalShare - rest + IT::size ):( equalShare ) );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t column( i*colsPerThread );

      if( column >= (~lhs).columns() )
         continue;

      const size_t n( min( colsPerThread, (~lhs).columns() - column ) );

      if( vectorizable && lhsAligned && rhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleAssign( target, submatrix<aligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
      else if( vectorizable && lhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleAssign( target, submatrix<unaligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
      else if( vectorizable && rhsAligned ) {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleAssign( target, submatrix<aligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
      else {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleAssign( target, submatrix<unaligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP assignment of a row-major sparse matrix
//        to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side row-major sparse matrix to be assigned.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP assignment
// of a row-major sparse matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side sparse matrix
void smpAssign_backend( DenseMatrix<MT1,SO>& lhs, const SparseMatrix<MT2,rowMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).rows() % threads ) != 0UL )? 1UL : 0UL );
   const size_t rowsPerThread( (~lhs).rows() / threads + addon );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t row( i*rowsPerThread );

      if( row >= (~lhs).rows() )
         continue;

      const size_t m( min( rowsPerThread, (~lhs).rows() - row ) );
      UnalignedTarget target( submatrix<unaligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
      TheThreadBackend::scheduleAssign( target, submatrix<unaligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP assignment of a column-major sparse matrix
//        to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side column-major sparse matrix to be assigned.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP assignment
// of a column-major sparse matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side sparse matrix
void smpAssign_backend( DenseMatrix<MT1,SO>& lhs, const SparseMatrix<MT2,columnMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).columns() % threads ) != 0UL )? 1UL : 0UL );
   const size_t colsPerThread( (~lhs).columns() / threads + addon );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t column( i*colsPerThread );

      if( column >= (~lhs).columns() )
         continue;

      const size_t n( min( colsPerThread, (~lhs).columns() - column ) );
      UnalignedTarget target( submatrix<unaligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
      TheThreadBackend::scheduleAssign( target, submatrix<unaligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the C++11/Boost thread-based SMP assignment to a dense matrix.
// \ingroup smp
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side matrix to be assigned.
// \return void
//
// This function implements the default C++11/Boost thread-based SMP assignment to a dense matrix.
// Due to the explicit application of the SFINAE principle, this function can only be selected by
// the compiler in case both operands are SMP-assignable and the element types of both operands
// are not SMP-assignable.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1  // Type of the left-hand side dense matrix
        , bool SO1      // Storage order of the left-hand side matrix
        , typename MT2  // Type of the right-hand side matrix
        , bool SO2 >    // Storage order of the right-hand side matrix
inline typename DisableIf< And< IsSMPAssignable<MT1>, IsSMPAssignable<MT2> > >::Type
   smpAssign( DenseMatrix<MT1,SO1>& lhs, const Matrix<MT2,SO2>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == (~rhs).rows()   , "Invalid number of rows"    );
   BLAZE_INTERNAL_ASSERT( (~lhs).columns() == (~rhs).columns(), "Invalid number of columns" );

   assign( ~lhs, ~rhs );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Implementation of the C++11/Boost thread-based SMP assignment to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side column-major sparse matrix to be assigned.
// \return void
//
// This function implements the C++11/Boost thread-based SMP assignment to a dense matrix. Due
// to the explicit application of the SFINAE principle, this function can only be selected by
// the compiler in case both operands are SMP-assignable and the element types of both operands
// are not SMP-assignable.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1  // Type of the left-hand side dense matrix
        , bool SO1      // Storage order of the left-hand side matrix
        , typename MT2  // Type of the right-hand side matrix
        , bool SO2 >    // Storage order of the right-hand side matrix
inline typename EnableIf< And< IsSMPAssignable<MT1>, IsSMPAssignable<MT2> > >::Type
   smpAssign( DenseMatrix<MT1,SO1>& lhs, const Matrix<MT2,SO2>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_CONSTRAINT_MUST_NOT_BE_SMP_ASSIGNABLE( typename MT1::ElementType );
   BLAZE_CONSTRAINT_MUST_NOT_BE_SMP_ASSIGNABLE( typename MT2::ElementType );

   BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == (~rhs).rows()   , "Invalid number of rows"    );
   BLAZE_INTERNAL_ASSERT( (~lhs).columns() == (~rhs).columns(), "Invalid number of columns" );

   BLAZE_PARALLEL_SECTION
   {
      if( isSerialSectionActive() || !(~rhs).canSMPAssign() ) {
         assign( ~lhs, ~rhs );
      }
      else {
         smpAssign_backend( ~lhs, ~rhs );
      }
   }
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  ADDITION ASSIGNMENT
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP addition assignment of a row-major dense
//        matrix to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side row-major dense matrix to be added.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP addition
// assignment of a row-major dense matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side dense matrix
void smpAddAssign_backend( DenseMatrix<MT1,SO>& lhs, const DenseMatrix<MT2,rowMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef IntrinsicTrait<typename MT1::ElementType>         IT;
   typedef typename SubmatrixExprTrait<MT1,aligned>::Type    AlignedTarget;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const bool vectorizable( MT1::vectorizable && MT2::vectorizable && IsSame<ET1,ET2>::value );
   const bool lhsAligned  ( (~lhs).isAligned() );
   const bool rhsAligned  ( (~rhs).isAligned() );

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).rows() % threads ) != 0UL )? 1UL : 0UL );
   const size_t equalShare   ( (~lhs).rows() / threads + addon );
   const size_t rest         ( equalShare & ( IT::size - 1UL ) );
   const size_t rowsPerThread( ( vectorizable && rest )?( equalShare - rest + IT::size ):( equalShare ) );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t row( i*rowsPerThread );

      if( row >= (~lhs).rows() )
         continue;

      const size_t m( min( rowsPerThread, (~lhs).rows() - row ) );

      if( vectorizable && lhsAligned && rhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleAddAssign( target, submatrix<aligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
      else if( vectorizable && lhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleAddAssign( target, submatrix<unaligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
      else if( vectorizable && rhsAligned ) {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleAddAssign( target, submatrix<aligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
      else {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleAddAssign( target, submatrix<unaligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP addition assignment of a column-major dense
//        matrix to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side column-major dense matrix to be added.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP addition
// assignment of a column-major dense matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side dense matrix
void smpAddAssign_backend( DenseMatrix<MT1,SO>& lhs, const DenseMatrix<MT2,columnMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef IntrinsicTrait<typename MT1::ElementType>         IT;
   typedef typename SubmatrixExprTrait<MT1,aligned>::Type    AlignedTarget;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const bool vectorizable( MT1::vectorizable && MT2::vectorizable && IsSame<ET1,ET2>::value );
   const bool lhsAligned  ( (~lhs).isAligned() );
   const bool rhsAligned  ( (~rhs).isAligned() );

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).columns() % threads ) != 0UL )? 1UL : 0UL );
   const size_t equalShare   ( (~lhs).columns() / threads + addon );
   const size_t rest         ( equalShare & ( IT::size - 1UL ) );
   const size_t colsPerThread( ( vectorizable && rest )?( equalShare - rest + IT::size ):( equalShare ) );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t column( i*colsPerThread );

      if( column >= (~lhs).columns() )
         continue;

      const size_t n( min( colsPerThread, (~lhs).columns() - column ) );

      if( vectorizable && lhsAligned && rhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleAddAssign( target, submatrix<aligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
      else if( vectorizable && lhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleAddAssign( target, submatrix<unaligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
      else if( vectorizable && rhsAligned ) {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleAddAssign( target, submatrix<aligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
      else {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleAddAssign( target, submatrix<unaligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP addition assignment of a row-major sparse
//        matrix to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side row-major sparse matrix to be added.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP addition
// assignment of a row-major sparse matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side sparse matrix
void smpAddAssign_backend( DenseMatrix<MT1,SO>& lhs, const SparseMatrix<MT2,rowMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).rows() % threads ) != 0UL )? 1UL : 0UL );
   const size_t rowsPerThread( (~lhs).rows() / threads + addon );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t row( i*rowsPerThread );

      if( row >= (~lhs).rows() )
         continue;

      const size_t m( min( rowsPerThread, (~lhs).rows() - row ) );
      UnalignedTarget target( submatrix<unaligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
      TheThreadBackend::scheduleAddAssign( target, submatrix<unaligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP addition assignment of a column-major sparse
//        matrix to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side column-major sparse matrix to be added.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP addition
// assignment of a column-major sparse matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side sparse matrix
void smpAddAssign_backend( DenseMatrix<MT1,SO>& lhs, const SparseMatrix<MT2,columnMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).columns() % threads ) != 0UL )? 1UL : 0UL );
   const size_t colsPerThread( (~lhs).columns() / threads + addon );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t column( i*colsPerThread );

      if( column >= (~lhs).columns() )
         continue;

      const size_t n( min( colsPerThread, (~lhs).columns() - column ) );
      UnalignedTarget target( submatrix<unaligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
      TheThreadBackend::scheduleAddAssign( target, submatrix<unaligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the C++11/Boost thread-based SMP addition assignment to a
//        dense matrix.
// \ingroup smp
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side matrix to be added.
// \return void
//
// This function implements the default C++11/Boost thread-based SMP addition assignment to a
// dense matrix. Due to the explicit application of the SFINAE principle, this function can only
// be selected by the compiler in case both operands are SMP-assignable and the element types of
// both operands are not SMP-assignable.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1  // Type of the left-hand side dense matrix
        , bool SO1      // Storage order of the left-hand side matrix
        , typename MT2  // Type of the right-hand side matrix
        , bool SO2 >    // Storage order of the right-hand side matrix
inline typename DisableIf< And< IsSMPAssignable<MT1>, IsSMPAssignable<MT2> > >::Type
   smpAddAssign( DenseMatrix<MT1,SO1>& lhs, const Matrix<MT2,SO2>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == (~rhs).rows()   , "Invalid number of rows"    );
   BLAZE_INTERNAL_ASSERT( (~lhs).columns() == (~rhs).columns(), "Invalid number of columns" );

   addAssign( ~lhs, ~rhs );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Implementation of the C++11/Boost thread-based SMP addition assignment to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side row-major dense matrix to be added.
// \return void
//
// This function implements the C++11/Boost thread-based SMP addition assignment to a dense matrix.
// Due to the explicit application of the SFINAE principle, this function can only be selected by
// the compiler in case both operands are SMP-assignable and the element types of both operands
// are not SMP-assignable.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1  // Type of the left-hand side dense matrix
        , bool SO1      // Storage order of the left-hand side matrix
        , typename MT2  // Type of the right-hand side matrix
        , bool SO2 >    // Storage order of the right-hand side matrix
inline typename EnableIf< And< IsSMPAssignable<MT1>, IsSMPAssignable<MT2> > >::Type
   smpAddAssign( DenseMatrix<MT1,SO1>& lhs, const Matrix<MT2,SO2>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_CONSTRAINT_MUST_NOT_BE_SMP_ASSIGNABLE( typename MT1::ElementType );
   BLAZE_CONSTRAINT_MUST_NOT_BE_SMP_ASSIGNABLE( typename MT2::ElementType );

   BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == (~rhs).rows()   , "Invalid number of rows"    );
   BLAZE_INTERNAL_ASSERT( (~lhs).columns() == (~rhs).columns(), "Invalid number of columns" );

   BLAZE_PARALLEL_SECTION
   {
      if( isSerialSectionActive() || !(~rhs).canSMPAssign() ) {
         addAssign( ~lhs, ~rhs );
      }
      else {
         smpAddAssign_backend( ~lhs, ~rhs );
      }
   }
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  SUBTRACTION ASSIGNMENT
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP subtraction assignment of a row-major dense
//        matrix to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side row-major dense matrix to be subtracted.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP subtraction
// assignment of a row-major dense matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side dense matrix
void smpSubAssign_backend( DenseMatrix<MT1,SO>& lhs, const DenseMatrix<MT2,rowMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef IntrinsicTrait<typename MT1::ElementType>         IT;
   typedef typename SubmatrixExprTrait<MT1,aligned>::Type    AlignedTarget;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const bool vectorizable( MT1::vectorizable && MT2::vectorizable && IsSame<ET1,ET2>::value );
   const bool lhsAligned  ( (~lhs).isAligned() );
   const bool rhsAligned  ( (~rhs).isAligned() );

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).rows() % threads ) != 0UL )? 1UL : 0UL );
   const size_t equalShare   ( (~lhs).rows() / threads + addon );
   const size_t rest         ( equalShare & ( IT::size - 1UL ) );
   const size_t rowsPerThread( ( vectorizable && rest )?( equalShare - rest + IT::size ):( equalShare ) );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t row( i*rowsPerThread );

      if( row >= (~lhs).rows() )
         continue;

      const size_t m( min( rowsPerThread, (~lhs).rows() - row ) );

      if( vectorizable && lhsAligned && rhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleSubAssign( target, submatrix<aligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
      else if( vectorizable && lhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleSubAssign( target, submatrix<unaligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
      else if( vectorizable && rhsAligned ) {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleSubAssign( target, submatrix<aligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
      else {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
         TheThreadBackend::scheduleSubAssign( target, submatrix<unaligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
      }
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP subtraction assignment of a column-major
//        dense matrix to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side column-major dense matrix to be subtracted.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP subtraction
// assignment of a column-major dense matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side dense matrix
void smpSubAssign_backend( DenseMatrix<MT1,SO>& lhs, const DenseMatrix<MT2,columnMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef IntrinsicTrait<typename MT1::ElementType>         IT;
   typedef typename SubmatrixExprTrait<MT1,aligned>::Type    AlignedTarget;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const bool vectorizable( MT1::vectorizable && MT2::vectorizable && IsSame<ET1,ET2>::value );
   const bool lhsAligned  ( (~lhs).isAligned() );
   const bool rhsAligned  ( (~rhs).isAligned() );

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).columns() % threads ) != 0UL )? 1UL : 0UL );
   const size_t equalShare   ( (~lhs).columns() / threads + addon );
   const size_t rest         ( equalShare & ( IT::size - 1UL ) );
   const size_t colsPerThread( ( vectorizable && rest )?( equalShare - rest + IT::size ):( equalShare ) );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t column( i*colsPerThread );

      if( column >= (~lhs).columns() )
         continue;

      const size_t n( min( colsPerThread, (~lhs).columns() - column ) );

      if( vectorizable && lhsAligned && rhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleSubAssign( target, submatrix<aligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
      else if( vectorizable && lhsAligned ) {
         AlignedTarget target( submatrix<aligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleSubAssign( target, submatrix<unaligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
      else if( vectorizable && rhsAligned ) {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleSubAssign( target, submatrix<aligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
      else {
         UnalignedTarget target( submatrix<unaligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
         TheThreadBackend::scheduleSubAssign( target, submatrix<unaligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
      }
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP subtraction assignment of a row-major sparse
//        matrix to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side row-major sparse matrix to be subtracted.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP subtraction
// assignment of a row-major sparse matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side sparse matrix
void smpSubAssign_backend( DenseMatrix<MT1,SO>& lhs, const SparseMatrix<MT2,rowMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).rows() % threads ) != 0UL )? 1UL : 0UL );
   const size_t rowsPerThread( (~lhs).rows() / threads + addon );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t row( i*rowsPerThread );

      if( row >= (~lhs).rows() )
         continue;

      const size_t m( min( rowsPerThread, (~lhs).rows() - row ) );
      UnalignedTarget target( submatrix<unaligned>( ~lhs, row, 0UL, m, (~lhs).columns() ) );
      TheThreadBackend::scheduleSubAssign( target, submatrix<unaligned>( ~rhs, row, 0UL, m, (~lhs).columns() ) );
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Backend of the C++11/Boost thread-based SMP subtraction assignment of a column-major
//        sparse matrix to a dense matrix.
// \ingroup math
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side column-major sparse matrix to be subtracted.
// \return void
//
// This function is the backend implementation of the C++11/Boost thread-based SMP subtraction
// assignment of a column-major sparse matrix to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1    // Type of the left-hand side dense matrix
        , bool SO         // Storage order of the left-hand side dense matrix
        , typename MT2 >  // Type of the right-hand side sparse matrix
void smpSubAssign_backend( DenseMatrix<MT1,SO>& lhs, const SparseMatrix<MT2,columnMajor>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( isParallelSectionActive(), "Invalid call outside a parallel section" );

   typedef typename MT1::ElementType                         ET1;
   typedef typename MT2::ElementType                         ET2;
   typedef typename SubmatrixExprTrait<MT1,unaligned>::Type  UnalignedTarget;

   const size_t threads      ( TheThreadBackend::size() );
   const size_t addon        ( ( ( (~lhs).columns() % threads ) != 0UL )? 1UL : 0UL );
   const size_t colsPerThread( (~lhs).columns() / threads + addon );

   for( size_t i=0UL; i<threads; ++i )
   {
      const size_t column( i*colsPerThread );

      if( column >= (~lhs).columns() )
         continue;

      const size_t n( min( colsPerThread, (~lhs).columns() - column ) );
      UnalignedTarget target( submatrix<unaligned>( ~lhs, 0UL, column, (~lhs).rows(), n ) );
      TheThreadBackend::scheduleSubAssign( target, submatrix<unaligned>( ~rhs, 0UL, column, (~lhs).rows(), n ) );
   }

   TheThreadBackend::wait();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the C++11/Boost thread-based SMP subtracction assignment to a
//        dense matrix.
// \ingroup smp
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side matrix to be subtracted.
// \return void
//
// This function implements the default C++11/Boost thread-based SMP subtraction assignment to a
// dense matrix. Due to the explicit application of the SFINAE principle, this function can only
// be selected by the compiler in case both operands are SMP-assignable and the element types of
// both operands are not SMP-assignable.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1  // Type of the left-hand side dense matrix
        , bool SO1      // Storage order of the left-hand side matrix
        , typename MT2  // Type of the right-hand side matrix
        , bool SO2 >    // Storage order of the right-hand side matrix
inline typename DisableIf< And< IsSMPAssignable<MT1>, IsSMPAssignable<MT2> > >::Type
   smpSubAssign( DenseMatrix<MT1,SO1>& lhs, const Matrix<MT2,SO2>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == (~rhs).rows()   , "Invalid number of rows"    );
   BLAZE_INTERNAL_ASSERT( (~lhs).columns() == (~rhs).columns(), "Invalid number of columns" );

   subAssign( ~lhs, ~rhs );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Implementation of the C++11/Boost thread-based SMP subtracction assignment to a dense
//        matrix.
// \ingroup smp
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side matrix to be subtracted.
// \return void
//
// This function implements the default C++11/Boost thread-based SMP subtraction assignment of a
// matrix to a dense matrix. Due to the explicit application of the SFINAE principle, this function
// can only be selected by the compiler in case both operands are SMP-assignable and the element
// types of both operands are not SMP-assignable.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1  // Type of the left-hand side dense matrix
        , bool SO1      // Storage order of the left-hand side matrix
        , typename MT2  // Type of the right-hand side matrix
        , bool SO2 >    // Storage order of the right-hand side matrix
inline typename EnableIf< And< IsSMPAssignable<MT1>, IsSMPAssignable<MT2> > >::Type
   smpSubAssign( DenseMatrix<MT1,SO1>& lhs, const Matrix<MT2,SO2>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_CONSTRAINT_MUST_NOT_BE_SMP_ASSIGNABLE( typename MT1::ElementType );
   BLAZE_CONSTRAINT_MUST_NOT_BE_SMP_ASSIGNABLE( typename MT2::ElementType );

   BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == (~rhs).rows()   , "Invalid number of rows"    );
   BLAZE_INTERNAL_ASSERT( (~lhs).columns() == (~rhs).columns(), "Invalid number of columns" );

   BLAZE_PARALLEL_SECTION
   {
      if( isSerialSectionActive() || !(~rhs).canSMPAssign() ) {
         subAssign( ~lhs, ~rhs );
      }
      else {
         smpSubAssign_backend( ~lhs, ~rhs );
      }
   }
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  MULTIPLICATION ASSIGNMENT
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the C++11/Boost thread-based SMP multiplication assignment
//        to a dense matrix.
// \ingroup smp
//
// \param lhs The target left-hand side dense matrix.
// \param rhs The right-hand side matrix to be multiplied.
// \return void
//
// This function implements the default C++11/Boost thread-based SMP multiplication assignment
// to a dense matrix.\n
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename MT1  // Type of the left-hand side dense matrix
        , bool SO1      // Storage order of the left-hand side matrix
        , typename MT2  // Type of the right-hand side matrix
        , bool SO2 >    // Storage order of the right-hand side matrix
inline void smpMultAssign( DenseMatrix<MT1,SO1>& lhs, const Matrix<MT2,SO2>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == (~rhs).rows()   , "Invalid number of rows"    );
   BLAZE_INTERNAL_ASSERT( (~lhs).columns() == (~rhs).columns(), "Invalid number of columns" );

   multAssign( ~lhs, ~rhs );
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  COMPILE TIME CONSTRAINT
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
namespace {

BLAZE_STATIC_ASSERT( BLAZE_CPP_THREADS_PARALLEL_MODE || BLAZE_BOOST_THREADS_PARALLEL_MODE );

}
/*! \endcond */
//*************************************************************************************************

} // namespace blaze

#endif
