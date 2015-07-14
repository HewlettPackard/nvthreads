//=================================================================================================
/*!
//  \file blaze/math/typetraits/NumericElementType.h
//  \brief Header file for the NumericElementType type trait
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

#ifndef _BLAZE_MATH_TYPETRAITS_NUMERICELEMENTTYPE_H_
#define _BLAZE_MATH_TYPETRAITS_NUMERICELEMENTTYPE_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <blaze/util/mpl/If.h>
#include <blaze/util/typetraits/IsNumeric.h>


namespace blaze {

//=================================================================================================
//
//  CLASS DEFINITION
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Evaluation of the numeric element type of a given data type.
// \ingroup math_type_traits
//
// Via this type trait it is possible to evaluate the numeric (fundamental or complex) element
// type at the heart of a given data type. Examples:

   \code
   blaze::NumericElementType< double >::Type                              // corresponds to double
   blaze::NumericElementType< complex<float> >::Type                      // corresponds to complex<float>
   blaze::NumericElementType< StaticVector<int,3UL> >::Type               // corresponds to int
   blaze::NumericElementType< CompressedVector< DynamicVector<float> > >  // corresponds to float
   \endcode

// Note that per default NumericElementType only supports fundamental/built-in data types,
// complex, and data types with the nested type definition \a ElementType. Support for other
// data types can be added by specializing the NumericElementType class template.
*/
template< typename T >
struct NumericElementType
{
 private:
   //**struct Numeric******************************************************************************
   /*! \cond BLAZE_INTERNAL */
   template< typename T2 >
   struct Numeric { typedef T2  Type; };
   /*! \endcond */
   //**********************************************************************************************

   //**struct Other********************************************************************************
   /*! \cond BLAZE_INTERNAL */
   template< typename T2 >
   struct Other { typedef typename NumericElementType<typename T2::ElementType>::Type  Type; };
   /*! \endcond */
   //**********************************************************************************************

 public:
   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   typedef typename If< IsNumeric<T>, Numeric<T>, Other<T> >::Type::Type  Type;
   /*! \endcond */
   //**********************************************************************************************
};
//*************************************************************************************************

} // namespace blaze

#endif