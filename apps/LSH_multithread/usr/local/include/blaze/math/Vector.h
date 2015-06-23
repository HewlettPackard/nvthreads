//=================================================================================================
/*!
//  \file blaze/math/Vector.h
//  \brief Header file for all basic Vector functionality
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

#ifndef _BLAZE_MATH_VECTOR_H_
#define _BLAZE_MATH_VECTOR_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <iomanip>
#include <ostream>
#include <blaze/math/expressions/Vector.h>
#include <blaze/math/traits/MultTrait.h>
#include <blaze/math/TransposeFlag.h>


namespace blaze {

//=================================================================================================
//
//  GLOBAL OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*!\name Vector operators */
//@{
template< typename T1, typename T2 >
inline const typename MultTrait<typename T1::ElementType,typename T2::ElementType>::Type
   operator,( const Vector<T1,false>& lhs, const Vector<T2,false>& rhs );

template< typename T1, typename T2 >
inline const typename MultTrait<typename T1::ElementType,typename T2::ElementType>::Type
   operator,( const Vector<T1,false>& lhs, const Vector<T2,true>& rhs );

template< typename T1, typename T2 >
inline const typename MultTrait<typename T1::ElementType,typename T2::ElementType>::Type
   operator,( const Vector<T1,true>& lhs, const Vector<T2,false>& rhs );

template< typename T1, typename T2 >
inline const typename MultTrait<typename T1::ElementType,typename T2::ElementType>::Type
   operator,( const Vector<T1,true>& lhs, const Vector<T2,true>& rhs );

template< typename VT, bool TF >
inline std::ostream& operator<<( std::ostream& os, const Vector<VT,TF>& dv );
//@}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Multiplication operator for the scalar product (inner product) of two vectors
//        (\f$ s=(\vec{a},\vec{b}) \f$).
// \ingroup vector
//
// \param lhs The left-hand side vector for the inner product.
// \param rhs The right-hand side vector for the inner product.
// \return The scalar product.
*/
template< typename T1    // Type of the left-hand side vector
        , typename T2 >  // Type of the right-hand side vector
inline const typename MultTrait<typename T1::ElementType,typename T2::ElementType>::Type
   operator,( const Vector<T1,false>& lhs, const Vector<T2,false>& rhs )
{
   return trans(~lhs) * (~rhs);
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Multiplication operator for the scalar product (inner product) of two vectors
//        (\f$ s=(\vec{a},\vec{b}) \f$).
// \ingroup vector
//
// \param lhs The left-hand side vector for the inner product.
// \param rhs The right-hand side vector for the inner product.
// \return The scalar product.
*/
template< typename T1    // Type of the left-hand side vector
        , typename T2 >  // Type of the right-hand side vector
inline const typename MultTrait<typename T1::ElementType,typename T2::ElementType>::Type
   operator,( const Vector<T1,false>& lhs, const Vector<T2,true>& rhs )
{
   return trans(~lhs) * trans(~rhs);
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Multiplication operator for the scalar product (inner product) of two vectors
//        (\f$ s=(\vec{a},\vec{b}) \f$).
// \ingroup vector
//
// \param lhs The left-hand side vector for the inner product.
// \param rhs The right-hand side vector for the inner product.
// \return The scalar product.
*/
template< typename T1    // Type of the left-hand side vector
        , typename T2 >  // Type of the right-hand side vector
inline const typename MultTrait<typename T1::ElementType,typename T2::ElementType>::Type
   operator,( const Vector<T1,true>& lhs, const Vector<T2,false>& rhs )
{
   return (~lhs) * (~rhs);
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Multiplication operator for the scalar product (inner product) of two vectors
//        (\f$ s=(\vec{a},\vec{b}) \f$).
// \ingroup vector
//
// \param lhs The left-hand side vector for the inner product.
// \param rhs The right-hand side vector for the inner product.
// \return The scalar product.
*/
template< typename T1    // Type of the left-hand side vector
        , typename T2 >  // Type of the right-hand side vector
inline const typename MultTrait<typename T1::ElementType,typename T2::ElementType>::Type
   operator,( const Vector<T1,true>& lhs, const Vector<T2,true>& rhs )
{
   return (~lhs) * trans(~rhs);
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Global output operator for dense and sparse vectors.
// \ingroup vector
//
// \param os Reference to the output stream.
// \param v Reference to a constant vector object.
// \return Reference to the output stream.
*/
template< typename VT  // Type of the vector
        , bool TF >    // Transpose flag
inline std::ostream& operator<<( std::ostream& os, const Vector<VT,TF>& v )
{
   if( (~v).size() == 0UL ) {
      os << "( )\n";
   }
   else if( TF == rowVector ) {
      os << "(";
      for( size_t i=0UL; i<(~v).size(); ++i )
         os << " " << (~v)[i];
      os << " )\n";
   }
   else {
      for( size_t i=0UL; i<(~v).size(); ++i )
         os << "( " << std::setw( 11UL ) << (~v)[i] << " )\n";
   }

   return os;
}
//*************************************************************************************************

} // namespace blaze

#endif
