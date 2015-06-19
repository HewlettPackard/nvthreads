//=================================================================================================
/*!
//  \file blaze/math/DenseVector.h
//  \brief Header file for all basic DenseVector functionality
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

#ifndef _BLAZE_MATH_DENSEVECTOR_H_
#define _BLAZE_MATH_DENSEVECTOR_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <cmath>
#include <blaze/math/expressions/DenseVector.h>
#include <blaze/math/expressions/DVecAbsExpr.h>
#include <blaze/math/expressions/DVecDVecAddExpr.h>
#include <blaze/math/expressions/DVecDVecCrossExpr.h>
#include <blaze/math/expressions/DVecDVecMultExpr.h>
#include <blaze/math/expressions/DVecDVecSubExpr.h>
#include <blaze/math/expressions/DVecEvalExpr.h>
#include <blaze/math/expressions/DVecScalarDivExpr.h>
#include <blaze/math/expressions/DVecScalarMultExpr.h>
#include <blaze/math/expressions/DVecSerialExpr.h>
#include <blaze/math/expressions/DVecSVecAddExpr.h>
#include <blaze/math/expressions/DVecSVecCrossExpr.h>
#include <blaze/math/expressions/DVecSVecSubExpr.h>
#include <blaze/math/expressions/DVecTransExpr.h>
#include <blaze/math/expressions/SparseVector.h>
#include <blaze/math/expressions/SVecDVecCrossExpr.h>
#include <blaze/math/expressions/SVecDVecSubExpr.h>
#include <blaze/math/expressions/SVecSVecCrossExpr.h>
#include <blaze/math/expressions/TDVecDVecMultExpr.h>
#include <blaze/math/Functions.h>
#include <blaze/math/shims/Equal.h>
#include <blaze/math/shims/IsDefault.h>
#include <blaze/math/shims/IsNaN.h>
#include <blaze/math/shims/Square.h>
#include <blaze/math/smp/DenseVector.h>
#include <blaze/math/smp/SparseVector.h>
#include <blaze/math/traits/CMathTrait.h>
#include <blaze/math/TransposeFlag.h>
#include <blaze/math/Vector.h>
#include <blaze/util/Assert.h>
#include <blaze/util/constraints/Numeric.h>
#include <blaze/util/EnableIf.h>
#include <blaze/util/Types.h>
#include <blaze/util/typetraits/IsNumeric.h>
#include <blaze/util/typetraits/RemoveReference.h>


namespace blaze {

//=================================================================================================
//
//  GLOBAL OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*!\name DenseVector operators */
//@{
template< typename T1, bool TF1, typename T2, bool TF2 >
inline bool operator==( const DenseVector<T1,TF1>& lhs, const DenseVector<T2,TF2>& rhs );

template< typename T1, bool TF1, typename T2, bool TF2 >
inline bool operator==( const DenseVector<T1,TF1>& lhs, const SparseVector<T2,TF2>& rhs );

template< typename T1, bool TF1, typename T2, bool TF2 >
inline bool operator==( const SparseVector<T1,TF1>& lhs, const DenseVector<T2,TF2>& rhs );

template< typename T1, typename T2, bool TF >
inline typename EnableIf< IsNumeric<T2>, bool >::Type
   operator==( const DenseVector<T1,TF>& vec, T2 scalar );

template< typename T1, typename T2, bool TF >
inline typename EnableIf< IsNumeric<T1>, bool >::Type
   operator==( T1 scalar, const DenseVector<T2,TF>& vec );

template< typename T1, bool TF1, typename T2, bool TF2 >
inline bool operator!=( const DenseVector<T1,TF1>& lhs, const DenseVector<T2,TF2>& rhs );

template< typename T1, bool TF1, typename T2, bool TF2 >
inline bool operator!=( const DenseVector<T1,TF1>& lhs, const SparseVector<T2,TF2>& rhs );

