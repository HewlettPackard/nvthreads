//=================================================================================================
/*!
//  \file blaze/math/typetraits/IsView.h
//  \brief Header file for the IsView type trait
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

#ifndef _BLAZE_MATH_TYPETRAITS_ISVIEW_H_
#define _BLAZE_MATH_TYPETRAITS_ISVIEW_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <boost/type_traits/is_base_of.hpp>
#include <blaze/math/expressions/View.h>
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
/*!\brief Auxiliary helper struct for the IsView type trait.
// \ingroup math_type_traits
*/
template< typename T >
struct IsViewHelper
{
 private:
   //**********************************************************************************************
   typedef typename RemoveCV<T>::Type  T2;
   //**********************************************************************************************

 public:
   //**********************************************************************************************
   enum { value = boost::is_base_of<View,T2>::value && !boost::is_base_of<T2,View>::value };
   typedef typename SelectType<value,TrueType,FalseType>::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Compile time check for views.
// \ingroup math_type_traits
//
// This type trait tests whether or not the given template parameter is a view (i.e. subvector,
// submatrix, row, column, ...). In case the type is a view, the \a value member enumeration is
// set to 1, the nested type definition \a Type is \a TrueType, and the class derives from
// \a TrueType. Otherwise \a value is set to 0, \a Type is \a FalseType, and the class derives
// from \a FalseType.

   \code
   using blaze::columnVector;

   typedef blaze::DynamicVector<double,columnVector>  VectorType;
   typedef blaze::DenseSubvector<VectorType>          SubvectorType;

   typedef blaze::CompressedMatrix<int,rowMajor>  MatrixType;
   typedef blaze::Row<MatrixType>                 RowType;
   typedef blaze::Column<MatrixType>              ColumnType;

   blaze::IsView< SubvectorType >::value    // Evaluates to 1
   blaze::IsView< const RowType >::Type     // Results in TrueType
   blaze::IsView< volatile ColumnType >     // Is derived from TrueType
   blaze::IsView< float >::value            // Evaluates to 0
   blaze::IsView< const VectorType >::Type  // Results in FalseType
   blaze::IsView< volatile MatrixType >     // Is derived from FalseType
   \endcode
*/
template< typename T >
struct IsView : public IsViewHelper<T>::Type
{
 public:
   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   enum { value = IsViewHelper<T>::value };
   typedef typename IsViewHelper<T>::Type  Type;
   /*! \endcond */
   //**********************************************************************************************
};
//*************************************************************************************************

} // namespace blaze

#endif
