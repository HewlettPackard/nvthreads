//=================================================================================================
/*!
//  \file blaze/math/dense/HybridMatrix.h
//  \brief Header file for the implementation of a fixed-size matrix
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

#ifndef _BLAZE_MATH_DENSE_HYBRIDMATRIX_H_
#define _BLAZE_MATH_DENSE_HYBRIDMATRIX_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <blaze/math/dense/DenseIterator.h>
#include <blaze/math/expressions/DenseMatrix.h>
#include <blaze/math/expressions/SparseMatrix.h>
#include <blaze/math/Forward.h>
#include <blaze/math/Functions.h>
#include <blaze/math/Intrinsics.h>
#include <blaze/math/shims/IsDefault.h>
#include <blaze/math/shims/Reset.h>
#include <blaze/math/traits/AddTrait.h>
#include <blaze/math/traits/ColumnTrait.h>
#include <blaze/math/traits/DivTrait.h>
#include <blaze/math/traits/MathTrait.h>
#include <blaze/math/traits/MultTrait.h>
#include <blaze/math/traits/RowTrait.h>
#include <blaze/math/traits/SubmatrixTrait.h>
#include <blaze/math/traits/SubTrait.h>
#include <blaze/math/typetraits/IsColumnMajorMatrix.h>
#include <blaze/math/typetraits/IsRowMajorMatrix.h>
#include <blaze/math/typetraits/IsSparseMatrix.h>
#include <blaze/system/StorageOrder.h>
#include <blaze/util/AlignedArray.h>
#include <blaze/util/Assert.h>
#include <blaze/util/constraints/Const.h>
#include <blaze/util/constraints/FloatingPoint.h>
#include <blaze/util/constraints/Numeric.h>
#include <blaze/util/constraints/Pointer.h>
#include <blaze/util/constraints/Reference.h>
#include <blaze/util/constraints/Vectorizable.h>
#include <blaze/util/constraints/Volatile.h>
#include <blaze/util/DisableIf.h>
#include <blaze/util/EnableIf.h>
#include <blaze/util/Memory.h>
#include <blaze/util/StaticAssert.h>
#include <blaze/util/Template.h>
#include <blaze/util/Types.h>
#include <blaze/util/typetraits/IsNumeric.h>
#include <blaze/util/typetraits/IsSame.h>
#include <blaze/util/typetraits/IsVectorizable.h>
#include <blaze/util/Unused.h>


namespace blaze {

//=================================================================================================
//
//  CLASS DEFINITION
//
//=================================================================================================

//*************************************************************************************************
/*!\defgroup hybrid_matrix HybridMatrix
// \ingroup dense_matrix
*/
/*!\brief Efficient implementation of a dynamically sized matrix with static memory.
// \ingroup hybrid_matrix
//
// The HybridMatrix class template combines the flexibility of a dynamically sized matrix with
// the efficiency and performance of a fixed-size matrix. It is implemented as a crossing between
// the blaze::StaticMatrix and the blaze::DynamicMatrix class templates: Similar to the static
// matrix it uses static stack memory instead of dynamically allocated memory and similar to the
// dynamic matrix it can be resized (within the extend of the static memory). The type of the
// elements, the maximum number of rows and columns and the storage order of the matrix can be
// specified via the four template parameters:

   \code
   template< typename Type, size_t M, size_t N, bool SO >
   class HybridMatrix;
   \endcode

//  - Type: specifies the type of the matrix elements. HybridMatrix can be used with any
//          non-cv-qualified, non-reference, non-pointer element type.
//  - M   : specifies the maximum number of rows of the matrix.
//  - N   : specifies the maximum number of columns of the matrix. Note that it is expected
//          that HybridMatrix is only used for tiny and small matrices.
//  - SO  : specifies the storage order (blaze::rowMajor, blaze::columnMajor) of the matrix.
//          The default value is blaze::rowMajor.
//
// Depending on the storage order, the matrix elements are either stored in a row-wise fashion
// or in a column-wise fashion. Given the 2x3 matrix

                          \f[\left(\begin{array}{*{3}{c}}
                          1 & 2 & 3 \\
                          4 & 5 & 6 \\
                          \end{array}\right)\f]\n

// in case of row-major order the elements are stored in the order

                          \f[\left(\begin{array}{*{6}{c}}
                          1 & 2 & 3 & 4 & 5 & 6. \\
                          \end{array}\right)\f]

// In case of column-major order the elements are stored in the order

                          \f[\left(\begin{array}{*{6}{c}}
                          1 & 4 & 2 & 5 & 3 & 6. \\
                          \end{array}\right)\f]

// The use of HybridMatrix is very natural and intuitive. All operations (addition, subtraction,
// multiplication, scaling, ...) can be performed on all possible combination of row-major and
// column-major dense and sparse matrices with fitting element types. The following example gives
// an impression of the use of HybridMatrix:

   \code
   using blaze::HybridMatrix;
   using blaze::CompressedMatrix;
   using blaze::rowMajor;
   using blaze::columnMajor;

   HybridMatrix<double,rowMajor> A( 2, 3 );   // Default constructed, non-initialized, row-major 2x3 matrix
   A(0,0) = 1.0; A(0,1) = 2.0; A(0,2) = 3.0;  // Initialization of the first row
   A(1,0) = 4.0; A(1,1) = 5.0; A(1,2) = 6.0;  // Initialization of the second row

   HybridMatrix<float,columnMajor> B( 2, 3 );   // Default constructed column-major single precision 2x3 matrix
   B(0,0) = 1.0; B(0,1) = 3.0; B(0,2) = 5.0;    // Initialization of the first row
   B(1,0) = 2.0; B(1,1) = 4.0; B(1,2) = 6.0;    // Initialization of the second row

   CompressedMatrix<float> C( 2, 3 );        // Empty row-major sparse single precision matrix
   HybridMatrix<float>     D( 3, 2, 4.0F );  // Directly, homogeneously initialized single precision 3x2 matrix

   HybridMatrix<double,rowMajor>    E( A );  // Creation of a new row-major matrix as a copy of A
   HybridMatrix<double,columnMajor> F;       // Creation of a default column-major matrix

   E = A + B;     // Matrix addition and assignment to a row-major matrix
   E = A - C;     // Matrix subtraction and assignment to a column-major matrix
   F = A * D;     // Matrix multiplication between two matrices of different element types

   A *= 2.0;      // In-place scaling of matrix A
   E  = 2.0 * B;  // Scaling of matrix B
   F  = D * 2.0;  // Scaling of matrix D

   E += A - B;    // Addition assignment
   E -= A + C;    // Subtraction assignment
   F *= A * D;    // Multiplication assignment
   \endcode
*/
template< typename Type                    // Data type of the matrix
        , size_t M                         // Number of rows
        , size_t N                         // Number of columns
        , bool SO = defaultStorageOrder >  // Storage order
class HybridMatrix : public DenseMatrix< HybridMatrix<Type,M,N,SO>, SO >
{
 private:
   //**Type definitions****************************************************************************
   typedef IntrinsicTrait<Type>  IT;  //!< Intrinsic trait for the matrix element type.
   //**********************************************************************************************

   //**********************************************************************************************
   //! Alignment adjustment
   enum { NN = N + ( IT::size - ( N % IT::size ) ) % IT::size };
   //**********************************************************************************************

 public:
   //**Type definitions****************************************************************************
   typedef HybridMatrix<Type,M,N,SO>   This;            //!< Type of this HybridMatrix instance.
   typedef This                        ResultType;      //!< Result type for expression template evaluations.
   typedef HybridMatrix<Type,M,N,!SO>  OppositeType;    //!< Result type with opposite storage order for expression template evaluations.
   typedef HybridMatrix<Type,N,M,!SO>  TransposeType;   //!< Transpose type for expression template evaluations.
   typedef Type                        ElementType;     //!< Type of the matrix elements.
   typedef typename IT::Type           IntrinsicType;   //!< Intrinsic type of the matrix elements.
   typedef const Type&                 ReturnType;      //!< Return type for expression template evaluations.
   typedef const This&                 CompositeType;   //!< Data type for composite expression templates.
   typedef Type&                       Reference;       //!< Reference to a non-constant matrix value.
   typedef const Type&                 ConstReference;  //!< Reference to a constant matrix value.
   typedef Type*                       Pointer;         //!< Pointer to a non-constant matrix value.
   typedef const Type*                 ConstPointer;    //!< Pointer to a constant matrix value.
   typedef DenseIterator<Type>         Iterator;        //!< Iterator over non-constant elements.
   typedef DenseIterator<const Type>   ConstIterator;   //!< Iterator over constant elements.
   //**********************************************************************************************

   //**Compilation flags***************************************************************************
   //! Compilation flag for intrinsic optimization.
   /*! The \a vectorizable compilation flag indicates whether expressions the matrix is involved
       in can be optimized via intrinsics. In case the element type of the matrix is a vectorizable
       data type, the \a vectorizable compilation flag is set to \a true, otherwise it is set to
       \a false. */
   enum { vectorizable = IsVectorizable<Type>::value };

   //! Compilation flag for SMP assignments.
   /*! The \a smpAssignable compilation flag indicates whether the matrix can be used in SMP
       (shared memory parallel) assignments (both on the left-hand and right-hand side of the
       assignment). */
   enum { smpAssignable = 0 };
   //**********************************************************************************************

   //**Constructors********************************************************************************
   /*!\name Constructors */
   //@{
                              explicit inline HybridMatrix();
                              explicit inline HybridMatrix( size_t m, size_t n );
                              explicit inline HybridMatrix( size_t m, size_t n, const Type& init );
   template< typename Other > explicit inline HybridMatrix( size_t m, size_t n, const Other* array );

   template< typename Other, size_t M2, size_t N2 >
   explicit inline HybridMatrix( const Other (&array)[M2][N2] );

                                     inline HybridMatrix( const HybridMatrix& m );
   template< typename MT, bool SO2 > inline HybridMatrix( const Matrix<MT,SO2>& m );
   //@}
   //**********************************************************************************************

   //**Destructor**********************************************************************************
   // No explicitly declared destructor.
   //**********************************************************************************************

   //**Data access functions***********************************************************************
   /*!\name Data access functions */
   //@{
   inline Reference      operator()( size_t i, size_t j );
   inline ConstReference operator()( size_t i, size_t j ) const;
   inline Pointer        data  ();
   inline ConstPointer   data  () const;
   inline Pointer        data  ( size_t i );
   inline ConstPointer   data  ( size_t i ) const;
   inline Iterator       begin ( size_t i );
   inline ConstIterator  begin ( size_t i ) const;
   inline ConstIterator  cbegin( size_t i ) const;
   inline Iterator       end   ( size_t i );
   inline ConstIterator  end   ( size_t i ) const;
   inline ConstIterator  cend  ( size_t i ) const;
   //@}
   //**********************************************************************************************

   //**Assignment operators************************************************************************
   /*!\name Assignment operators */
   //@{
   template< typename Other, size_t M2, size_t N2 >
   inline HybridMatrix& operator=( const Other (&array)[M2][N2] );

                                     inline HybridMatrix& operator= ( const Type& set );
                                     inline HybridMatrix& operator= ( const HybridMatrix& rhs );
   template< typename MT, bool SO2 > inline HybridMatrix& operator= ( const Matrix<MT,SO2>& rhs );
   template< typename MT, bool SO2 > inline HybridMatrix& operator+=( const Matrix<MT,SO2>& rhs );
   template< typename MT, bool SO2 > inline HybridMatrix& operator-=( const Matrix<MT,SO2>& rhs );
   template< typename MT, bool SO2 > inline HybridMatrix& operator*=( const Matrix<MT,SO2>& rhs );

   template< typename Other >
   inline typename EnableIf< IsNumeric<Other>, HybridMatrix >::Type&
      operator*=( Other rhs );

   template< typename Other >
   inline typename EnableIf< IsNumeric<Other>, HybridMatrix >::Type&
      operator/=( Other rhs );
   //@}
   //**********************************************************************************************

   //**Utility functions***************************************************************************
   /*!\name Utility functions */
   //@{
                              inline size_t        rows() const;
                              inline size_t        columns() const;
                              inline size_t        spacing() const;
                              inline size_t        capacity() const;
                              inline size_t        capacity( size_t i ) const;
                              inline size_t        nonZeros() const;
                              inline size_t        nonZeros( size_t i ) const;
                              inline void          reset();
                              inline void          reset( size_t i );
                              inline void          clear();
                                     void          resize ( size_t m, size_t n, bool preserve=true );
                              inline void          extend ( size_t m, size_t n, bool preserve=true );
                              inline HybridMatrix& transpose();
   template< typename Other > inline HybridMatrix& scale( const Other& scalar );
                              inline void          swap( HybridMatrix& m ) /* throw() */;
   //@}
   //**********************************************************************************************

   //**Memory functions****************************************************************************
   /*!\name Memory functions */
   //@{
   static inline void* operator new  ( std::size_t size );
   static inline void* operator new[]( std::size_t size );
   static inline void* operator new  ( std::size_t size, const std::nothrow_t& );
   static inline void* operator new[]( std::size_t size, const std::nothrow_t& );

   static inline void operator delete  ( void* ptr );
   static inline void operator delete[]( void* ptr );
   static inline void operator delete  ( void* ptr, const std::nothrow_t& );
   static inline void operator delete[]( void* ptr, const std::nothrow_t& );
   //@}
   //**********************************************************************************************

 private:
   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename MT >
   struct VectorizedAssign {
      enum { value = vectorizable && MT::vectorizable &&
                     IsSame<Type,typename MT::ElementType>::value &&
                     IsRowMajorMatrix<MT>::value };
   };
   /*! \endcond */
   //**********************************************************************************************

   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename MT >
   struct VectorizedAddAssign {
      enum { value = vectorizable && MT::vectorizable &&
                     IsSame<Type,typename MT::ElementType>::value &&
                     IntrinsicTrait<Type>::addition &&
                     IsRowMajorMatrix<MT>::value };
   };
   /*! \endcond */
   //**********************************************************************************************

   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename MT >
   struct VectorizedSubAssign {
      enum { value = vectorizable && MT::vectorizable &&
                     IsSame<Type,typename MT::ElementType>::value &&
                     IntrinsicTrait<Type>::subtraction &&
                     IsRowMajorMatrix<MT>::value };
   };
   /*! \endcond */
   //**********************************************************************************************

 public:
   //**Expression template evaluation functions****************************************************
   /*!\name Expression template evaluation functions */
   //@{
   template< typename Other > inline bool canAlias ( const Other* alias ) const;
   template< typename Other > inline bool isAliased( const Other* alias ) const;

   inline bool isAligned() const;

   inline IntrinsicType load  ( size_t i, size_t j ) const;
   inline IntrinsicType loadu ( size_t i, size_t j ) const;
   inline void          store ( size_t i, size_t j, const IntrinsicType& value );
   inline void          storeu( size_t i, size_t j, const IntrinsicType& value );
   inline void          stream( size_t i, size_t j, const IntrinsicType& value );

   template< typename MT, bool SO2 >
   inline typename DisableIf< VectorizedAssign<MT> >::Type
      assign( const DenseMatrix<MT,SO2>& rhs );

   template< typename MT, bool SO2 >
   inline typename EnableIf< VectorizedAssign<MT> >::Type
      assign( const DenseMatrix<MT,SO2>& rhs );

   template< typename MT > inline void assign( const SparseMatrix<MT,SO>&  rhs );
   template< typename MT > inline void assign( const SparseMatrix<MT,!SO>& rhs );

   template< typename MT, bool SO2 >
   inline typename DisableIf< VectorizedAddAssign<MT> >::Type
      addAssign( const DenseMatrix<MT,SO2>& rhs );

   template< typename MT, bool SO2 >
   inline typename EnableIf< VectorizedAddAssign<MT> >::Type
      addAssign( const DenseMatrix<MT,SO2>& rhs );

   template< typename MT > inline void addAssign( const SparseMatrix<MT,SO>&  rhs );
   template< typename MT > inline void addAssign( const SparseMatrix<MT,!SO>& rhs );

   template< typename MT, bool SO2 >
   inline typename DisableIf< VectorizedSubAssign<MT> >::Type
      subAssign( const DenseMatrix<MT,SO2>& rhs );

