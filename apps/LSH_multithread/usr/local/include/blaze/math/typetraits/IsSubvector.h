//=================================================================================================
/*!
//  \file blaze/math/typetraits/IsSubvector.h
//  \brief Header file for the IsSubvector type trait
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

#ifndef _BLAZE_MATH_TYPETRAITS_ISSUBVECTOR_H_
#define _BLAZE_MATH_TYPETRAITS_ISSUBVECTOR_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <boost/type_traits/is_base_of.hpp>
#include <blaze/math/expressions/Subvector.h>
#include <blaze/util/FalseType.h>
#include <blaze/util/SelectType.h>
#include <blaze/util/TrueType.h>
#include <blaze/util/typetraits/RemoveCV.h>


namespace blaze {

//=================================================================================================
//
//  CLASS DEFINITION
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Auxiliary helper struct for the IsSubvector type trait.
// \ingroup math_type_traits
*/
template< typename T >
struct IsSubvectorHelper
{
 private:
   //**********************************************************************************************
   typedef typename RemoveCV<T>::Type  T2;
   //**********************************************************************************************

 public:
   //**********************************************************************************************
   enum { value = boost::is_base_of<Subvector,T2>::value && !boost::is_base_of<T2,Subvector>::value };
   typedef typename SelectType<value,TrueType,FalseType>::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Compile time check for subvectors.
// \ingroup math_type_traits
//
// This type trait tests whether or not the given template parameter is a subvector (i.e. dense or
// sparse subvector). In case the type is a subvector, the \a value member enumeration is set to
// 1, the nested type definition \a Type is \a TrueType, and the class derives from \a TrueType.
// Otherwise \a value is set to 0, \a Type is \a FalseType, and the class derives from \a FalseType.

   \code
   typedef blaze::DynamicVector<double,columnVector>  DenseVectorType1;
   typedef blaze::DenseSubvector<DenseVectorType1>    DenseSubvectorType1;

   typedef blaze::StaticVector<float,3UL,rowVector>   DenseVectorType2;
   typedef blaze::DenseSubvector<DenseVectorType2>    DenseSubvectorType2;

   typedef blaze::CompressedVector<int,columnVector>  SparseVectorType;
   typedef blaze::SparseSubvector<SparseVectorType>   SparseSubvectorType;

   blaze::IsSubvector< SparseSubvectorType >::value       // Evaluates to 1
   blaze::IsSubvector< const DenseSubvectorType1 >::Type  // Results in TrueType
   blaze::IsSubvector< volatile DenseSubvectorType2 >     // Is derived from TrueType
   blaze::IsSubvector< DenseVectorType1 >::value          // Evaluates to 0
   blaze::IsSubvector< const SparseVectorType >::Type     // Results in FalseType
   blaze::IsSubvector< volatile long double >             // Is derived from FalseType
   \endcode
*/
template< typename T >
struct IsSubvector : public IsSubvectorHelper<T>::Type
{
 public:
   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   enum { value = IsSubvectorHelper<T>::value };
   typedef typename IsSubvectorHelper<T>::Type  Type;
   /*! \endcond */
   //**********************************************************************************************
};
//*************************************************************************************************

} // namespace blaze

#endif