template< typename T1, bool TF1, typename T2, bool TF2 >
inline bool operator!=( const SparseVector<T1,TF1>& lhs, const DenseVector<T2,TF2>& rhs );

template< typename T1, typename T2, bool TF >
inline typename EnableIf< IsNumeric<T2>, bool >::Type
   operator!=( const DenseVector<T1,TF>& vec, T2 scalar );

template< typename T1, typename T2, bool TF >
inline typename EnableIf< IsNumeric<T1>, bool >::Type
   operator!=( T1 scalar, const DenseVector<T2,TF>& vec );
//@}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Equality operator for the comparison of two dense vectors.
// \ingroup dense_vector
//
// \param lhs The left-hand side dense vector for the comparison.
// \param rhs The right-hand side dense vector for the comparison.
// \return \a true if the two vectors are equal, \a false if not.
*/
template< typename T1  // Type of the left-hand side dense vector
        , bool TF1     // Transpose flag of the left-hand side dense vector
        , typename T2  // Type of the right-hand side dense vector
        , bool TF2 >   // Transpose flag of the right-hand side dense vector
inline bool operator==( const DenseVector<T1,TF1>& lhs, const DenseVector<T2,TF2>& rhs )
{
   typedef typename T1::CompositeType  CT1;
   typedef typename T2::CompositeType  CT2;

   // Early exit in case the vector sizes don't match
   if( (~lhs).size() != (~rhs).size() ) return false;

   // Evaluation of the two dense vector operands
   CT1 a( ~lhs );
   CT2 b( ~rhs );

   // In order to compare the two vectors, the data values of the lower-order data
   // type are converted to the higher-order data type within the equal function.
   for( size_t i=0; i<a.size(); ++i )
      if( !equal( a[i], b[i] ) ) return false;
   return true;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Equality operator for the comparison of a dense vector and a sparse vector.
// \ingroup dense_vector
//
// \param lhs The left-hand side dense vector for the comparison.
// \param rhs The right-hand side sparse vector for the comparison.
// \return \a true if the two vectors are equal, \a false if not.
*/
template< typename T1  // Type of the left-hand side dense vector
        , bool TF1     // Transpose flag of the left-hand side dense vector
        , typename T2  // Type of the right-hand side sparse vector
        , bool TF2 >   // Transpose flag of the right-hand side sparse vector
inline bool operator==( const DenseVector<T1,TF1>& lhs, const SparseVector<T2,TF2>& rhs )
{
   typedef typename T1::CompositeType  CT1;
   typedef typename T2::CompositeType  CT2;
   typedef typename RemoveReference<CT2>::Type::ConstIterator  ConstIterator;

   // Early exit in case the vector sizes don't match
   if( (~lhs).size() != (~rhs).size() ) return false;

   // Evaluation of the dense vector and sparse vector operand
   CT1 a( ~lhs );
   CT2 b( ~rhs );

   // In order to compare the two vectors, the data values of the lower-order data
   // type are converted to the higher-order data type within the equal function.
   size_t i( 0 );

   for( ConstIterator element=b.begin(); element!=b.end(); ++element, ++i ) {
      for( ; i<element->index(); ++i ) {
         if( !isDefault( a[i] ) ) return false;
      }
      if( !equal( element->value(), a[i] ) ) return false;
   }
   for( ; i<a.size(); ++i ) {
      if( !isDefault( a[i] ) ) return false;
   }

   return true;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Equality operator for the comparison of a sparse vector and a dense vector.
// \ingroup dense_vector
//
// \param lhs The left-hand side sparse vector for the comparison.
// \param rhs The right-hand side dense vector for the comparison.
// \return \a true if the two vectors are equal, \a false if not.
*/
template< typename T1  // Type of the left-hand side sparse vector
        , bool TF1     // Transpose flag of the left-hand side sparse vector
        , typename T2  // Type of the right-hand side dense vector
        , bool TF2 >   // Transpose flag of the right-hand side dense vector
inline bool operator==( const SparseVector<T1,TF1>& lhs, const DenseVector<T2,TF2>& rhs )
{
   return ( rhs == lhs );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Equality operator for the comparison of a dense vector and a scalar value.
// \ingroup dense_vector
//
// \param vec The left-hand side dense vector for the comparison.
// \param scalar The right-hand side scalar value for the comparison.
// \return \a true if all elements of the vector are equal to the scalar, \a false if not.
//
// If all values of the vector are equal to the scalar value, the equality test returns \a true,
// otherwise \a false. Note that this function can only be used with built-in, numerical data
// types!
*/
template< typename T1  // Type of the left-hand side dense vector
        , typename T2  // Type of the right-hand side scalar
        , bool TF >    // Transpose flag
inline typename EnableIf< IsNumeric<T2>, bool >::Type
   operator==( const DenseVector<T1,TF>& vec, T2 scalar )
{
   typedef typename T1::CompositeType  CT1;

   // Evaluation of the dense vector operand
   CT1 a( ~vec );

   // In order to compare the vector and the scalar value, the data values of the lower-order
   // data type are converted to the higher-order data type within the equal function.
   for( size_t i=0; i<a.size(); ++i )
      if( !equal( a[i], scalar ) ) return false;
   return true;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Equality operator for the comparison of a scalar value and a dense vector.
// \ingroup dense_vector
//
// \param scalar The left-hand side scalar value for the comparison.
// \param vec The right-hand side dense vector for the comparison.
// \return \a true if all elements of the vector are equal to the scalar, \a false if not.
//
// If all values of the vector are equal to the scalar value, the equality test returns \a true,
// otherwise \a false. Note that this function can only be used with built-in, numerical data
// types!
*/
template< typename T1  // Type of the left-hand side scalar
        , typename T2  // Type of the right-hand side dense vector
        , bool TF >    // Transpose flag
inline typename EnableIf< IsNumeric<T1>, bool >::Type
   operator==( T1 scalar, const DenseVector<T2,TF>& vec )
{
   return ( vec == scalar );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Inequality operator for the comparison of two dense vectors.
// \ingroup dense_vector
//
// \param lhs The left-hand side dense vector for the comparison.
// \param rhs The right-hand side dense vector for the comparison.
// \return \a true if the two vectors are not equal, \a false if they are equal.
*/
template< typename T1  // Type of the left-hand side dense vector
        , bool TF1     // Transpose flag of the left-hand side dense vector
        , typename T2  // Type of the right-hand side dense vector
        , bool TF2 >   // Transpose flag of the right-hand side dense vector
inline bool operator!=( const DenseVector<T1,TF1>& lhs, const DenseVector<T2,TF2>& rhs )
{
   return !( lhs == rhs );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Inequality operator for the comparison of a dense vector and a sparse vector.
// \ingroup dense_vector
//
// \param lhs The left-hand side dense vector for the comparison.
// \param rhs The right-hand side sparse vector for the comparison.
// \return \a true if the two vectors are not equal, \a false if they are equal.
*/
template< typename T1  // Type of the left-hand side dense vector
        , bool TF1     // Transpose flag of the left-hand side dense vector
        , typename T2  // Type of the right-hand side sparse vector
        , bool TF2 >   // Transpose flag of the right-hand side sparse vector
inline bool operator!=( const DenseVector<T1,TF1>& lhs, const SparseVector<T2,TF2>& rhs )
{
   return !( lhs == rhs );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Inequality operator for the comparison of a sparse vector and a dense vector.
// \ingroup dense_vector
//
// \param lhs The left-hand side sparse vector for the comparison.
// \param rhs The right-hand side dense vector for the comparison.
// \return \a true if the two vectors are not equal, \a false if they are equal.
*/
template< typename T1  // Type of the left-hand side sparse vector
        , bool TF1     // Transpose flag of the left-hand side sparse vector
        , typename T2  // Type of the right-hand side dense vector
        , bool TF2 >   // Transpose flag of the right-hand side dense vector
inline bool operator!=( const SparseVector<T1,TF1>& lhs, const DenseVector<T2,TF2>& rhs )
{
   return !( rhs == lhs );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Inequality operator for the comparison of a dense vector and a scalar value.
// \ingroup dense_vector
//
// \param vec The left-hand side dense vector for the comparison.
// \param scalar The right-hand side scalar value for the comparison.
// \return \a true if at least one element of the vector is different from the scalar, \a false if not.
//
// If one value of the vector is inequal to the scalar value, the inequality test returns \a true,
// otherwise \a false. Note that this function can only be used with built-in, numerical data
// types!
*/
template< typename T1  // Type of the left-hand side dense vector
        , typename T2  // Type of the right-hand side scalar
        , bool TF >    // Transpose flag
inline typename EnableIf< IsNumeric<T2>, bool >::Type
   operator!=( const DenseVector<T1,TF>& vec, T2 scalar )
{
   return !( vec == scalar );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Inequality operator for the comparison of a scalar value and a dense vector.
// \ingroup dense_vector
//
// \param scalar The left-hand side scalar value for the comparison.
// \param vec The right-hand side dense vector for the comparison.
// \return \a true if at least one element of the vector is different from the scalar, \a false if not.
//
// If one value of the vector is inequal to the scalar value, the inequality test returns \a true,
// otherwise \a false. Note that this function can only be used with built-in, numerical data
// types!
*/
template< typename T1  // Type of the left-hand side scalar
        , typename T2  // Type of the right-hand side vector
        , bool TF >    // Transpose flag
inline typename EnableIf< IsNumeric<T1>, bool >::Type
   operator!=( T1 scalar, const DenseVector<T2,TF>& vec )
{
   return !( vec == scalar );
}
//*************************************************************************************************




//=================================================================================================
//
//  GLOBAL FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\name DenseVector functions */
//@{
template< typename VT, bool TF >
bool isnan( const DenseVector<VT,TF>& dv );

template< typename VT, bool TF >
typename CMathTrait<typename VT::ElementType>::Type length( const DenseVector<VT,TF>& dv );

template< typename VT, bool TF >
const typename VT::ElementType sqrLength( const DenseVector<VT,TF>& dv );

template< typename VT, bool TF >
const typename VT::ElementType min( const DenseVector<VT,TF>& dv );

template< typename VT, bool TF >
const typename VT::ElementType max( const DenseVector<VT,TF>& dv );
//@}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Checks the given dense vector for not-a-number elements.
// \ingroup dense_vector
//
// \param dv The vector to be checked for not-a-number elements.
// \return \a true if at least one element of the vector is not-a-number, \a false otherwise.
//
// This function checks the N-dimensional dense vector for not-a-number (NaN) elements. If at
// least one element of the vector is not-a-number, the function returns \a true, otherwise it
// returns \a false.

   \code
   blaze::DynamicVector<double> a;
   // ... Resizing and initialization
   if( isnan( a ) ) { ... }
   \endcode

// Note that this function only works for vectors with floating point elements. The attempt to
// use it for a vector with a non-floating point element type results in a compile time error.
*/
template< typename VT  // Type of the dense vector
        , bool TF >    // Transpose flag
bool isnan( const DenseVector<VT,TF>& dv )
{
   typedef typename VT::CompositeType  CT;

   CT a( ~dv );  // Evaluation of the dense vector operand

   for( size_t i=0UL; i<a.size(); ++i ) {
      if( isnan( a[i] ) ) return true;
   }
   return false;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Calculation of the dense vector length \f$|\vec{a}|\f$.
// \ingroup dense_vector
//
// \param dv The given dense vector.
// \return The length of the dense vector.
//
// This function calculates the actual length of the dense vector. The return type of the length()
// function depends on the actual element type of the vector instance:
//
// <table border="0" cellspacing="0" cellpadding="1">
//    <tr>
//       <td width="250px"> \b ElementType </td>
//       <td width="100px"> \b LengthType </td>
//    </tr>
//    <tr>
//       <td>float</td>
//       <td>float</td>
//    </tr>
//    <tr>
//       <td>integral data types and double</td>
//       <td>double</td>
//    </tr>
//    <tr>
//       <td>long double</td>
//       <td>long double</td>
//    </tr>
// </table>
//
// \b Note: This operation is only defined for numeric data types. In case the element type is
// not a numeric data type (i.e. a user defined data type or boolean) the attempt to use the
// length() function results in a compile time error!
*/
template< typename VT  // Type of the dense vector
        , bool TF >    // Transpose flag
typename CMathTrait<typename VT::ElementType>::Type length( const DenseVector<VT,TF>& dv )
{
   typedef typename VT::ElementType                ElementType;
   typedef typename CMathTrait<ElementType>::Type  LengthType;

   BLAZE_CONSTRAINT_MUST_BE_NUMERIC_TYPE( ElementType );

   LengthType sum( 0 );
   for( size_t i=0UL; i<(~dv).size(); ++i )
      sum += sq( (~dv)[i] );
   return std::sqrt( sum );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Calculation of the dense vector square length \f$|\vec{a}|^2\f$.
// \ingroup dense_vector
//
// \param dv The given dense vector.
// \return The square length of the dense vector.
//
// This function calculates the actual square length of the dense vector.
//
// \b Note: This operation is only defined for numeric data types. In case the element type is
// not a numeric data type (i.e. a user defined data type or boolean) the attempt to use the
// sqrLength() function results in a compile time error!
*/
template< typename VT  // Type of the dense vector
        , bool TF >    // Transpose flag
const typename VT::ElementType sqrLength( const DenseVector<VT,TF>& dv )
{
   typedef typename VT::ElementType  ElementType;

   BLAZE_CONSTRAINT_MUST_BE_NUMERIC_TYPE( ElementType );

   ElementType sum( 0 );
   for( size_t i=0UL; i<(~dv).size(); ++i )
      sum += sq( (~dv)[i] );
   return sum;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the smallest element of the dense vector.
// \ingroup dense_vector
//
// \param dv The given dense vector.
// \return The smallest dense vector element.
//
// This function returns the smallest element of the given dense vector. This function can
// only be used for element types that support the smaller-than relationship. In case the
// vector currently has a size of 0, the returned value is the default value (e.g. 0 in case
// of fundamental data types).
*/
template< typename VT  // Type of the dense vector
        , bool TF >    // Transpose flag
const typename VT::ElementType min( const DenseVector<VT,TF>& dv )
{
   using blaze::min;

   typedef typename VT::ElementType    ET;
   typedef typename VT::CompositeType  CT;

   CT a( ~dv );  // Evaluation of the dense vector operand

   if( a.size() == 0UL ) return ET();

   ET minimum( a[0] );
   for( size_t i=1UL; i<a.size(); ++i )
      minimum = min( minimum, a[i] );
   return minimum;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the largest element of the dense vector.
// \ingroup dense_vector
//
// \param dv The given dense vector.
// \return The largest dense vector element.
//
// This function returns the largest element of the given dense vector. This function can
// only be used for element types that support the smaller-than relationship. In case the
// vector currently has a size of 0, the returned value is the default value (e.g. 0 in case
// of fundamental data types).
*/
template< typename VT  // Type of the dense vector
        , bool TF >    // Transpose flag
const typename VT::ElementType max( const DenseVector<VT,TF>& dv )
{
   using blaze::max;

   typedef typename VT::ElementType    ET;
   typedef typename VT::CompositeType  CT;

   CT a( ~dv );  // Evaluation of the dense vector operand

   if( a.size() == 0UL ) return ET();

   ET maximum( a[0] );
   for( size_t i=1UL; i<a.size(); ++i )
      maximum = max( maximum, a[i] );
   return maximum;
}
//*************************************************************************************************

} // namespace blaze

#endif