   template< typename MT, bool SO2 >
   inline typename EnableIf< VectorizedSubAssign<MT> >::Type
      subAssign( const DenseMatrix<MT,SO2>& rhs );

   template< typename MT > inline void subAssign( const SparseMatrix<MT,SO>&  rhs );
   template< typename MT > inline void subAssign( const SparseMatrix<MT,!SO>& rhs );
   //@}
   //**********************************************************************************************

 private:
   //**Member variables****************************************************************************
   /*!\name Member variables */
   //@{
   AlignedArray<Type,M*NN> v_;  //!< The statically allocated matrix elements.
                                /*!< Access to the matrix elements is gained via the function call
                                     operator. In case of row-major order the memory layout of the
                                     elements is
                                     \f[\left(\begin{array}{*{5}{c}}
                                     0            & 1             & 2             & \cdots & N-1         \\
                                     N            & N+1           & N+2           & \cdots & 2 \cdot N-1 \\
                                     \vdots       & \vdots        & \vdots        & \ddots & \vdots      \\
                                     M \cdot N-N  & M \cdot N-N+1 & M \cdot N-N+2 & \cdots & M \cdot N-1 \\
                                     \end{array}\right)\f]. */
   size_t m_;                   //!< The current number of rows of the matrix.
   size_t n_;                   //!< The current number of columns of the matrix.
   //@}
   //**********************************************************************************************

   //**Compile time checks*************************************************************************
   /*! \cond BLAZE_INTERNAL */
   BLAZE_CONSTRAINT_MUST_NOT_BE_POINTER_TYPE  ( Type );
   BLAZE_CONSTRAINT_MUST_NOT_BE_REFERENCE_TYPE( Type );
   BLAZE_CONSTRAINT_MUST_NOT_BE_CONST         ( Type );
   BLAZE_CONSTRAINT_MUST_NOT_BE_VOLATILE      ( Type );
   BLAZE_STATIC_ASSERT( NN % IT::size == 0UL );
   BLAZE_STATIC_ASSERT( NN >= N );
   /*! \endcond */
   //**********************************************************************************************
};
//*************************************************************************************************




//=================================================================================================
//
//  CONSTRUCTORS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief The default constructor for HybridMatrix.
//
// The size of a default constructed HybridMatrix is initially set to 0.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline HybridMatrix<Type,M,N,SO>::HybridMatrix()
   : v_()       // The statically allocated matrix elements
   , m_( 0UL )  // The current number of rows of the matrix
   , n_( 0UL )  // The current number of columns of the matrix
{
   if( IsVectorizable<Type>::value ) {
      for( size_t i=0UL; i<M*NN; ++i )
         v_[i] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Constructor for a matrix of size \f$ m \times n \f$. No element initialization is performed!
//
// \param m The number of rows of the matrix.
// \param n The number of columns of the matrix.
// \exception std::invalid_argument Invalid number of rows for hybrid matrix.
// \exception std::invalid_argument Invalid number of columns for hybrid matrix.
//
// This constructor creates a hybrid matrix of size \f$ m \times n \f$, but leaves the elements
// uninitialized. In case \a m is larger than the maximum allowed number of rows (i.e. \a m > M)
// or \a n is larger than the maximum allowed number of columns a \a std::invalid_argument
// exception is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline HybridMatrix<Type,M,N,SO>::HybridMatrix( size_t m, size_t n )
   : v_()     // The statically allocated matrix elements
   , m_( m )  // The current number of rows of the matrix
   , n_( n )  // The current number of columns of the matrix
{
   if( m > M )
      throw std::invalid_argument( "Invalid number of rows for hybrid matrix" );

   if( n > N )
      throw std::invalid_argument( "Invalid number of columns for hybrid matrix" );

   if( IsVectorizable<Type>::value ) {
      for( size_t i=0UL; i<M*NN; ++i )
         v_[i] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Constructor for a homogenous initialization of all \f$ m \times n \f$ matrix elements.
//
// \param m The number of rows of the matrix.
// \param n The number of columns of the matrix.
// \param init The initial value of the matrix elements.
// \exception std::invalid_argument Invalid number of rows for hybrid matrix.
// \exception std::invalid_argument Invalid number of columns for hybrid matrix.
//
// This constructor creates a hybrid matrix of size \f$ m \times n \f$ and initializes all
// matrix elements with the specified value. In case \a m is larger than the maximum allowed
// number of rows (i.e. \a m > M) or \a n is larger than the maximum allowed number of columns
// a \a std::invalid_argument exception is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline HybridMatrix<Type,M,N,SO>::HybridMatrix( size_t m, size_t n, const Type& init )
   : v_()     // The statically allocated matrix elements
   , m_( m )  // The current number of rows of the matrix
   , n_( n )  // The current number of columns of the matrix
{
   if( m > M )
      throw std::invalid_argument( "Invalid number of rows for hybrid matrix" );

   if( n > N )
      throw std::invalid_argument( "Invalid number of columns for hybrid matrix" );

   for( size_t i=0UL; i<m; ++i ) {
      for( size_t j=0UL; j<n; ++j )
         v_[i*NN+j] = init;

      if( IsVectorizable<Type>::value ) {
         for( size_t j=n; j<NN; ++j )
            v_[i*NN+j] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t i=m; i<M; ++i )
         for( size_t j=0UL; j<NN; ++j )
            v_[i*NN+j] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Array initialization of all matrix elements.
//
// \param m The number of rows of the matrix.
// \param n The number of columns of the matrix.
// \param array Dynamic array for the initialization.
// \exception std::invalid_argument Invalid number of rows for hybrid matrix.
// \exception std::invalid_argument Invalid number of columns for hybrid matrix.
//
// This constructor offers the option to directly initialize the elements of the matrix with
// a dynamic array:

   \code
   using blaze::rowMajor;

   int* array = new int[20];
   // ... Initialization of the dynamic array
   blaze::HybridMatrix<int,rowMajor> v( 4UL, 5UL, array );
   delete[] array;
   \endcode

// The matrix is sized accoring to the given size of the array and initialized with the values
// from the given array. In case \a m is larger than the maximum allowed number of rows (i.e.
// \a m > M) or \a n is larger than the maximum allowed number of columns a \a std::invalid_argument
// exception is thrown. Note that it is expected that the given \a array has at least \a m by
// \a n elements. Providing an array with less elements results in undefined behavior!
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N          // Number of columns
        , bool SO >         // Storage order
template< typename Other >  // Data type of the initialization array
inline HybridMatrix<Type,M,N,SO>::HybridMatrix( size_t m, size_t n, const Other* array )
   : v_()     // The statically allocated matrix elements
   , m_( m )  // The current number of rows of the matrix
   , n_( n )  // The current number of columns of the matrix
{
   if( m > M )
      throw std::invalid_argument( "Invalid number of rows for hybrid matrix" );

   if( n > N )
      throw std::invalid_argument( "Invalid number of columns for hybrid matrix" );

   for( size_t i=0UL; i<m; ++i ) {
      for( size_t j=0UL; j<n; ++j )
         v_[i*NN+j] = array[i*n+j];

      if( IsVectorizable<Type>::value ) {
         for( size_t j=n; j<NN; ++j )
            v_[i*NN+j] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t i=m; i<M; ++i )
         for( size_t j=0UL; j<NN; ++j )
            v_[i*NN+j] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Array initialization of all matrix elements.
//
// \param array \f$ M \times N \f$ dimensional array for the initialization.
//
// This constructor offers the option to directly initialize the elements of the matrix with
// a static array:

   \code
   using blaze::rowMajor;

   const int init[3][3] = { { 1, 2, 3 },
                            { 4, 5 },
                            { 7, 8, 9 } };
   blaze::HybridMatrix<int,3,3,rowMajor> A( init );
   \endcode

// The matrix is sized accoring to the size of the array and initialized with the values from
// the given array. Missing values are initialized with default values (as e.g. the value 6 in
// the example).
*/
template< typename Type   // Data type of the matrix
        , size_t M        // Number of rows
        , size_t N        // Number of columns
        , bool SO >       // Storage order
template< typename Other  // Data type of the initialization array
        , size_t M2       // Number of rows of the initialization array
        , size_t N2 >     // Number of columns of the initialization array
inline HybridMatrix<Type,M,N,SO>::HybridMatrix( const Other (&array)[M2][N2] )
   : v_()      // The statically allocated matrix elements
   , m_( M2 )  // The current number of rows of the matrix
   , n_( N2 )  // The current number of columns of the matrix
{
   BLAZE_STATIC_ASSERT( M2 <= M );
   BLAZE_STATIC_ASSERT( N2 <= N );

   for( size_t i=0UL; i<M2; ++i ) {
      for( size_t j=0UL; j<N2; ++j )
         v_[i*NN+j] = array[i][j];

      if( IsVectorizable<Type>::value ) {
         for( size_t j=N2; j<NN; ++j )
            v_[i*NN+j] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t i=M2; i<M; ++i )
         for( size_t j=0UL; j<NN; ++j )
            v_[i*NN+j] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief The copy constructor for HybridMatrix.
//
// \param m Matrix to be copied.
//
// The copy constructor is explicitly defined due to the required dynamic memory management
// and in order to enable/facilitate NRV optimization.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N          // Number of columns
        , bool SO >         // Storage order
inline HybridMatrix<Type,M,N,SO>::HybridMatrix( const HybridMatrix& m )
   : v_()        // The statically allocated matrix elements
   , m_( m.m_ )  // The current number of rows of the matrix
   , n_( m.n_ )  // The current number of columns of the matrix
{
   for( size_t i=0UL; i<m_; ++i ) {
      for( size_t j=0UL; j<n_; ++j )
         v_[i*NN+j] = m.v_[i*NN+j];

      if( IsVectorizable<Type>::value ) {
         for( size_t j=n_; j<NN; ++j )
            v_[i*NN+j] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t i=m_; i<M; ++i )
         for( size_t j=0UL; j<NN; ++j )
            v_[i*NN+j] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Conversion constructor from different matrices.
//
// \param m Matrix to be copied.
// \exception std::invalid_argument Invalid setup of hybrid matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the foreign matrix
        , bool SO2 >     // Storage order of the foreign matrix
inline HybridMatrix<Type,M,N,SO>::HybridMatrix( const Matrix<MT,SO2>& m )
   : v_()                  // The statically allocated matrix elements
   , m_( (~m).rows() )     // The current number of rows of the matrix
   , n_( (~m).columns() )  // The current number of columns of the matrix
{
   using blaze::assign;

   if( (~m).rows() > M || (~m).columns() > N )
      throw std::invalid_argument( "Invalid setup of hybrid matrix" );

   for( size_t i=0UL; i<m_; ++i ) {
      for( size_t j=( IsSparseMatrix<MT>::value   ? 0UL : n_ );
                  j<( IsVectorizable<Type>::value ? NN  : n_ ); ++j ) {
         v_[i*NN+j] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t i=m_; i<M; ++i )
         for( size_t j=0UL; j<NN; ++j )
            v_[i*NN+j] = Type();
   }

   assign( *this, ~m );
}
//*************************************************************************************************




//=================================================================================================
//
//  DATA ACCESS FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief 2D-access to the matrix elements.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \return Reference to the accessed value.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::Reference
   HybridMatrix<Type,M,N,SO>::operator()( size_t i, size_t j )
{
   BLAZE_USER_ASSERT( i<M, "Invalid row access index"    );
   BLAZE_USER_ASSERT( j<N, "Invalid column access index" );
   return v_[i*NN+j];
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 2D-access to the matrix elements.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \return Reference-to-const to the accessed value.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::ConstReference
   HybridMatrix<Type,M,N,SO>::operator()( size_t i, size_t j ) const
{
   BLAZE_USER_ASSERT( i<M, "Invalid row access index"    );
   BLAZE_USER_ASSERT( j<N, "Invalid column access index" );
   return v_[i*NN+j];
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Low-level data access to the matrix elements.
//
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage of the hybrid matrix. Note that you
// can NOT assume that all matrix elements lie adjacent to each other! The hybrid matrix may
// use techniques such as padding to improve the alignment of the data. Whereas the number of
// elements within a row/column are given by the \c rows() and \c columns() member functions,
// respectively, the total number of elements including padding is given by the \c spacing()
// member function.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::Pointer
   HybridMatrix<Type,M,N,SO>::data()
{
   return v_;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Low-level data access to the matrix elements.
//
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage of the hybrid matrix. Note that you
// can NOT assume that all matrix elements lie adjacent to each other! The hybrid matrix may
// use techniques such as padding to improve the alignment of the data. Whereas the number of
// elements within a row/column are given by the \c rows() and \c columns() member functions,
// respectively, the total number of elements including padding is given by the \c spacing()
// member function.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::ConstPointer
   HybridMatrix<Type,M,N,SO>::data() const
{
   return v_;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Low-level data access to the matrix elements of row/column \a i.
//
// \param i The row/column index.
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage for the elements in row/column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::Pointer
   HybridMatrix<Type,M,N,SO>::data( size_t i )
{
   BLAZE_USER_ASSERT( i < M, "Invalid dense matrix row access index" );
   return v_ + i*NN;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Low-level data access to the matrix elements of row/column \a i.
//
// \param i The row/column index.
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage for the elements in row/column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::ConstPointer
   HybridMatrix<Type,M,N,SO>::data( size_t i ) const
{
   BLAZE_USER_ASSERT( i < M, "Invalid dense matrix row access index" );
   return v_ + i*NN;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator to the first element of row/column \a i.
//
// \param i The row/column index.
// \return Iterator to the first element of row/column \a i.
//
// This function returns a row/column iterator to the first element of row/column \a i. In case
// the storage order is set to \a rowMajor the function returns an iterator to the first element
// of row \a i, in case the storage flag is set to \a columnMajor the function returns an iterator
// to the first element of column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::Iterator
   HybridMatrix<Type,M,N,SO>::begin( size_t i )
{
   BLAZE_USER_ASSERT( i < M, "Invalid dense matrix row access index" );
   return Iterator( v_ + i*NN );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator to the first element of row/column \a i.
//
// \param i The row/column index.
// \return Iterator to the first element of row/column \a i.
//
// This function returns a row/column iterator to the first element of row/column \a i. In case
// the storage order is set to \a rowMajor the function returns an iterator to the first element
// of row \a i, in case the storage flag is set to \a columnMajor the function returns an iterator
// to the first element of column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::ConstIterator
   HybridMatrix<Type,M,N,SO>::begin( size_t i ) const
{
   BLAZE_USER_ASSERT( i < M, "Invalid dense matrix row access index" );
   return ConstIterator( v_ + i*NN );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator to the first element of row/column \a i.
//
// \param i The row/column index.
// \return Iterator to the first element of row/column \a i.
//
// This function returns a row/column iterator to the first element of row/column \a i. In case
// the storage order is set to \a rowMajor the function returns an iterator to the first element
// of row \a i, in case the storage flag is set to \a columnMajor the function returns an iterator
// to the first element of column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::ConstIterator
   HybridMatrix<Type,M,N,SO>::cbegin( size_t i ) const
{
   BLAZE_USER_ASSERT( i < M, "Invalid dense matrix row access index" );
   return ConstIterator( v_ + i*NN );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator just past the last element of row/column \a i.
//
// \param i The row/column index.
// \return Iterator just past the last element of row/column \a i.
//
// This function returns an row/column iterator just past the last element of row/column \a i.
// In case the storage order is set to \a rowMajor the function returns an iterator just past
// the last element of row \a i, in case the storage flag is set to \a columnMajor the function
// returns an iterator just past the last element of column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::Iterator
   HybridMatrix<Type,M,N,SO>::end( size_t i )
{
   BLAZE_USER_ASSERT( i < M, "Invalid dense matrix row access index" );
   return Iterator( v_ + i*NN + N );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator just past the last element of row/column \a i.
//
// \param i The row/column index.
// \return Iterator just past the last element of row/column \a i.
//
// This function returns an row/column iterator just past the last element of row/column \a i.
// In case the storage order is set to \a rowMajor the function returns an iterator just past
// the last element of row \a i, in case the storage flag is set to \a columnMajor the function
// returns an iterator just past the last element of column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::ConstIterator
   HybridMatrix<Type,M,N,SO>::end( size_t i ) const
{
   BLAZE_USER_ASSERT( i < M, "Invalid dense matrix row access index" );
   return ConstIterator( v_ + i*NN + N );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator just past the last element of row/column \a i.
//
// \param i The row/column index.
// \return Iterator just past the last element of row/column \a i.
//
// This function returns an row/column iterator just past the last element of row/column \a i.
// In case the storage order is set to \a rowMajor the function returns an iterator just past
// the last element of row \a i, in case the storage flag is set to \a columnMajor the function
// returns an iterator just past the last element of column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::ConstIterator
   HybridMatrix<Type,M,N,SO>::cend( size_t i ) const
{
   BLAZE_USER_ASSERT( i < M, "Invalid dense matrix row access index" );
   return ConstIterator( v_ + i*NN + N );
}
//*************************************************************************************************




//=================================================================================================
//
//  ASSIGNMENT OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Array assignment to all matrix elements.
//
// \param array \f$ M \times N \f$ dimensional array for the assignment.
// \return Reference to the assigned matrix.
//
// This assignment operator offers the option to directly set all elements of the matrix:

   \code
   using blaze::rowMajor;

   const real init[3][3] = { { 1, 2, 3 },
                             { 4, 5 },
                             { 7, 8, 9 } };
   blaze::HybridMatrix<int,3UL,3UL,rowMajor> A;
   A = init;
   \endcode

// The matrix is assigned the values from the given array. Missing values are initialized with
// default values (as e.g. the value 6 in the example).
*/
template< typename Type   // Data type of the matrix
        , size_t M        // Number of rows
        , size_t N        // Number of columns
        , bool SO >       // Storage order
template< typename Other  // Data type of the initialization array
        , size_t M2       // Number of rows of the initialization array
        , size_t N2 >     // Number of columns of the initialization array
inline HybridMatrix<Type,M,N,SO>& HybridMatrix<Type,M,N,SO>::operator=( const Other (&array)[M2][N2] )
{
   BLAZE_STATIC_ASSERT( M2 <= M );
   BLAZE_STATIC_ASSERT( N2 <= N );

   resize( M2, N2 );

   for( size_t i=0UL; i<M2; ++i )
      for( size_t j=0UL; j<N2; ++j )
         v_[i*NN+j] = array[i][j];

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Homogenous assignment to all matrix elements.
//
// \param set Scalar value to be assigned to all matrix elements.
// \return Reference to the assigned matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline HybridMatrix<Type,M,N,SO>& HybridMatrix<Type,M,N,SO>::operator=( const Type& set )
{
   BLAZE_INTERNAL_ASSERT( m_ <= M, "Invalid number of rows detected"    );
   BLAZE_INTERNAL_ASSERT( n_ <= N, "Invalid number of columns detected" );

   for( size_t i=0UL; i<m_; ++i )
      for( size_t j=0UL; j<n_; ++j )
         v_[i*NN+j] = set;

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Copy assignment operator for HybridMatrix.
//
// \param rhs Matrix to be copied.
// \return Reference to the assigned matrix.
//
// Explicit definition of a copy assignment operator for performance reasons.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline HybridMatrix<Type,M,N,SO>& HybridMatrix<Type,M,N,SO>::operator=( const HybridMatrix& rhs )
{
   using blaze::assign;

   BLAZE_INTERNAL_ASSERT( m_ <= M, "Invalid number of rows detected"    );
   BLAZE_INTERNAL_ASSERT( n_ <= N, "Invalid number of columns detected" );

   resize( rhs.rows(), rhs.columns() );
   assign( *this, ~rhs );

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Assignment operator for different matrices.
//
// \param rhs Matrix to be copied.
// \return Reference to the assigned matrix.
// \exception std::invalid_argument Invalid assignment to hybrid matrix.
//
// This constructor initializes the matrix as a copy of the given matrix. In case the
// number of rows of the given matrix is not M or the number of columns is not N, a
// \a std::invalid_argument exception is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side matrix
        , bool SO2 >     // Storage order of the right-hand side matrix
inline HybridMatrix<Type,M,N,SO>& HybridMatrix<Type,M,N,SO>::operator=( const Matrix<MT,SO2>& rhs )
{
   using blaze::assign;

   if( (~rhs).rows() > M || (~rhs).columns() > N )
      throw std::invalid_argument( "Invalid assignment to hybrid matrix" );

   if( (~rhs).canAlias( this ) ) {
      HybridMatrix tmp( ~rhs );
      swap( tmp );
   }
   else {
      resize( (~rhs).rows(), (~rhs).columns() );
      if( IsSparseMatrix<MT>::value )
         reset();
      assign( *this, ~rhs );
   }

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Addition assignment operator for the addition of a matrix (\f$ A+=B \f$).
//
// \param rhs The right-hand side matrix to be added to the matrix.
// \return Reference to the matrix.
// \exception std::invalid_argument Matrix sizes do not match.
//
// In case the current sizes of the two matrices don't match, a \a std::invalid_argument exception
// is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side matrix
        , bool SO2 >     // Storage order of the right-hand side matrix
inline HybridMatrix<Type,M,N,SO>& HybridMatrix<Type,M,N,SO>::operator+=( const Matrix<MT,SO2>& rhs )
{
   using blaze::addAssign;

   if( (~rhs).rows() != m_ || (~rhs).columns() != n_ )
      throw std::invalid_argument( "Matrix sizes do not match" );

   if( (~rhs).canAlias( this ) ) {
      typename MT::ResultType tmp( ~rhs );
      addAssign( *this, tmp );
   }
   else {
      addAssign( *this, ~rhs );
   }

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Subtraction assignment operator for the subtraction of a matrix (\f$ A-=B \f$).
//
// \param rhs The right-hand side matrix to be subtracted from the matrix.
// \return Reference to the matrix.
// \exception std::invalid_argument Matrix sizes do not match.
//
// In case the current sizes of the two matrices don't match, a \a std::invalid_argument exception
// is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side matrix
        , bool SO2 >     // Storage order of the right-hand side matrix
inline HybridMatrix<Type,M,N,SO>& HybridMatrix<Type,M,N,SO>::operator-=( const Matrix<MT,SO2>& rhs )
{
   using blaze::subAssign;

   if( (~rhs).rows() != m_ || (~rhs).columns() != n_ )
      throw std::invalid_argument( "Matrix sizes do not match" );

   if( (~rhs).canAlias( this ) ) {
      typename MT::ResultType tmp( ~rhs );
      subAssign( *this, tmp );
   }
   else {
      subAssign( *this, ~rhs );
   }

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Multiplication assignment operator for the multiplication of a matrix (\f$ A*=B \f$).
//
// \param rhs The right-hand side matrix for the multiplication.
// \return Reference to the matrix.
// \exception std::invalid_argument Matrix sizes do not match.
//
// In case the current sizes of the two given matrices don't match, a \a std::invalid_argument
// is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side matrix
        , bool SO2 >     // Storage order of the right-hand side matrix
inline HybridMatrix<Type,M,N,SO>& HybridMatrix<Type,M,N,SO>::operator*=( const Matrix<MT,SO2>& rhs )
{
   if( n_ != (~rhs).rows() || (~rhs).columns() > N )
      throw std::invalid_argument( "Matrix sizes do not match" );

   HybridMatrix tmp( *this * (~rhs) );
   return this->operator=( tmp );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Multiplication assignment operator for the multiplication between a matrix and
//        a scalar value (\f$ A*=s \f$).
//
// \param rhs The right-hand side scalar value for the multiplication.
// \return Reference to the matrix.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N          // Number of columns
        , bool SO >         // Storage order
template< typename Other >  // Data type of the right-hand side scalar
inline typename EnableIf< IsNumeric<Other>, HybridMatrix<Type,M,N,SO> >::Type&
   HybridMatrix<Type,M,N,SO>::operator*=( Other rhs )
{
   using blaze::assign;

   assign( *this, (*this) * rhs );
   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Division assignment operator for the division of a matrix by a scalar value
//        (\f$ A/=s \f$).
//
// \param rhs The right-hand side scalar value for the division.
// \return Reference to the matrix.
//
// \b Note: A division by zero is only checked by an user assert.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N          // Number of columns
        , bool SO >         // Storage order
template< typename Other >  // Data type of the right-hand side scalar
inline typename EnableIf< IsNumeric<Other>, HybridMatrix<Type,M,N,SO> >::Type&
   HybridMatrix<Type,M,N,SO>::operator/=( Other rhs )
{
   using blaze::assign;

   BLAZE_USER_ASSERT( rhs != Other(0), "Division by zero detected" );

   assign( *this, (*this) / rhs );
   return *this;
}
//*************************************************************************************************




//=================================================================================================
//
//  UTILITY FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Returns the current number of rows of the matrix.
//
// \return The number of rows of the matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline size_t HybridMatrix<Type,M,N,SO>::rows() const
{
   return m_;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the current number of columns of the matrix.
//
// \return The number of columns of the matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline size_t HybridMatrix<Type,M,N,SO>::columns() const
{
   return n_;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the spacing between the beginning of two rows.
//
// \return The spacing between the beginning of two rows.
//
// This function returns the spacing between the beginning of two rows, i.e. the total number
// of elements of a row.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline size_t HybridMatrix<Type,M,N,SO>::spacing() const
{
   return NN;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the maximum capacity of the matrix.
//
// \return The capacity of the matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline size_t HybridMatrix<Type,M,N,SO>::capacity() const
{
   return M*NN;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the current capacity of the specified row/column.
//
// \param i The index of the row/column.
// \return The current capacity of row/column \a i.
//
// This function returns the current capacity of the specified row/column. In case the
// storage order is set to \a rowMajor the function returns the capacity of row \a i,
// in case the storage flag is set to \a columnMajor the function returns the capacity
// of column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline size_t HybridMatrix<Type,M,N,SO>::capacity( size_t i ) const
{
   UNUSED_PARAMETER( i );

   BLAZE_USER_ASSERT( i < rows(), "Invalid row access index" );

   return NN;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the total number of non-zero elements in the matrix
//
// \return The number of non-zero elements in the matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline size_t HybridMatrix<Type,M,N,SO>::nonZeros() const
{
   size_t nonzeros( 0UL );

   for( size_t i=0UL; i<m_; ++i )
      for( size_t j=0UL; j<n_; ++j )
         if( !isDefault( v_[i*NN+j] ) )
            ++nonzeros;

   return nonzeros;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the number of non-zero elements in the specified row/column.
//
// \param i The index of the row/column.
// \return The number of non-zero elements of row/column \a i.
//
// This function returns the current number of non-zero elements in the specified row/column.
// In case the storage order is set to \a rowMajor the function returns the number of non-zero
// elements in row \a i, in case the storage flag is set to \a columnMajor the function returns
// the number of non-zero elements in column \a i.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline size_t HybridMatrix<Type,M,N,SO>::nonZeros( size_t i ) const
{
   BLAZE_USER_ASSERT( i < rows(), "Invalid row access index" );

   const size_t jend( i*NN+n_ );
   size_t nonzeros( 0UL );

   for( size_t j=i*NN; j<jend; ++j )
      if( !isDefault( v_[j] ) )
         ++nonzeros;

   return nonzeros;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Reset to the default initial values.
//
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::reset()
{
   using blaze::reset;

   for( size_t i=0UL; i<m_; ++i )
      for( size_t j=0UL; j<n_; ++j )
         reset( v_[i*NN+j] );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Reset the specified row/column to the default initial values.
//
// \param i The index of the row/column.
// \return void
//
// This function resets the values in the specified row/column to their default value. In case
// the storage order is set to \a rowMajor the function resets the values in row \a i, in case
// the storage order is set to \a columnMajor the function resets the values in column \a i.
// Note that the capacity of the row/column remains unchanged.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::reset( size_t i )
{
   using blaze::reset;

   BLAZE_USER_ASSERT( i < rows(), "Invalid row access index" );
   for( size_t j=0UL; j<n_; ++j )
      reset( v_[i*NN+j] );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Clearing the hybrid matrix.
//
// \return void
//
// After the clear() function, the size of the matrix is 0.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::clear()
{
   resize( 0UL, 0UL );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Changing the size of the matrix.
//
// \param m The new number of rows of the matrix.
// \param n The new number of columns of the matrix.
// \param preserve \a true if the old values of the matrix should be preserved, \a false if not.
// \return void
//
// This function resizes the matrix using the given size to \f$ m \times n \f$. In case the given
// number of rows \a m is larger than the maximum number of rows (i.e. if m > M) or in case the
// given number of columns \a n is larger than the maximum number of column (i.e. if n > N) a
// \a std::invalid_argument exception is thrown. Note that this function may invalidate all
// existing views (submatrices, rows, columns, ...) on the matrix if it is used to shrink the
// matrix. Additionally, during this operation all matrix elements are potentially changed. In
// order to preserve the old matrix values, the \a preserve flag can be set to \a true.
//
// Note that in case the number of rows or columns is increased new matrix elements are not
// initialized! The following example illustrates the resize operation of a \f$ 2 \times 4 \f$
// matrix to a \f$ 4 \times 2 \f$ matrix. The new, uninitialized elements are marked with \a x:

                              \f[
                              \left(\begin{array}{*{4}{c}}
                              1 & 2 & 3 & 4 \\
                              5 & 6 & 7 & 8 \\
                              \end{array}\right)

                              \Longrightarrow

                              \left(\begin{array}{*{2}{c}}
                              1 & 2 \\
                              5 & 6 \\
                              x & x \\
                              x & x \\
                              \end{array}\right)
                              \f]
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
void HybridMatrix<Type,M,N,SO>::resize( size_t m, size_t n, bool preserve )
{
   UNUSED_PARAMETER( preserve );

   if( m > M )
      throw std::invalid_argument( "Invalid number of rows for hybrid matrix" );

   if( n > N )
      throw std::invalid_argument( "Invalid number of columns for hybrid matrix" );

   if( IsVectorizable<Type>::value && n < n_ ) {
      for( size_t i=0UL; i<m; ++i )
         for( size_t j=n; j<n_; ++j )
            v_[i*NN+j] = Type();
   }

   if( IsVectorizable<Type>::value && m < m_ ) {
      for( size_t i=m; i<m_; ++i )
         for( size_t j=0UL; j<n_; ++j )
            v_[i*NN+j] = Type();
   }

   m_ = m;
   n_ = n;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Extending the size of the matrix.
//
// \param m Number of additional rows.
// \param n Number of additional columns.
// \param preserve \a true if the old values of the matrix should be preserved, \a false if not.
// \return void
//
// This function increases the matrix size by \a m rows and \a n columns. In case the resulting
// number of rows or columns is larger than the maximum number of rows or columns (i.e. if m > M
// or n > N) a \a std::invalid_argument exception is thrown. During this operation, all matrix
// elements are potentially changed. In order to preserve the old matrix values, the \a preserve
// flag can be set to \a true.\n
// Note that new matrix elements are not initialized!
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::extend( size_t m, size_t n, bool preserve )
{
   UNUSED_PARAMETER( preserve );
   resize( m_+m, n_+n );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Transposing the matrix.
//
// \return Reference to the transposed matrix.
// \exception std::logic_error Impossible transpose operation.
//
// This function transposes the hybrid matrix in-place. Note that this function can only be used
// on hybrid matrices whose current dimensions allow an in-place transpose operation. In case the
// current number of rows is larger than the maximum number of columns or if the current number
// of columns is larger than the maximum number of rows, an \a std::logic_error is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline HybridMatrix<Type,M,N,SO>& HybridMatrix<Type,M,N,SO>::transpose()
{
   using std::swap;

   if( m_ > N || n_ > M )
      throw std::logic_error( "Impossible transpose operation" );

   const size_t maxsize( max( m_, n_ ) );
   for( size_t i=1UL; i<maxsize; ++i )
      for( size_t j=0UL; j<i; ++j )
         swap( v_[i*NN+j], v_[j*NN+i] );

   if( IsVectorizable<Type>::value && m_ < n_ ) {
      for( size_t i=0UL; i<m_; ++i ) {
         for( size_t j=m_; j<n_; ++j ) {
            v_[i*NN+j] = Type();
         }
      }
   }

   if( IsVectorizable<Type>::value && m_ > n_ ) {
      for( size_t i=n_; i<m_; ++i ) {
         for( size_t j=0UL; j<n_; ++j ) {
            v_[i*NN+j] = Type();
         }
      }
   }

   swap( m_, n_ );

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Scaling of the matrix by the scalar value \a scalar (\f$ A*=s \f$).
//
// \param scalar The scalar value for the matrix scaling.
// \return Reference to the matrix.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N          // Number of columns
        , bool SO >         // Storage order
template< typename Other >  // Data type of the scalar value
inline HybridMatrix<Type,M,N,SO>& HybridMatrix<Type,M,N,SO>::scale( const Other& scalar )
{
   for( size_t i=0UL; i<m_; ++i )
      for( size_t j=0UL; j<n_; ++j )
         v_[i*NN+j] *= scalar;

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Swapping the contents of two hybrid matrices.
//
// \param m The matrix to be swapped.
// \return void
// \exception no-throw guarantee.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::swap( HybridMatrix& m ) /* throw() */
{
   using std::swap;

   const size_t maxrows( max( m_, m.m_ ) );
   const size_t maxcols( max( n_, m.n_ ) );

   for( size_t i=0UL; i<maxrows; ++i ) {
      for( size_t j=0UL; j<maxcols; ++j ) {
         swap( v_[i*NN+j], m(i,j) );
      }
   }

   swap( m_, m.m_ );
   swap( n_, m.n_ );
}
//*************************************************************************************************




//=================================================================================================
//
//  MEMORY FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Class specific implementation of operator new.
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridMatrix class template.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void* HybridMatrix<Type,M,N,SO>::operator new( std::size_t size )
{
   UNUSED_PARAMETER( size );

   BLAZE_INTERNAL_ASSERT( size == sizeof( HybridMatrix ), "Invalid number of bytes detected" );

   return allocate<HybridMatrix>( 1UL );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of operator new[].
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridMatrix class template.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void* HybridMatrix<Type,M,N,SO>::operator new[]( std::size_t size )
{
   BLAZE_INTERNAL_ASSERT( size >= sizeof( HybridMatrix )       , "Invalid number of bytes detected" );
   BLAZE_INTERNAL_ASSERT( size %  sizeof( HybridMatrix ) == 0UL, "Invalid number of bytes detected" );

   return allocate<HybridMatrix>( size/sizeof(HybridMatrix) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of the no-throw operator new.
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridMatrix class template.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void* HybridMatrix<Type,M,N,SO>::operator new( std::size_t size, const std::nothrow_t& )
{
   UNUSED_PARAMETER( size );

   BLAZE_INTERNAL_ASSERT( size == sizeof( HybridMatrix ), "Invalid number of bytes detected" );

   return allocate<HybridMatrix>( 1UL );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of the no-throw operator new[].
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridMatrix class template.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void* HybridMatrix<Type,M,N,SO>::operator new[]( std::size_t size, const std::nothrow_t& )
{
   BLAZE_INTERNAL_ASSERT( size >= sizeof( HybridMatrix )       , "Invalid number of bytes detected" );
   BLAZE_INTERNAL_ASSERT( size %  sizeof( HybridMatrix ) == 0UL, "Invalid number of bytes detected" );

   return allocate<HybridMatrix>( size/sizeof(HybridMatrix) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of operator delete.
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::operator delete( void* ptr )
{
   deallocate( static_cast<HybridMatrix*>( ptr ) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of operator delete[].
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::operator delete[]( void* ptr )
{
   deallocate( static_cast<HybridMatrix*>( ptr ) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of no-throw operator delete.
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::operator delete( void* ptr, const std::nothrow_t& )
{
   deallocate( static_cast<HybridMatrix*>( ptr ) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of no-throw operator delete[].
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::operator delete[]( void* ptr, const std::nothrow_t& )
{
   deallocate( static_cast<HybridMatrix*>( ptr ) );
}
//*************************************************************************************************




//=================================================================================================
//
//  EXPRESSION TEMPLATE EVALUATION FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Returns whether the matrix can alias with the given address \a alias.
//
// \param alias The alias to be checked.
// \return \a true in case the alias corresponds to this matrix, \a false if not.
//
// This function returns whether the given address can alias with the matrix. In contrast
// to the isAliased() function this function is allowed to use compile time expressions
// to optimize the evaluation.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N          // Number of columns
        , bool SO >         // Storage order
template< typename Other >  // Data type of the foreign expression
inline bool HybridMatrix<Type,M,N,SO>::canAlias( const Other* alias ) const
{
   return static_cast<const void*>( this ) == static_cast<const void*>( alias );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns whether the matrix is aliased with the given address \a alias.
//
// \param alias The alias to be checked.
// \return \a true in case the alias corresponds to this matrix, \a false if not.
//
// This function returns whether the given address is aliased with the matrix. In contrast
// to the conAlias() function this function is not allowed to use compile time expressions
// to optimize the evaluation.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N          // Number of columns
        , bool SO >         // Storage order
template< typename Other >  // Data type of the foreign expression
inline bool HybridMatrix<Type,M,N,SO>::isAliased( const Other* alias ) const
{
   return static_cast<const void*>( this ) == static_cast<const void*>( alias );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns whether the matrix is properly aligned in memory.
//
// \return \a true in case the matrix is aligned, \a false if not.
//
// This function returns whether the matrix is guaranteed to be properly aligned in memory, i.e.
// whether the beginning and the end of each row/column of the matrix are guaranteed to conform
// to the alignment restrictions of the element type \a Type.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline bool HybridMatrix<Type,M,N,SO>::isAligned() const
{
   return true;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Aligned load of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \return The loaded intrinsic element.
//
// This function performs an aligned load of a specific intrinsic element of the dense matrix.
// The row index must be smaller than the number of rows and the column index must be smaller
// than the number of columns. Additionally, the column index (in case of a row-major matrix)
// or the row index (in case of a column-major matrix) must be a multiple of the number of
// values inside the intrinsic element. This function must \b NOT be called explicitly! It is
// used internally for the performance optimized evaluation of expression templates. Calling
// this function explicitly might result in erroneous results and/or in compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::IntrinsicType
   HybridMatrix<Type,M,N,SO>::load( size_t i, size_t j ) const
{
   using blaze::load;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_ , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_ , "Invalid column access index" );
   BLAZE_INTERNAL_ASSERT( j + IT::size <= NN , "Invalid column access index" );
   BLAZE_INTERNAL_ASSERT( j % IT::size == 0UL, "Invalid column access index" );

   return load( &v_[i*NN+j] );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Unaligned load of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \return The loaded intrinsic element.
//
// This function performs an unaligned load of a specific intrinsic element of the dense matrix.
// The row index must be smaller than the number of rows and the column index must be smaller
// than the number of columns. Additionally, the column index (in case of a row-major matrix)
// or the row index (in case of a column-major matrix) must be a multiple of the number of
// values inside the intrinsic element. This function must \b NOT be called explicitly! It is
// used internally for the performance optimized evaluation of expression templates. Calling
// this function explicitly might result in erroneous results and/or in compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline typename HybridMatrix<Type,M,N,SO>::IntrinsicType
   HybridMatrix<Type,M,N,SO>::loadu( size_t i, size_t j ) const
{
   using blaze::loadu;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_ , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_ , "Invalid column access index" );
   BLAZE_INTERNAL_ASSERT( j + IT::size <= NN , "Invalid column access index" );

   return loadu( &v_[i*NN+j] );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Aligned store of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \param value The intrinsic element to be stored.
// \return void
//
// This function performs an aligned store of a specific intrinsic element of the dense matrix.
// The row index must be smaller than the number of rows and the column index must be smaller
// than the number of columns. Additionally, the column index (in case of a row-major matrix)
// or the row index (in case of a column-major matrix) must be a multiple of the number of
// values inside the intrinsic element. This function must \b NOT be called explicitly! It is
// used internally for the performance optimized evaluation of expression templates. Calling
// this function explicitly might result in erroneous results and/or in compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::store( size_t i, size_t j, const IntrinsicType& value )
{
   using blaze::store;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_ , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_ , "Invalid column access index" );
   BLAZE_INTERNAL_ASSERT( j + IT::size <= NN , "Invalid column access index" );
   BLAZE_INTERNAL_ASSERT( j % IT::size == 0UL, "Invalid column access index" );

   store( &v_[i*NN+j], value );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Unaligned store of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \param value The intrinsic element to be stored.
// \return void
//
// This function performs an unaligned store of a specific intrinsic element of the dense matrix.
// The row index must be smaller than the number of rows and the column index must be smaller
// than the number of columns. Additionally, the column index (in case of a row-major matrix)
// or the row index (in case of a column-major matrix) must be a multiple of the number of
// values inside the intrinsic element. This function must \b NOT be called explicitly! It is
// used internally for the performance optimized evaluation of expression templates. Calling
// this function explicitly might result in erroneous results and/or in compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::storeu( size_t i, size_t j, const IntrinsicType& value )
{
   using blaze::storeu;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_, "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_, "Invalid column access index" );
   BLAZE_INTERNAL_ASSERT( j + IT::size <= NN, "Invalid column access index" );

   storeu( &v_[i*NN+j], value );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Aligned, non-temporal store of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \param value The intrinsic element to be stored.
// \return void
//
// This function performs an aligned, non-temporal store of a specific intrinsic element of the
// dense matrix. The row index must be smaller than the number of rows and the column index must
// be smaller than the number of columns. Additionally, the column index (in case of a row-major
// matrix) or the row index (in case of a column-major matrix) must be a multiple of the number
// of values inside the intrinsic element. This function must \b NOT be called explicitly! It
// is used internally for the performance optimized evaluation of expression templates. Calling
// this function explicitly might result in erroneous results and/or in compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void HybridMatrix<Type,M,N,SO>::stream( size_t i, size_t j, const IntrinsicType& value )
{
   using blaze::stream;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_ , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_ , "Invalid column access index" );
   BLAZE_INTERNAL_ASSERT( j + IT::size <= NN , "Invalid column access index" );
   BLAZE_INTERNAL_ASSERT( j % IT::size == 0UL, "Invalid column access index" );

   stream( &v_[i*NN+j], value );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO2 >     // Storage order of the right-hand side dense matrix
inline typename DisableIf< typename HybridMatrix<Type,M,N,SO>::BLAZE_TEMPLATE VectorizedAssign<MT> >::Type
   HybridMatrix<Type,M,N,SO>::assign( const DenseMatrix<MT,SO2>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   for( size_t i=0UL; i<m_; ++i ) {
      for( size_t j=0UL; j<n_; ++j ) {
         v_[i*NN+j] = (~rhs)(i,j);
      }
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Intrinsic optimized implementation of the assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO2 >     // Storage order of the right-hand side dense matrix
inline typename EnableIf< typename HybridMatrix<Type,M,N,SO>::BLAZE_TEMPLATE VectorizedAssign<MT> >::Type
   HybridMatrix<Type,M,N,SO>::assign( const DenseMatrix<MT,SO2>& rhs )
{
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t i=0UL; i<m_; ++i ) {
      for( size_t j=0UL; j<n_; j+=IT::size ) {
         store( &v_[i*NN+j], (~rhs).load(i,j) );
      }
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the assignment of a row-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,SO>::assign( const SparseMatrix<MT,SO>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t i=0UL; i<m_; ++i )
      for( RhsConstIterator element=(~rhs).begin(i); element!=(~rhs).end(i); ++element )
         v_[i*NN+element->index()] = element->value();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the assignment of a column-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,SO>::assign( const SparseMatrix<MT,!SO>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t j=0UL; j<n_; ++j )
      for( RhsConstIterator element=(~rhs).begin(j); element!=(~rhs).end(j); ++element )
         v_[element->index()*NN+j] = element->value();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the addition assignment of a row-major dense matrix.
//
// \param rhs The right-hand side dense matrix to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO2 >     // Storage order of the right-hand side dense matrix
inline typename DisableIf< typename HybridMatrix<Type,M,N,SO>::BLAZE_TEMPLATE VectorizedAddAssign<MT> >::Type
   HybridMatrix<Type,M,N,SO>::addAssign( const DenseMatrix<MT,SO2>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   for( size_t i=0UL; i<m_; ++i ) {
      for( size_t j=0UL; j<n_; ++j ) {
         v_[i*NN+j] += (~rhs)(i,j);
      }
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Intrinsic optimized implementation of the addition assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO2 >     // Storage order of the right-hand side dense matrix
inline typename EnableIf< typename HybridMatrix<Type,M,N,SO>::BLAZE_TEMPLATE VectorizedAddAssign<MT> >::Type
   HybridMatrix<Type,M,N,SO>::addAssign( const DenseMatrix<MT,SO2>& rhs )
{
   using blaze::load;
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t i=0UL; i<m_; ++i ) {
      for( size_t j=0UL; j<n_; j+=IT::size ) {
         store( &v_[i*NN+j], load( &v_[i*NN+j] ) + (~rhs).load(i,j) );
      }
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the addition assignment of a row-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,SO>::addAssign( const SparseMatrix<MT,SO>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t i=0UL; i<m_; ++i )
      for( RhsConstIterator element=(~rhs).begin(i); element!=(~rhs).end(i); ++element )
         v_[i*NN+element->index()] += element->value();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the addition assignment of a column-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,SO>::addAssign( const SparseMatrix<MT,!SO>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t j=0UL; j<n_; ++j )
      for( RhsConstIterator element=(~rhs).begin(j); element!=(~rhs).end(j); ++element )
         v_[element->index()*NN+j] += element->value();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the subtraction assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO2 >     // Storage order of the right-hand side dense matrix
inline typename DisableIf< typename HybridMatrix<Type,M,N,SO>::BLAZE_TEMPLATE VectorizedSubAssign<MT> >::Type
   HybridMatrix<Type,M,N,SO>::subAssign( const DenseMatrix<MT,SO2>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   for( size_t i=0UL; i<m_; ++i ) {
      for( size_t j=0UL; j<n_; ++j ) {
         v_[i*NN+j] -= (~rhs)(i,j);
      }
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Intrinsic optimized implementation of the subtraction assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO2 >     // Storage order of the right-hand side dense matrix
inline typename EnableIf< typename HybridMatrix<Type,M,N,SO>::BLAZE_TEMPLATE VectorizedSubAssign<MT> >::Type
   HybridMatrix<Type,M,N,SO>::subAssign( const DenseMatrix<MT,SO2>& rhs )
{
   using blaze::load;
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t i=0UL; i<m_; ++i ) {
      for( size_t j=0UL; j<n_; j+=IT::size ) {
         store( &v_[i*NN+j], load( &v_[i*NN+j] ) - (~rhs).load(i,j) );
      }
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the subtraction assignment of a row-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,SO>::subAssign( const SparseMatrix<MT,SO>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t i=0UL; i<m_; ++i )
      for( RhsConstIterator element=(~rhs).begin(i); element!=(~rhs).end(i); ++element )
         v_[i*NN+element->index()] -= element->value();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the subtraction assignment of a column-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,SO>::subAssign( const SparseMatrix<MT,!SO>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t j=0UL; j<n_; ++j )
      for( RhsConstIterator element=(~rhs).begin(j); element!=(~rhs).end(j); ++element )
         v_[element->index()*NN+j] -= element->value();
}
//*************************************************************************************************








//=================================================================================================
//
//  CLASS TEMPLATE SPECIALIZATION FOR COLUMN-MAJOR MATRICES
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Specialization of HybridMatrix for column-major matrices.
// \ingroup hybrid_matrix
//
// This specialization of HybridMatrix adapts the class template to the requirements of
// column-major matrices.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
class HybridMatrix<Type,M,N,true> : public DenseMatrix< HybridMatrix<Type,M,N,true>, true >
{
 private:
   //**Type definitions****************************************************************************
   typedef IntrinsicTrait<Type>  IT;  //!< Intrinsic trait for the matrix element type.
   //**********************************************************************************************

   //**********************************************************************************************
   //! Alignment adjustment
   enum { MM = M + ( IT::size - ( M % IT::size ) ) % IT::size };
   //**********************************************************************************************

 public:
   //**Type definitions****************************************************************************
   typedef HybridMatrix<Type,M,N,true>   This;            //!< Type of this HybridMatrix instance.
   typedef This                          ResultType;      //!< Result type for expression template evaluations.
   typedef HybridMatrix<Type,M,N,false>  OppositeType;    //!< Result type with opposite storage order for expression template evaluations.
   typedef HybridMatrix<Type,N,M,false>  TransposeType;   //!< Transpose type for expression template evaluations.
   typedef Type                          ElementType;     //!< Type of the matrix elements.
   typedef typename IT::Type             IntrinsicType;   //!< Intrinsic type of the matrix elements.
   typedef const Type&                   ReturnType;      //!< Return type for expression template evaluations.
   typedef const This&                   CompositeType;   //!< Data type for composite expression templates.
   typedef Type&                         Reference;       //!< Reference to a non-constant matrix value.
   typedef const Type&                   ConstReference;  //!< Reference to a constant matrix value.
   typedef Type*                         Pointer;         //!< Pointer to a non-constant matrix value.
   typedef const Type*                   ConstPointer;    //!< Pointer to a constant matrix value.
   typedef DenseIterator<Type>           Iterator;        //!< Iterator over non-constant elements.
   typedef DenseIterator<const Type>     ConstIterator;   //!< Iterator over constant elements.
   //**********************************************************************************************

   //**Compilation flags***************************************************************************
   //! Compilation flag for intrinsic optimization.
   /*! The \a vectorizable compilation flag indicates whether expressions the matrix is involved
       in can be optimized via intrinsics. In case the element type of the matrix is a vectorizable
       data type, the \a vectorizable compilation flag is set to \a true, otherwise it is set to
       \a false. */
   enum { vectorizable = IsVectorizable<Type>::value };

   //! Compilation flag for SMP assignments.
   /*! The \a smpAssignable compilation flag indicates whether the matrix can be used in SMP
       (shared memory parallel) assignments (both on the left-hand and right-hand side of the
       assignment). */
   enum { smpAssignable = 0 };
   //**********************************************************************************************

   //**Constructors********************************************************************************
   /*!\name Constructors */
   //@{
                              explicit inline HybridMatrix();
                              explicit inline HybridMatrix( size_t m, size_t n );
                              explicit inline HybridMatrix( size_t m, size_t n, const Type& init );
   template< typename Other > explicit inline HybridMatrix( size_t m, size_t n, const Other* array );

   template< typename Other, size_t M2, size_t N2 >
   explicit inline HybridMatrix( const Other (&array)[M2][N2] );

                                    inline HybridMatrix( const HybridMatrix& m );
   template< typename MT, bool SO > inline HybridMatrix( const Matrix<MT,SO>& m );
   //@}
   //**********************************************************************************************

   //**Destructor**********************************************************************************
   // No explicitly declared destructor.
   //**********************************************************************************************

   //**Data access functions***********************************************************************
   /*!\name Data access functions */
   //@{
   inline Reference      operator()( size_t i, size_t j );
   inline ConstReference operator()( size_t i, size_t j ) const;
   inline Pointer        data  ();
   inline ConstPointer   data  () const;
   inline Pointer        data  ( size_t j );
   inline ConstPointer   data  ( size_t j ) const;
   inline Iterator       begin ( size_t j );
   inline ConstIterator  begin ( size_t j ) const;
   inline ConstIterator  cbegin( size_t j ) const;
   inline Iterator       end   ( size_t j );
   inline ConstIterator  end   ( size_t j ) const;
   inline ConstIterator  cend  ( size_t j ) const;
   //@}
   //**********************************************************************************************

   //**Assignment operators************************************************************************
   /*!\name Assignment operators */
   //@{
   template< typename Other, size_t M2, size_t N2 >
   inline HybridMatrix& operator=( const Other (&array)[M2][N2] );

                                    inline HybridMatrix& operator= ( const Type& set );
                                    inline HybridMatrix& operator= ( const HybridMatrix& rhs );
   template< typename MT, bool SO > inline HybridMatrix& operator= ( const Matrix<MT,SO>& rhs );
   template< typename MT, bool SO > inline HybridMatrix& operator+=( const Matrix<MT,SO>& rhs );
   template< typename MT, bool SO > inline HybridMatrix& operator-=( const Matrix<MT,SO>& rhs );
   template< typename MT, bool SO > inline HybridMatrix& operator*=( const Matrix<MT,SO>& rhs );

   template< typename Other >
   inline typename EnableIf< IsNumeric<Other>, HybridMatrix >::Type&
      operator*=( Other rhs );

   template< typename Other >
   inline typename EnableIf< IsNumeric<Other>, HybridMatrix >::Type&
      operator/=( Other rhs );
   //@}
   //**********************************************************************************************

   //**Utility functions***************************************************************************
   /*!\name Utility functions */
   //@{
                              inline size_t        rows() const;
                              inline size_t        columns() const;
                              inline size_t        spacing() const;
                              inline size_t        capacity() const;
                              inline size_t        capacity( size_t j ) const;
                              inline size_t        nonZeros() const;
                              inline size_t        nonZeros( size_t j ) const;
                              inline void          reset();
                              inline void          reset( size_t i );
                              inline void          clear();
                                     void          resize ( size_t m, size_t n, bool preserve=true );
                              inline void          extend ( size_t m, size_t n, bool preserve=true );
                              inline HybridMatrix& transpose();
   template< typename Other > inline HybridMatrix& scale( const Other& scalar );
                              inline void          swap( HybridMatrix& m ) /* throw() */;
   //@}
   //**********************************************************************************************

   //**Memory functions****************************************************************************
   /*!\name Memory functions */
   //@{
   static inline void* operator new  ( std::size_t size );
   static inline void* operator new[]( std::size_t size );
   static inline void* operator new  ( std::size_t size, const std::nothrow_t& );
   static inline void* operator new[]( std::size_t size, const std::nothrow_t& );

   static inline void operator delete  ( void* ptr );
   static inline void operator delete[]( void* ptr );
   static inline void operator delete  ( void* ptr, const std::nothrow_t& );
   static inline void operator delete[]( void* ptr, const std::nothrow_t& );
   //@}
   //**********************************************************************************************

 private:
   //**********************************************************************************************
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename MT >
   struct VectorizedAssign {
      enum { value = vectorizable && MT::vectorizable &&
                     IsSame<Type,typename MT::ElementType>::value &&
                     IsColumnMajorMatrix<MT>::value };
   };
   //**********************************************************************************************

   //**********************************************************************************************
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename MT >
   struct VectorizedAddAssign {
      enum { value = vectorizable && MT::vectorizable &&
                     IsSame<Type,typename MT::ElementType>::value &&
                     IntrinsicTrait<Type>::addition &&
                     IsColumnMajorMatrix<MT>::value };
   };
   //**********************************************************************************************

   //**********************************************************************************************
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename MT >
   struct VectorizedSubAssign {
      enum { value = vectorizable && MT::vectorizable &&
                     IsSame<Type,typename MT::ElementType>::value &&
                     IntrinsicTrait<Type>::subtraction &&
                     IsColumnMajorMatrix<MT>::value };
   };
   //**********************************************************************************************

 public:
   //**Expression template evaluation functions****************************************************
   /*!\name Expression template evaluation functions */
   //@{
   template< typename Other > inline bool canAlias ( const Other* alias ) const;
   template< typename Other > inline bool isAliased( const Other* alias ) const;

   inline bool isAligned() const;

   inline IntrinsicType load  ( size_t i, size_t j ) const;
   inline IntrinsicType loadu ( size_t i, size_t j ) const;
   inline void          store ( size_t i, size_t j, const IntrinsicType& value );
   inline void          storeu( size_t i, size_t j, const IntrinsicType& value );
   inline void          stream( size_t i, size_t j, const IntrinsicType& value );

   template< typename MT, bool SO >
   inline typename DisableIf< VectorizedAssign<MT> >::Type
      assign( const DenseMatrix<MT,SO>& rhs );

   template< typename MT, bool SO >
   inline typename EnableIf< VectorizedAssign<MT> >::Type
      assign( const DenseMatrix<MT,SO>& rhs );

   template< typename MT > inline void assign( const SparseMatrix<MT,true>&  rhs );
   template< typename MT > inline void assign( const SparseMatrix<MT,false>& rhs );

   template< typename MT, bool SO >
   inline typename DisableIf< VectorizedAddAssign<MT> >::Type
      addAssign( const DenseMatrix<MT,SO>& rhs );

   template< typename MT, bool SO >
   inline typename EnableIf< VectorizedAddAssign<MT> >::Type
      addAssign( const DenseMatrix<MT,SO>& rhs );

   template< typename MT > inline void addAssign( const SparseMatrix<MT,true>&  rhs );
   template< typename MT > inline void addAssign( const SparseMatrix<MT,false>& rhs );

   template< typename MT, bool SO >
   inline typename DisableIf< VectorizedSubAssign<MT> >::Type
      subAssign( const DenseMatrix<MT,SO>& rhs );

   template< typename MT, bool SO >
   inline typename EnableIf< VectorizedSubAssign<MT> >::Type
      subAssign( const DenseMatrix<MT,SO>& rhs );

   template< typename MT > inline void subAssign( const SparseMatrix<MT,true>&  rhs );
   template< typename MT > inline void subAssign( const SparseMatrix<MT,false>& rhs );
   //@}
   //**********************************************************************************************

 private:
   //**Member variables****************************************************************************
   /*!\name Member variables */
   //@{
   AlignedArray<Type,MM*N> v_;  //!< The statically allocated matrix elements.
                                /*!< Access to the matrix elements is gained via the
                                     function call operator. */
   size_t m_;                   //!< The current number of rows of the matrix.
   size_t n_;                   //!< The current number of columns of the matrix.
   //@}
   //**********************************************************************************************

   //**Compile time checks*************************************************************************
   BLAZE_CONSTRAINT_MUST_NOT_BE_POINTER_TYPE  ( Type );
   BLAZE_CONSTRAINT_MUST_NOT_BE_REFERENCE_TYPE( Type );
   BLAZE_CONSTRAINT_MUST_NOT_BE_CONST         ( Type );
   BLAZE_CONSTRAINT_MUST_NOT_BE_VOLATILE      ( Type );
   BLAZE_STATIC_ASSERT( MM % IT::size == 0UL );
   BLAZE_STATIC_ASSERT( MM >= M );
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  CONSTRUCTORS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief The default constructor for HybridMatrix.
//
// All matrix elements are initialized to the default value (i.e. 0 for integral data types).
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline HybridMatrix<Type,M,N,true>::HybridMatrix()
   : v_()       // The statically allocated matrix elements
   , m_( 0UL )  // The current number of rows of the matrix
   , n_( 0UL )  // The current number of columns of the matrix
{
   if( IsVectorizable<Type>::value ) {
      for( size_t i=0UL; i<MM*N; ++i )
         v_[i] = Type();
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Constructor for a matrix of size \f$ m \times n \f$. No element initialization is performed!
//
// \param m The number of rows of the matrix.
// \param n The number of columns of the matrix.
// \exception std::invalid_argument Invalid number of rows for hybrid matrix.
// \exception std::invalid_argument Invalid number of columns for hybrid matrix.
//
// This constructor creates a hybrid matrix of size \f$ m \times n \f$, but leaves the elements
// uninitialized. In case \a m is larger than the maximum allowed number of rows (i.e. \a m > M)
// or \a n is larger than the maximum allowed number of columns a \a std::invalid_argument
// exception is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline HybridMatrix<Type,M,N,true>::HybridMatrix( size_t m, size_t n )
   : v_()     // The statically allocated matrix elements
   , m_( m )  // The current number of rows of the matrix
   , n_( n )  // The current number of columns of the matrix
{
   if( m > M )
      throw std::invalid_argument( "Invalid number of rows for hybrid matrix" );

   if( n > N )
      throw std::invalid_argument( "Invalid number of columns for hybrid matrix" );

   if( IsVectorizable<Type>::value ) {
      for( size_t i=0UL; i<MM*N; ++i )
         v_[i] = Type();
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Constructor for a homogenous initialization of all \f$ m \times n \f$ matrix elements.
//
// \param m The number of rows of the matrix.
// \param n The number of columns of the matrix.
// \param init The initial value of the matrix elements.
// \exception std::invalid_argument Invalid number of rows for hybrid matrix.
// \exception std::invalid_argument Invalid number of columns for hybrid matrix.
//
// This constructor creates a hybrid matrix of size \f$ m \times n \f$ and initializes all
// matrix elements with the specified value. In case \a m is larger than the maximum allowed
// number of rows (i.e. \a m > M) or \a n is larger than the maximum allowed number of columns
// a \a std::invalid_argument exception is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline HybridMatrix<Type,M,N,true>::HybridMatrix( size_t m, size_t n, const Type& init )
   : v_()     // The statically allocated matrix elements
   , m_( m )  // The current number of rows of the matrix
   , n_( n )  // The current number of columns of the matrix
{
   if( m > M )
      throw std::invalid_argument( "Invalid number of rows for hybrid matrix" );

   if( n > N )
      throw std::invalid_argument( "Invalid number of columns for hybrid matrix" );

   for( size_t j=0UL; j<n; ++j ) {
      for( size_t i=0UL; i<m; ++i )
         v_[i+j*MM] = init;

      if( IsVectorizable<Type>::value ) {
         for( size_t i=m; i<MM; ++i )
            v_[i+j*MM] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t j=n; j<N; ++j )
         for( size_t i=0UL; i<MM; ++i )
            v_[i+j*MM] = Type();
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Array initialization of all matrix elements.
//
// \param m The number of rows of the matrix.
// \param n The number of columns of the matrix.
// \param array Dynamic array for the initialization.
// \exception std::invalid_argument Invalid number of rows for hybrid matrix.
// \exception std::invalid_argument Invalid number of columns for hybrid matrix.
//
// This constructor offers the option to directly initialize the elements of the matrix with
// a dynamic array:

   \code
   using blaze::columnMajor;

   int* array = new int[20];
   // ... Initialization of the dynamic array
   blaze::HybridMatrix<int,columnMajor> v( 4UL, 5UL, array );
   delete[] array;
   \endcode

// The matrix is sized accoring to the given size of the array and initialized with the values
// from the given array. In case \a m is larger than the maximum allowed number of rows (i.e.
// \a m > M) or \a n is larger than the maximum allowed number of columns a \a std::invalid_argument
// exception is thrown. Note that it is expected that the given \a array has at least \a m by
// \a n elements. Providing an array with less elements results in undefined behavior!
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N >        // Number of columns
template< typename Other >  // Data type of the initialization array
inline HybridMatrix<Type,M,N,true>::HybridMatrix( size_t m, size_t n, const Other* array )
   : v_()     // The statically allocated matrix elements
   , m_( m )  // The current number of rows of the matrix
   , n_( n )  // The current number of columns of the matrix
{
   if( m > M )
      throw std::invalid_argument( "Invalid number of rows for hybrid matrix" );

   if( n > N )
      throw std::invalid_argument( "Invalid number of columns for hybrid matrix" );

   for( size_t j=0UL; j<n; ++j ) {
      for( size_t i=0UL; i<m; ++i )
         v_[i+j*MM] = array[i+j*m];

      if( IsVectorizable<Type>::value ) {
         for( size_t i=m; i<MM; ++i )
            v_[i+j*MM] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t j=n; j<N; ++j )
         for( size_t i=0UL; i<MM; ++i )
            v_[i+j*MM] = Type();
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Array initialization of all matrix elements.
//
// \param array \f$ M \times N \f$ dimensional array for the initialization.
//
// This constructor offers the option to directly initialize the elements of the matrix with
// a static array:

   \code
   using blaze::columnMajor;

   const int init[3][3] = { { 1, 2, 3 },
                            { 4, 5 },
                            { 7, 8, 9 } };
   blaze::HybridMatrix<int,3,3,columnMajor> A( init );
   \endcode

// The matrix is sized accoring to the size of the array and initialized with the values from
// the given array. Missing values are initialized with default values (as e.g. the value 6 in
// the example).
*/
template< typename Type   // Data type of the matrix
        , size_t M        // Number of rows
        , size_t N >      // Number of columns
template< typename Other  // Data type of the initialization array
        , size_t M2       // Number of rows of the initialization array
        , size_t N2 >     // Number of columns of the initialization array
inline HybridMatrix<Type,M,N,true>::HybridMatrix( const Other (&array)[M2][N2] )
   : v_()      // The statically allocated matrix elements
   , m_( M2 )  // The current number of rows of the matrix
   , n_( N2 )  // The current number of columns of the matrix
{
   BLAZE_STATIC_ASSERT( M2 <= M );
   BLAZE_STATIC_ASSERT( N2 <= N );

   for( size_t j=0UL; j<N2; ++j ) {
      for( size_t i=0UL; i<M2; ++i )
         v_[i+j*MM] = array[i][j];

      if( IsVectorizable<Type>::value ) {
         for( size_t i=M2; i<MM; ++i )
            v_[i+j*MM] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t j=N2; j<N; ++j )
         for( size_t i=0UL; i<MM; ++i )
            v_[i+j*MM] = Type();
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief The copy constructor for HybridMatrix.
//
// \param m Matrix to be copied.
//
// The copy constructor is explicitly defined due to the required dynamic memory management
// and in order to enable/facilitate NRV optimization.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N >        // Number of columns
inline HybridMatrix<Type,M,N,true>::HybridMatrix( const HybridMatrix& m )
   : v_()        // The statically allocated matrix elements
   , m_( m.m_ )  // The current number of rows of the matrix
   , n_( m.n_ )  // The current number of columns of the matrix
{
   for( size_t j=0UL; j<n_; ++j ) {
      for( size_t i=0UL; i<m_; ++i )
         v_[i+j*MM] = m.v_[i+j*MM];

      if( IsVectorizable<Type>::value ) {
         for( size_t i=m_; i<MM; ++i )
            v_[i+j*MM] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t j=n_; j<N; ++j )
         for( size_t i=0UL; i<MM; ++i )
            v_[i+j*MM] = Type();
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Conversion constructor from different matrices.
//
// \param m Matrix to be copied.
// \exception std::invalid_argument Invalid setup of hybrid matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the foreign matrix
        , bool SO2 >     // Storage order of the foreign matrix
inline HybridMatrix<Type,M,N,true>::HybridMatrix( const Matrix<MT,SO2>& m )
   : v_()                  // The statically allocated matrix elements
   , m_( (~m).rows() )     // The current number of rows of the matrix
   , n_( (~m).columns() )  // The current number of columns of the matrix
{
   using blaze::assign;

   if( (~m).rows() > M || (~m).columns() > N )
      throw std::invalid_argument( "Invalid setup of hybrid matrix" );

   for( size_t j=0UL; j<n_; ++j ) {
      for( size_t i=( IsSparseMatrix<MT>::value   ? 0UL : m_ );
                  i<( IsVectorizable<Type>::value ? MM  : m_ ); ++i ) {
         v_[i+j*MM] = Type();
      }
   }

   if( IsVectorizable<Type>::value ) {
      for( size_t j=n_; j<N; ++j )
         for( size_t i=0UL; i<MM; ++i )
            v_[i+j*MM] = Type();
   }

   assign( *this, ~m );
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  DATA ACCESS FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief 2D-access to the matrix elements.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \return Reference to the accessed value.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::Reference
   HybridMatrix<Type,M,N,true>::operator()( size_t i, size_t j )
{
   BLAZE_USER_ASSERT( i<M, "Invalid row access index"    );
   BLAZE_USER_ASSERT( j<N, "Invalid column access index" );
   return v_[i+j*MM];
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief 2D-access to the matrix elements.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \return Reference-to-const to the accessed value.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::ConstReference
   HybridMatrix<Type,M,N,true>::operator()( size_t i, size_t j ) const
{
   BLAZE_USER_ASSERT( i<M, "Invalid row access index"    );
   BLAZE_USER_ASSERT( j<N, "Invalid column access index" );
   return v_[i+j*MM];
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Low-level data access to the matrix elements.
//
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage of the hybrid matrix. Note that you
// can NOT assume that all matrix elements lie adjacent to each other! The hybrid matrix may
// use techniques such as padding to improve the alignment of the data. Whereas the number of
// elements within a column are given by the \c columns() member functions, the total number
// of elements including padding is given by the \c spacing() member function.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::Pointer
   HybridMatrix<Type,M,N,true>::data()
{
   return v_;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Low-level data access to the matrix elements.
//
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage of the hybrid matrix. Note that you
// can NOT assume that all matrix elements lie adjacent to each other! The hybrid matrix may
// use techniques such as padding to improve the alignment of the data. Whereas the number of
// elements within a column are given by the \c columns() member functions, the total number
// of elements including padding is given by the \c spacing() member function.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::ConstPointer
   HybridMatrix<Type,M,N,true>::data() const
{
   return v_;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Low-level data access to the matrix elements of column \a j.
//
// \param j The column index.
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage for the elements in column \a j.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::Pointer
   HybridMatrix<Type,M,N,true>::data( size_t j )
{
   BLAZE_USER_ASSERT( j < N, "Invalid dense matrix column access index" );
   return v_ + j*MM;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Low-level data access to the matrix elements of column \a j.
//
// \param j The column index.
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage for the elements in column \a j
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::ConstPointer
   HybridMatrix<Type,M,N,true>::data( size_t j ) const
{
   BLAZE_USER_ASSERT( j < N, "Invalid dense matrix column access index" );
   return v_ + j*MM;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns an iterator to the first element of column \a j.
//
// \param j The column index.
// \return Iterator to the first element of column \a j.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::Iterator
   HybridMatrix<Type,M,N,true>::begin( size_t j )
{
   BLAZE_USER_ASSERT( j < N, "Invalid dense matrix column access index" );
   return Iterator( v_ + j*MM );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns an iterator to the first element of column \a j.
//
// \param j The column index.
// \return Iterator to the first element of column \a j.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::ConstIterator
   HybridMatrix<Type,M,N,true>::begin( size_t j ) const
{
   BLAZE_USER_ASSERT( j < N, "Invalid dense matrix column access index" );
   return ConstIterator( v_ + j*MM );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns an iterator to the first element of column \a j.
//
// \param j The column index.
// \return Iterator to the first element of column \a j.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::ConstIterator
   HybridMatrix<Type,M,N,true>::cbegin( size_t j ) const
{
   BLAZE_USER_ASSERT( j < N, "Invalid dense matrix column access index" );
   return ConstIterator( v_ + j*MM );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns an iterator just past the last element of column \a j.
//
// \param j The column index.
// \return Iterator just past the last element of column \a j.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::Iterator
   HybridMatrix<Type,M,N,true>::end( size_t j )
{
   BLAZE_USER_ASSERT( j < N, "Invalid dense matrix column access index" );
   return Iterator( v_ + j*MM + M );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns an iterator just past the last element of column \a j.
//
// \param j The column index.
// \return Iterator just past the last element of column \a j.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::ConstIterator
   HybridMatrix<Type,M,N,true>::end( size_t j ) const
{
   BLAZE_USER_ASSERT( j < N, "Invalid dense matrix column access index" );
   return ConstIterator( v_ + j*MM + M );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns an iterator just past the last element of column \a j.
//
// \param j The column index.
// \return Iterator just past the last element of column \a j.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::ConstIterator
   HybridMatrix<Type,M,N,true>::cend( size_t j ) const
{
   BLAZE_USER_ASSERT( j < N, "Invalid dense matrix column access index" );
   return ConstIterator( v_ + j*MM + M );
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  ASSIGNMENT OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Array assignment to all matrix elements.
//
// \param array \f$ M \times N \f$ dimensional array for the assignment.
// \return Reference to the assigned matrix.
//
// This assignment operator offers the option to directly set all elements of the matrix:

   \code
   using blaze::columnMajor;

   const real init[3][3] = { { 1, 2, 3 },
                             { 4, 5 },
                             { 7, 8, 9 } };
   blaze::HybridMatrix<int,3UL,3UL,columnMajor> A;
   A = init;
   \endcode

// The matrix is assigned the values from the given array. Missing values are initialized with
// default values (as e.g. the value 6 in the example).
*/
template< typename Type   // Data type of the matrix
        , size_t M        // Number of rows
        , size_t N >      // Number of columns
template< typename Other  // Data type of the initialization array
        , size_t M2       // Number of rows of the initialization array
        , size_t N2 >     // Number of columns of the initialization array
inline HybridMatrix<Type,M,N,true>&
   HybridMatrix<Type,M,N,true>::operator=( const Other (&array)[M2][N2] )
{
   BLAZE_STATIC_ASSERT( M2 <= M );
   BLAZE_STATIC_ASSERT( N2 <= N );

   resize( M2, N2 );

   for( size_t j=0UL; j<N2; ++j )
      for( size_t i=0UL; i<M2; ++i )
         v_[i+j*MM] = array[i][j];

   return *this;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Homogenous assignment to all matrix elements.
//
// \param set Scalar value to be assigned to all matrix elements.
// \return Reference to the assigned matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline HybridMatrix<Type,M,N,true>&
   HybridMatrix<Type,M,N,true>::operator=( const Type& set )
{
   BLAZE_INTERNAL_ASSERT( m_ <= M, "Invalid number of rows detected"    );
   BLAZE_INTERNAL_ASSERT( n_ <= N, "Invalid number of columns detected" );

   for( size_t j=0UL; j<n_; ++j )
      for( size_t i=0UL; i<m_; ++i )
         v_[i+j*MM] = set;

   return *this;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Copy assignment operator for HybridMatrix.
//
// \param rhs Matrix to be copied.
// \return Reference to the assigned matrix.
//
// Explicit definition of a copy assignment operator for performance reasons.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline HybridMatrix<Type,M,N,true>&
   HybridMatrix<Type,M,N,true>::operator=( const HybridMatrix& rhs )
{
   using blaze::assign;

   BLAZE_INTERNAL_ASSERT( m_ <= M, "Invalid number of rows detected"    );
   BLAZE_INTERNAL_ASSERT( n_ <= N, "Invalid number of columns detected" );

   resize( rhs.rows(), rhs.columns() );
   assign( *this, ~rhs );

   return *this;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Assignment operator for different matrices.
//
// \param rhs Matrix to be copied.
// \return Reference to the assigned matrix.
// \exception std::invalid_argument Invalid assignment to hybrid matrix.
//
// This constructor initializes the matrix as a copy of the given matrix. In case the
// number of rows of the given matrix is not M or the number of columns is not N, a
// \a std::invalid_argument exception is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side matrix
        , bool SO >      // Storage order of the right-hand side matrix
inline HybridMatrix<Type,M,N,true>& HybridMatrix<Type,M,N,true>::operator=( const Matrix<MT,SO>& rhs )
{
   using blaze::assign;

   if( (~rhs).rows() > M || (~rhs).columns() > N )
      throw std::invalid_argument( "Invalid assignment to hybrid matrix" );

   if( (~rhs).canAlias( this ) ) {
      HybridMatrix tmp( ~rhs );
      swap( tmp );
   }
   else {
      resize( (~rhs).rows(), (~rhs).columns() );
      if( IsSparseMatrix<MT>::value )
         reset();
      assign( *this, ~rhs );
   }

   return *this;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Addition assignment operator for the addition of a matrix (\f$ A+=B \f$).
//
// \param rhs The right-hand side matrix to be added to the matrix.
// \return Reference to the matrix.
// \exception std::invalid_argument Matrix sizes do not match.
//
// In case the current sizes of the two matrices don't match, a \a std::invalid_argument exception
// is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side matrix
        , bool SO >      // Storage order of the right-hand side matrix
inline HybridMatrix<Type,M,N,true>& HybridMatrix<Type,M,N,true>::operator+=( const Matrix<MT,SO>& rhs )
{
   using blaze::addAssign;

   if( (~rhs).rows() != m_ || (~rhs).columns() != n_ )
      throw std::invalid_argument( "Matrix sizes do not match" );

   if( (~rhs).canAlias( this ) ) {
      typename MT::ResultType tmp( ~rhs );
      addAssign( *this, tmp );
   }
   else {
      addAssign( *this, ~rhs );
   }

   return *this;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Subtraction assignment operator for the subtraction of a matrix (\f$ A-=B \f$).
//
// \param rhs The right-hand side matrix to be subtracted from the matrix.
// \return Reference to the matrix.
// \exception std::invalid_argument Matrix sizes do not match.
//
// In case the current sizes of the two matrices don't match, a \a std::invalid_argument exception
// is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side matrix
        , bool SO >      // Storage order of the right-hand side matrix
inline HybridMatrix<Type,M,N,true>& HybridMatrix<Type,M,N,true>::operator-=( const Matrix<MT,SO>& rhs )
{
   using blaze::subAssign;

   if( (~rhs).rows() != m_ || (~rhs).columns() != n_ )
      throw std::invalid_argument( "Matrix sizes do not match" );

   if( (~rhs).canAlias( this ) ) {
      typename MT::ResultType tmp( ~rhs );
      subAssign( *this, tmp );
   }
   else {
      subAssign( *this, ~rhs );
   }

   return *this;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Multiplication assignment operator for the multiplication of a matrix (\f$ A*=B \f$).
//
// \param rhs The right-hand side matrix for the multiplication.
// \return Reference to the matrix.
// \exception std::invalid_argument Matrix sizes do not match.
//
// In case the current sizes of the two given matrices don't match, a \a std::invalid_argument
// is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side matrix
        , bool SO >      // Storage order of the right-hand side matrix
inline HybridMatrix<Type,M,N,true>& HybridMatrix<Type,M,N,true>::operator*=( const Matrix<MT,SO>& rhs )
{
   if( n_ != (~rhs).rows() || (~rhs).columns() > N )
      throw std::invalid_argument( "Matrix sizes do not match" );

   HybridMatrix tmp( *this * (~rhs) );
   return this->operator=( tmp );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Multiplication assignment operator for the multiplication between a matrix and
//        a scalar value (\f$ A*=s \f$).
//
// \param rhs The right-hand side scalar value for the multiplication.
// \return Reference to the matrix.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N >        // Number of columns
template< typename Other >  // Data type of the right-hand side scalar
inline typename EnableIf< IsNumeric<Other>, HybridMatrix<Type,M,N,true> >::Type&
   HybridMatrix<Type,M,N,true>::operator*=( Other rhs )
{
   using blaze::assign;

   assign( *this, (*this) * rhs );
   return *this;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Division assignment operator for the division of a matrix by a scalar value
//        (\f$ A/=s \f$).
//
// \param rhs The right-hand side scalar value for the division.
// \return Reference to the matrix.
//
// \b Note: A division by zero is only checked by an user assert.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N >        // Number of columns
template< typename Other >  // Data type of the right-hand side scalar
inline typename EnableIf< IsNumeric<Other>, HybridMatrix<Type,M,N,true> >::Type&
   HybridMatrix<Type,M,N,true>::operator/=( Other rhs )
{
   using blaze::assign;

   BLAZE_USER_ASSERT( rhs != Other(0), "Division by zero detected" );

   assign( *this, (*this) / rhs );
   return *this;
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  UTILITY FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns the current number of rows of the matrix.
//
// \return The number of rows of the matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline size_t HybridMatrix<Type,M,N,true>::rows() const
{
   return m_;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns the current number of columns of the matrix.
//
// \return The number of columns of the matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline size_t HybridMatrix<Type,M,N,true>::columns() const
{
   return n_;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns the spacing between the beginning of two columns.
//
// \return The spacing between the beginning of two columns.
//
// This function returns the spacing between the beginning of two column, i.e. the total number
// of elements of a column.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline size_t HybridMatrix<Type,M,N,true>::spacing() const
{
   return MM;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns the maximum capacity of the matrix.
//
// \return The capacity of the matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline size_t HybridMatrix<Type,M,N,true>::capacity() const
{
   return MM*N;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns the current capacity of the specified column.
//
// \param j The index of the column.
// \return The current capacity of column \a j.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline size_t HybridMatrix<Type,M,N,true>::capacity( size_t j ) const
{
   UNUSED_PARAMETER( j );

   BLAZE_USER_ASSERT( j < columns(), "Invalid column access index" );

   return MM;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns the total number of non-zero elements in the matrix
//
// \return The number of non-zero elements in the dense matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline size_t HybridMatrix<Type,M,N,true>::nonZeros() const
{
   size_t nonzeros( 0UL );

   for( size_t j=0UL; j<n_; ++j )
      for( size_t i=0UL; i<m_; ++i )
         if( !isDefault( v_[i+j*MM] ) )
            ++nonzeros;

   return nonzeros;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns the number of non-zero elements in the specified column.
//
// \param j The index of the column.
// \return The number of non-zero elements of column \a j.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline size_t HybridMatrix<Type,M,N,true>::nonZeros( size_t j ) const
{
   BLAZE_USER_ASSERT( j < columns(), "Invalid column access index" );

   const size_t iend( j*MM+m_ );
   size_t nonzeros( 0UL );

   for( size_t i=j*MM; i<iend; ++i )
      if( !isDefault( v_[i] ) )
         ++nonzeros;

   return nonzeros;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Reset to the default initial values.
//
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::reset()
{
   using blaze::reset;

   for( size_t j=0UL; j<n_; ++j )
      for( size_t i=0UL; i<m_; ++i )
         reset( v_[i+j*MM] );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Reset the specified column to the default initial values.
//
// \param j The index of the column.
// \return void
//
// This function reset the values in the specified column to their default value. Note that
// the capacity of the column remains unchanged.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::reset( size_t j )
{
   using blaze::reset;

   BLAZE_USER_ASSERT( j < columns(), "Invalid column access index" );
   for( size_t i=0UL; i<m_; ++i )
      reset( v_[i+j*MM] );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Clearing the hybrid matrix.
//
// \return void
//
// After the clear() function, the size of the matrix is 0.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::clear()
{
   resize( 0UL, 0UL );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Changing the size of the matrix.
//
// \param m The new number of rows of the matrix.
// \param n The new number of columns of the matrix.
// \param preserve \a true if the old values of the matrix should be preserved, \a false if not.
// \return void
//
// This function resizes the matrix using the given size to \f$ m \times n \f$. In case the given
// number of rows \a m is larger than the maximum number of rows (i.e. if m > M) or in case the
// given number of columns \a n is larger than the maximum number of column (i.e. if n > N) a
// \a std::invalid_argument exception is thrown. Note that this function may invalidate all
// existing views (submatrices, rows, columns, ...) on the matrix if it is used to shrink the
// matrix. Additionally, during this operation all matrix elements are potentially changed. In
// order to preserve the old matrix values, the \a preserve flag can be set to \a true.
//
// Note that in case the number of rows or columns is increased new matrix elements are not
// initialized! The following example illustrates the resize operation of a \f$ 2 \times 4 \f$
// matrix to a \f$ 4 \times 2 \f$ matrix. The new, uninitialized elements are marked with \a x:

                              \f[
                              \left(\begin{array}{*{4}{c}}
                              1 & 2 & 3 & 4 \\
                              5 & 6 & 7 & 8 \\
                              \end{array}\right)

                              \Longrightarrow

                              \left(\begin{array}{*{2}{c}}
                              1 & 2 \\
                              5 & 6 \\
                              x & x \\
                              x & x \\
                              \end{array}\right)
                              \f]
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
void HybridMatrix<Type,M,N,true>::resize( size_t m, size_t n, bool preserve )
{
   UNUSED_PARAMETER( preserve );

   if( m > M )
      throw std::invalid_argument( "Invalid number of rows for hybrid matrix" );

   if( n > N )
      throw std::invalid_argument( "Invalid number of columns for hybrid matrix" );

   if( IsVectorizable<Type>::value && m < m_ ) {
      for( size_t j=0UL; j<n; ++j )
         for( size_t i=m; i<m_; ++i )
            v_[i+j*MM] = Type();
   }

   if( IsVectorizable<Type>::value && n < n_ ) {
      for( size_t j=n; j<n_; ++j )
         for( size_t i=0UL; i<m_; ++i )
            v_[i+j*MM] = Type();
   }

   m_ = m;
   n_ = n;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Extending the size of the matrix.
//
// \param m Number of additional rows.
// \param n Number of additional columns.
// \param preserve \a true if the old values of the matrix should be preserved, \a false if not.
// \return void
//
// This function increases the matrix size by \a m rows and \a n columns. In case the resulting
// number of rows or columns is larger than the maximum number of rows or columns (i.e. if m > M
// or n > N) a \a std::invalid_argument exception is thrown. During this operation, all matrix
// elements are potentially changed. In order to preserve the old matrix values, the \a preserve
// flag can be set to \a true.\n
// Note that new matrix elements are not initialized!
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::extend( size_t m, size_t n, bool preserve )
{
   UNUSED_PARAMETER( preserve );
   resize( m_+m, n_+n );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Transposing the matrix.
//
// \return Reference to the transposed matrix.
// \exception std::logic_error Impossible transpose operation.
//
// This function transposes the hybrid matrix in-place. Note that this function can only be used
// on hybrid matrices whose current dimensions allow an in-place transpose operation. In case the
// current number of rows is larger than the maximum number of columns or if the current number
// of columns is larger than the maximum number of rows, an \a std::logic_error is thrown.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline HybridMatrix<Type,M,N,true>& HybridMatrix<Type,M,N,true>::transpose()
{
   using std::swap;

   if( m_ > N || n_ > M )
      throw std::logic_error( "Impossible transpose operation" );

   const size_t maxsize( max( m_, n_ ) );
   for( size_t j=1UL; j<maxsize; ++j )
      for( size_t i=0UL; i<j; ++i )
         swap( v_[i+j*MM], v_[j+i*MM] );

   if( IsVectorizable<Type>::value && n_ < m_ ) {
      for( size_t j=0UL; j<n_; ++j ) {
         for( size_t i=n_; i<m_; ++i ) {
            v_[i+j*MM] = Type();
         }
      }
   }

   if( IsVectorizable<Type>::value && n_ > m_ ) {
      for( size_t j=m_; j<n_; ++j )
         for( size_t i=0UL; i<m_; ++i )
            v_[i+j*MM] = Type();
   }

   swap( m_, n_ );

   return *this;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Scaling of the matrix by the scalar value \a scalar (\f$ A*=s \f$).
//
// \param scalar The scalar value for the matrix scaling.
// \return Reference to the matrix.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N >        // Number of columns
template< typename Other >  // Data type of the scalar value
inline HybridMatrix<Type,M,N,true>&
   HybridMatrix<Type,M,N,true>::scale( const Other& scalar )
{
   for( size_t j=0UL; j<n_; ++j )
      for( size_t i=0UL; i<m_; ++i )
         v_[i+j*MM] *= scalar;

   return *this;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Swapping the contents of two hybrid matrices.
//
// \param m The matrix to be swapped.
// \return void
// \exception no-throw guarantee.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::swap( HybridMatrix& m ) /* throw() */
{
   using std::swap;

   const size_t maxrows( max( m_, m.m_ ) );
   const size_t maxcols( max( n_, m.n_ ) );

   for( size_t j=0UL; j<maxcols; ++j ) {
      for( size_t i=0UL; i<maxrows; ++i ) {
         swap( v_[i+j*MM], m(i,j) );
      }
   }

   swap( m_, m.m_ );
   swap( n_, m.n_ );
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  MEMORY FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Class specific implementation of operator new.
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridMatrix class template.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void* HybridMatrix<Type,M,N,true>::operator new( std::size_t size )
{
   UNUSED_PARAMETER( size );

   BLAZE_INTERNAL_ASSERT( size == sizeof( HybridMatrix ), "Invalid number of bytes detected" );

   return allocate<HybridMatrix>( 1UL );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Class specific implementation of operator new[].
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridMatrix class template.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void* HybridMatrix<Type,M,N,true>::operator new[]( std::size_t size )
{
   BLAZE_INTERNAL_ASSERT( size >= sizeof( HybridMatrix )       , "Invalid number of bytes detected" );
   BLAZE_INTERNAL_ASSERT( size %  sizeof( HybridMatrix ) == 0UL, "Invalid number of bytes detected" );

   return allocate<HybridMatrix>( size/sizeof(HybridMatrix) );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Class specific implementation of the no-throw operator new.
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridMatrix class template.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void* HybridMatrix<Type,M,N,true>::operator new( std::size_t size, const std::nothrow_t& )
{
   UNUSED_PARAMETER( size );

   BLAZE_INTERNAL_ASSERT( size == sizeof( HybridMatrix ), "Invalid number of bytes detected" );

   return allocate<HybridMatrix>( 1UL );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Class specific implementation of the no-throw operator new[].
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridMatrix class template.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void* HybridMatrix<Type,M,N,true>::operator new[]( std::size_t size, const std::nothrow_t& )
{
   BLAZE_INTERNAL_ASSERT( size >= sizeof( HybridMatrix )       , "Invalid number of bytes detected" );
   BLAZE_INTERNAL_ASSERT( size %  sizeof( HybridMatrix ) == 0UL, "Invalid number of bytes detected" );

   return allocate<HybridMatrix>( size/sizeof(HybridMatrix) );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Class specific implementation of operator delete.
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::operator delete( void* ptr )
{
   deallocate( static_cast<HybridMatrix*>( ptr ) );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Class specific implementation of operator delete[].
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::operator delete[]( void* ptr )
{
   deallocate( static_cast<HybridMatrix*>( ptr ) );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Class specific implementation of no-throw operator delete.
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::operator delete( void* ptr, const std::nothrow_t& )
{
   deallocate( static_cast<HybridMatrix*>( ptr ) );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Class specific implementation of no-throw operator delete[].
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::operator delete[]( void* ptr, const std::nothrow_t& )
{
   deallocate( static_cast<HybridMatrix*>( ptr ) );
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  EXPRESSION TEMPLATE EVALUATION FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns whether the matrix can alias with the given address \a alias.
//
// \param alias The alias to be checked.
// \return \a true in case the alias corresponds to this matrix, \a false if not.
//
// This function returns whether the given address can alias with the matrix. In contrast
// to the isAliased() function this function is allowed to use compile time expressions
// to optimize the evaluation.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N >        // Number of columns
template< typename Other >  // Data type of the foreign expression
inline bool HybridMatrix<Type,M,N,true>::canAlias( const Other* alias ) const
{
   return static_cast<const void*>( this ) == static_cast<const void*>( alias );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns whether the matrix is aliased with the given address \a alias.
//
// \param alias The alias to be checked.
// \return \a true in case the alias corresponds to this matrix, \a false if not.
//
// This function returns whether the given address is aliased with the matrix. In contrast
// to the conAlias() function this function is not allowed to use compile time expressions
// to optimize the evaluation.
*/
template< typename Type     // Data type of the matrix
        , size_t M          // Number of rows
        , size_t N >        // Number of columns
template< typename Other >  // Data type of the foreign expression
inline bool HybridMatrix<Type,M,N,true>::isAliased( const Other* alias ) const
{
   return static_cast<const void*>( this ) == static_cast<const void*>( alias );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Returns whether the matrix is properly aligned in memory.
//
// \return \a true in case the matrix is aligned, \a false if not.
//
// This function returns whether the matrix is guaranteed to be properly aligned in memory, i.e.
// whether the beginning and the end of each column of the matrix are guaranteed to conform to
// the alignment restrictions of the element type \a Type.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline bool HybridMatrix<Type,M,N,true>::isAligned() const
{
   return true;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Aligned load of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \return The loaded intrinsic element.
//
// This function performs an aligned load of a specific intrinsic element of the dense matrix.
// The row index must be smaller than the number of rows and the column index must be smaller
// than the number of columns. Additionally, the row index must be a multiple of the number of
// values inside the intrinsic element. This function must \b NOT be called explicitly! It is
// used internally for the performance optimized evaluation of expression templates. Calling
// this function explicitly might result in erroneous results and/or in compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::IntrinsicType
   HybridMatrix<Type,M,N,true>::load( size_t i, size_t j ) const
{
   using blaze::load;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_ , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( i + IT::size <= MM , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( i % IT::size == 0UL, "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_ , "Invalid column access index" );

   return load( &v_[i+j*MM] );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Unaligned load of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \return The loaded intrinsic element.
//
// This function performs an unaligned load of a specific intrinsic element of the dense matrix.
// The row index must be smaller than the number of rows and the column index must be smaller
// than the number of columns. Additionally, the row index must be a multiple of the number of
// values inside the intrinsic element. This function must \b NOT be called explicitly! It is
// used internally for the performance optimized evaluation of expression templates. Calling
// this function explicitly might result in erroneous results and/or in compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline typename HybridMatrix<Type,M,N,true>::IntrinsicType
   HybridMatrix<Type,M,N,true>::loadu( size_t i, size_t j ) const
{
   using blaze::loadu;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_ , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( i + IT::size <= MM , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_ , "Invalid column access index" );

   return loadu( &v_[i+j*MM] );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Aligned store of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \param value The intrinsic element to be stored.
// \return void
//
// This function performs an aligned store of a specific intrinsic element of the dense matrix.
// The row index must be smaller than the number of rows and the column index must be smaller
// than the number of columns. Additionally, the row index must be a multiple of the number of
// values inside the intrinsic element. This function must \b NOT be called explicitly! It is
// used internally for the performance optimized evaluation of expression templates. Calling
// this function explicitly might result in erroneous results and/or in compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::store( size_t i, size_t j, const IntrinsicType& value )
{
   using blaze::store;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_ , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( i + IT::size <= MM , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( i % IT::size == 0UL, "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_ , "Invalid column access index" );

   store( &v_[i+j*MM], value );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Unaligned store of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \param value The intrinsic element to be stored.
// \return void
//
// This function performs an unaligned store of a specific intrinsic element of the dense matrix.
// The row index must be smaller than the number of rows and the column index must be smaller
// than the number of columns. Additionally, the row index must be a multiple of the number of
// values inside the intrinsic element. This function must \b NOT be called explicitly! It is
// used internally for the performance optimized evaluation of expression templates. Calling
// this function explicitly might result in erroneous results and/or in compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::storeu( size_t i, size_t j, const IntrinsicType& value )
{
   using blaze::storeu;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_, "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( i + IT::size <= MM, "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_, "Invalid column access index" );

   storeu( &v_[i+j*MM], value );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Aligned, non-temporal store of an intrinsic element of the matrix.
//
// \param i Access index for the row. The index has to be in the range [0..M-1].
// \param j Access index for the column. The index has to be in the range [0..N-1].
// \param value The intrinsic element to be stored.
// \return void
//
// This function performs an aligned, non-temporal store of a specific intrinsic element of the
// dense matrix. // The row index must be smaller than the number of rows and the column index
// must be smaller than the number of columns. Additionally, the row index must be a multiple
// of the number of values inside the intrinsic element. This function must \b NOT be called
// explicitly! It is used internally for the performance optimized evaluation of expression
// templates. Calling this function explicitly might result in erroneous results and/or in
// compilation errors.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
inline void HybridMatrix<Type,M,N,true>::stream( size_t i, size_t j, const IntrinsicType& value )
{
   using blaze::stream;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( i            <  m_ , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( i + IT::size <= MM , "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( i % IT::size == 0UL, "Invalid row access index"    );
   BLAZE_INTERNAL_ASSERT( j            <  n_ , "Invalid column access index" );

   stream( &v_[i+j*MM], value );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO >      // Storage order of the right-hand side dense matrix
inline typename DisableIf< typename HybridMatrix<Type,M,N,true>::BLAZE_TEMPLATE VectorizedAssign<MT> >::Type
   HybridMatrix<Type,M,N,true>::assign( const DenseMatrix<MT,SO>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   for( size_t j=0UL; j<n_; ++j ) {
      for( size_t i=0UL; i<m_; ++i ) {
         v_[i+j*MM] = (~rhs)(i,j);
      }
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Intrinsic optimized implementation of the assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO >      // Storage order of the right-hand side dense matrix
inline typename EnableIf< typename HybridMatrix<Type,M,N,true>::BLAZE_TEMPLATE VectorizedAssign<MT> >::Type
   HybridMatrix<Type,M,N,true>::assign( const DenseMatrix<MT,SO>& rhs )
{
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t j=0UL; j<n_; ++j ) {
      for( size_t i=0UL; i<m_; i+=IT::size ) {
         store( &v_[i+j*MM], (~rhs).load(i,j) );
      }
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the assignment of a column-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,true>::assign( const SparseMatrix<MT,true>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t j=0UL; j<n_; ++j )
      for( RhsConstIterator element=(~rhs).begin(j); element!=(~rhs).end(j); ++element )
         v_[element->index()+j*MM] = element->value();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the assignment of a row-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,true>::assign( const SparseMatrix<MT,false>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t i=0UL; i<m_; ++i )
      for( RhsConstIterator element=(~rhs).begin(i); element!=(~rhs).end(i); ++element )
         v_[i+element->index()*MM] = element->value();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the addition assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO >      // Storage order of the right-hand side dense matrix
inline typename DisableIf< typename HybridMatrix<Type,M,N,true>::BLAZE_TEMPLATE VectorizedAddAssign<MT> >::Type
   HybridMatrix<Type,M,N,true>::addAssign( const DenseMatrix<MT,SO>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   for( size_t j=0UL; j<n_; ++j ) {
      for( size_t i=0UL; i<m_; ++i ) {
         v_[i+j*MM] += (~rhs)(i,j);
      }
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Intrinsic optimized implementation of the addition assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO >      // Storage order of the right-hand side dense matrix
inline typename EnableIf< typename HybridMatrix<Type,M,N,true>::BLAZE_TEMPLATE VectorizedAddAssign<MT> >::Type
   HybridMatrix<Type,M,N,true>::addAssign( const DenseMatrix<MT,SO>& rhs )
{
   using blaze::load;
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t j=0UL; j<n_; ++j ) {
      for( size_t i=0UL; i<m_; i+=IT::size ) {
         store( &v_[i+j*MM], load( &v_[i+j*MM] ) + (~rhs).load(i,j) );
      }
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the addition assignment of a column-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,true>::addAssign( const SparseMatrix<MT,true>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t j=0UL; j<n_; ++j )
      for( RhsConstIterator element=(~rhs).begin(j); element!=(~rhs).end(j); ++element )
         v_[element->index()+j*MM] += element->value();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the addition assignment of a row-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,true>::addAssign( const SparseMatrix<MT,false>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t i=0UL; i<m_; ++i )
      for( RhsConstIterator element=(~rhs).begin(i); element!=(~rhs).end(i); ++element )
         v_[i+element->index()*MM] += element->value();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the subtraction assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO >      // Storage order of the right-hand side dense matrix
inline typename DisableIf< typename HybridMatrix<Type,M,N,true>::BLAZE_TEMPLATE VectorizedSubAssign<MT> >::Type
   HybridMatrix<Type,M,N,true>::subAssign( const DenseMatrix<MT,SO>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   for( size_t j=0UL; j<n_; ++j ) {
      for( size_t i=0UL; i<m_; ++i ) {
         v_[i+j*MM] -= (~rhs)(i,j);
      }
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Intrinsic optimized implementation of the subtraction assignment of a dense matrix.
//
// \param rhs The right-hand side dense matrix to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT    // Type of the right-hand side dense matrix
        , bool SO >      // Storage order of the right-hand side dense matrix
inline typename EnableIf< typename HybridMatrix<Type,M,N,true>::BLAZE_TEMPLATE VectorizedSubAssign<MT> >::Type
   HybridMatrix<Type,M,N,true>::subAssign( const DenseMatrix<MT,SO>& rhs )
{
   using blaze::load;
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t j=0UL; j<n_; ++j ) {
      for( size_t i=0UL; i<m_; i+=IT::size ) {
         store( &v_[i+j*MM], load( &v_[i+j*MM] ) - (~rhs).load(i,j) );
      }
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the subtraction assignment of a column-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,true>::subAssign( const SparseMatrix<MT,true>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t j=0UL; j<n_; ++j )
      for( RhsConstIterator element=(~rhs).begin(j); element!=(~rhs).end(j); ++element )
         v_[element->index()+j*MM] -= element->value();
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Default implementation of the subtraction assignment of a row-major sparse matrix.
//
// \param rhs The right-hand side sparse matrix to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N >     // Number of columns
template< typename MT >  // Type of the right-hand side sparse matrix
inline void HybridMatrix<Type,M,N,true>::subAssign( const SparseMatrix<MT,false>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).rows() == m_ && (~rhs).columns() == n_, "Invalid matrix size" );

   typedef typename MT::ConstIterator  RhsConstIterator;

   for( size_t i=0UL; i<m_; ++i )
      for( RhsConstIterator element=(~rhs).begin(i); element!=(~rhs).end(i); ++element )
         v_[i+element->index()*MM] -= element->value();
}
/*! \endcond */
//*************************************************************************************************








//=================================================================================================
//
//  UNDEFINED CLASS TEMPLATE SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Specialization of HybridMatrix for zero columns.
// \ingroup hybrid_matrix
//
// This specialization of the HybridMatrix class template is left undefined and therefore
// prevents the instantiation for zero columns.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , bool SO >      // Storage order
class HybridMatrix<Type,M,0UL,SO>;
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Specialization of HybridMatrix for zero rows.
// \ingroup hybrid_matrix
//
// This specialization of the HybridMatrix class template is left undefined and therefore
// prevents the instantiation for zero rows.
*/
template< typename Type  // Data type of the matrix
        , size_t N       // Number of columns
        , bool SO >      // Storage order
class HybridMatrix<Type,0UL,N,SO>;
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Specialization of HybridMatrix for 0 rows and 0 columns.
// \ingroup hybrid_matrix
//
// This specialization of the HybridMatrix class template is left undefined and therefore
// prevents the instantiation for 0 rows and 0 columns.
*/
template< typename Type  // Data type of the matrix
        , bool SO >      // Storage order
class HybridMatrix<Type,0UL,0UL,SO>;
/*! \endcond */
//*************************************************************************************************








//=================================================================================================
//
//  STATICMATRIX OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*!\name HybridMatrix operators */
//@{
template< typename Type, size_t M, size_t N, bool SO >
inline void reset( HybridMatrix<Type,M,N,SO>& m );

template< typename Type, size_t M, size_t N, bool SO >
inline void clear( HybridMatrix<Type,M,N,SO>& m );

template< typename Type, size_t M, size_t N, bool SO >
inline bool isDefault( const HybridMatrix<Type,M,N,SO>& m );

template< typename Type, size_t M, size_t N, bool SO >
inline void swap( HybridMatrix<Type,M,N,SO>& a, HybridMatrix<Type,M,N,SO>& b ) /* throw() */;
//@}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Resetting the given hybrid matrix.
// \ingroup hybrid_matrix
//
// \param m The matrix to be resetted.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void reset( HybridMatrix<Type,M,N,SO>& m )
{
   m.reset();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Clearing the given hybrid matrix.
// \ingroup hybrid_matrix
//
// \param m The matrix to be cleared.
// \return void
//
// Clearing a hybrid matrix is equivalent to resetting it via the reset() function.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void clear( HybridMatrix<Type,M,N,SO>& m )
{
   m.reset();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns whether the given hybrid matrix is in default state.
// \ingroup hybrid_matrix
//
// \param m The matrix to be tested for its default state.
// \return \a true in case the given matrix is component-wise zero, \a false otherwise.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline bool isDefault( const HybridMatrix<Type,M,N,SO>& m )
{
   if( SO == rowMajor ) {
      for( size_t i=0UL; i<m.rows(); ++i )
         for( size_t j=0UL; j<m.columns(); ++j )
            if( !isDefault( m(i,j) ) ) return false;
   }
   else {
      for( size_t j=0UL; j<m.columns(); ++j )
         for( size_t i=0UL; i<m.rows(); ++i )
            if( !isDefault( m(i,j) ) ) return false;
   }

   return true;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Swapping the contents of two hybrid matrices.
// \ingroup hybrid_matrix
//
// \param a The first matrix to be swapped.
// \param b The second matrix to be swapped.
// \return void
// \exception no-throw guarantee.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void swap( HybridMatrix<Type,M,N,SO>& a, HybridMatrix<Type,M,N,SO>& b ) /* throw() */
{
   a.swap( b );
}
//*************************************************************************************************




//=================================================================================================
//
//  ADDTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M1, size_t N1, bool SO, typename T2, size_t M2, size_t N2 >
struct AddTrait< HybridMatrix<T1,M1,N1,SO>, StaticMatrix<T2,M2,N2,SO> >
{
   typedef StaticMatrix< typename AddTrait<T1,T2>::Type, M2, N2, SO >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO1, typename T2, size_t M2, size_t N2, bool SO2 >
struct AddTrait< HybridMatrix<T1,M1,N1,SO1>, StaticMatrix<T2,M2,N2,SO2> >
{
   typedef StaticMatrix< typename AddTrait<T1,T2>::Type, M2, N2, false >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO, typename T2, size_t M2, size_t N2 >
struct AddTrait< StaticMatrix<T1,M1,N1,SO>, HybridMatrix<T2,M2,N2,SO> >
{
   typedef StaticMatrix< typename AddTrait<T1,T2>::Type, M2, N2, SO >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO1, typename T2, size_t M2, size_t N2, bool SO2 >
struct AddTrait< StaticMatrix<T1,M1,N1,SO1>, HybridMatrix<T2,M2,N2,SO2> >
{
   typedef StaticMatrix< typename AddTrait<T1,T2>::Type, M2, N2, false >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO, typename T2, size_t M2, size_t N2 >
struct AddTrait< HybridMatrix<T1,M1,N1,SO>, HybridMatrix<T2,M2,N2,SO> >
{
   typedef HybridMatrix< typename AddTrait<T1,T2>::Type, ( M1 < M2 )?( M1 ):( M2 ), ( N1 < N2 )?( N1 ):( N2 ), SO >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO1, typename T2, size_t M2, size_t N2, bool SO2 >
struct AddTrait< HybridMatrix<T1,M1,N1,SO1>, HybridMatrix<T2,M2,N2,SO2> >
{
   typedef HybridMatrix< typename AddTrait<T1,T2>::Type, ( M1 < M2 )?( M1 ):( M2 ), ( N1 < N2 )?( N1 ):( N2 ), false >  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  SUBTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M1, size_t N1, bool SO, typename T2, size_t M2, size_t N2 >
struct SubTrait< HybridMatrix<T1,M1,N1,SO>, StaticMatrix<T2,M2,N2,SO> >
{
   typedef StaticMatrix< typename SubTrait<T1,T2>::Type, M2, N2, SO >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO1, typename T2, size_t M2, size_t N2, bool SO2 >
struct SubTrait< HybridMatrix<T1,M1,N1,SO1>, StaticMatrix<T2,M2,N2,SO2> >
{
   typedef StaticMatrix< typename SubTrait<T1,T2>::Type, M2, N2, false >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO, typename T2, size_t M2, size_t N2 >
struct SubTrait< StaticMatrix<T1,M1,N1,SO>, HybridMatrix<T2,M2,N2,SO> >
{
   typedef StaticMatrix< typename SubTrait<T1,T2>::Type, M2, N2, SO >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO1, typename T2, size_t M2, size_t N2, bool SO2 >
struct SubTrait< StaticMatrix<T1,M1,N1,SO1>, HybridMatrix<T2,M2,N2,SO2> >
{
   typedef StaticMatrix< typename SubTrait<T1,T2>::Type, M2, N2, false >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO, typename T2, size_t M2, size_t N2 >
struct SubTrait< HybridMatrix<T1,M1,N1,SO>, HybridMatrix<T2,M2,N2,SO> >
{
   typedef HybridMatrix< typename SubTrait<T1,T2>::Type, ( M1 < M2 )?( M1 ):( M2 ), ( N1 < N2 )?( N1 ):( N2 ), SO >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO1, typename T2, size_t M2, size_t N2, bool SO2 >
struct SubTrait< HybridMatrix<T1,M1,N1,SO1>, HybridMatrix<T2,M2,N2,SO2> >
{
   typedef HybridMatrix< typename SubTrait<T1,T2>::Type, ( M1 < M2 )?( M1 ):( M2 ), ( N1 < N2 )?( N1 ):( N2 ), false >  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  MULTTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M, size_t N, bool SO, typename T2 >
struct MultTrait< HybridMatrix<T1,M,N,SO>, T2 >
{
   typedef HybridMatrix< typename MultTrait<T1,T2>::Type, M, N, SO >  Type;
   BLAZE_CONSTRAINT_MUST_BE_NUMERIC_TYPE( T2 );
};

template< typename T1, typename T2, size_t M, size_t N, bool SO >
struct MultTrait< T1, HybridMatrix<T2,M,N,SO> >
{
   typedef HybridMatrix< typename MultTrait<T1,T2>::Type, M, N, SO >  Type;
   BLAZE_CONSTRAINT_MUST_BE_NUMERIC_TYPE( T1 );
};

template< typename T1, size_t M, size_t N, bool SO, typename T2, size_t K >
struct MultTrait< HybridMatrix<T1,M,N,SO>, StaticVector<T2,K,false> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, M, false >  Type;
};

template< typename T1, size_t K, typename T2, size_t M, size_t N, bool SO >
struct MultTrait< StaticVector<T1,K,true>, HybridMatrix<T2,M,N,SO> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, N, true >  Type;
};

template< typename T1, size_t M, size_t N, bool SO, typename T2, size_t K >
struct MultTrait< HybridMatrix<T1,M,N,SO>, HybridVector<T2,K,false> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, M, false >  Type;
};

template< typename T1, size_t K, typename T2, size_t M, size_t N, bool SO >
struct MultTrait< HybridVector<T1,K,true>, HybridMatrix<T2,M,N,SO> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, N, true >  Type;
};

template< typename T1, size_t M, size_t N, bool SO, typename T2 >
struct MultTrait< HybridMatrix<T1,M,N,SO>, DynamicVector<T2,false> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, M, false >  Type;
};

template< typename T1, typename T2, size_t M, size_t N, bool SO >
struct MultTrait< DynamicVector<T1,true>, HybridMatrix<T2,M,N,SO> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, N, true >  Type;
};

template< typename T1, size_t M, size_t N, bool SO, typename T2 >
struct MultTrait< HybridMatrix<T1,M,N,SO>, CompressedVector<T2,false> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, M, false >  Type;
};

template< typename T1, typename T2, size_t M, size_t N, bool SO >
struct MultTrait< CompressedVector<T1,true>, HybridMatrix<T2,M,N,SO> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, N, true >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO1, typename T2, size_t M2, size_t N2, bool SO2 >
struct MultTrait< HybridMatrix<T1,M1,N1,SO1>, StaticMatrix<T2,M2,N2,SO2> >
{
   typedef HybridMatrix< typename MultTrait<T1,T2>::Type, M1, N2, SO1 >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO1, typename T2, size_t M2, size_t N2, bool SO2 >
struct MultTrait< StaticMatrix<T1,M1,N1,SO1>, HybridMatrix<T2,M2,N2,SO2> >
{
   typedef HybridMatrix< typename MultTrait<T1,T2>::Type, M1, N2, SO1 >  Type;
};

template< typename T1, size_t M1, size_t N1, bool SO1, typename T2, size_t M2, size_t N2, bool SO2 >
struct MultTrait< HybridMatrix<T1,M1,N1,SO1>, HybridMatrix<T2,M2,N2,SO2> >
{
   typedef HybridMatrix< typename MultTrait<T1,T2>::Type, M1, N2, SO1 >  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  DIVTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M, size_t N, bool SO, typename T2 >
struct DivTrait< HybridMatrix<T1,M,N,SO>, T2 >
{
   typedef HybridMatrix< typename DivTrait<T1,T2>::Type, M, N, SO >  Type;
   BLAZE_CONSTRAINT_MUST_BE_NUMERIC_TYPE( T2 );
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  MATHTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M, size_t N, bool SO, typename T2 >
struct MathTrait< HybridMatrix<T1,M,N,SO>, HybridMatrix<T2,M,N,SO> >
{
   typedef HybridMatrix< typename MathTrait<T1,T2>::HighType, M, N, SO >  HighType;
   typedef HybridMatrix< typename MathTrait<T1,T2>::LowType , M, N, SO >  LowType;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  SUBMATRIXTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M, size_t N, bool SO >
struct SubmatrixTrait< HybridMatrix<T1,M,N,SO> >
{
   typedef HybridMatrix<T1,M,N,SO>  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  ROWTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M, size_t N, bool SO >
struct RowTrait< HybridMatrix<T1,M,N,SO> >
{
   typedef HybridVector<T1,N,true>  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  COLUMNTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M, size_t N, bool SO >
struct ColumnTrait< HybridMatrix<T1,M,N,SO> >
{
   typedef HybridVector<T1,M,false>  Type;
};
/*! \endcond */
//*************************************************************************************************

} // namespace blaze

#endif
