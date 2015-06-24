//=================================================================================================
/*!
//  \file blaze/math/traits/SubmatrixExprTrait.h
//  \brief Header file for the SubmatrixExprTrait class template
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

#ifndef _BLAZE_MATH_TRAITS_SUBMATRIXEXPRTRAIT_H_
#define _BLAZE_MATH_TRAITS_SUBMATRIXEXPRTRAIT_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <blaze/math/typetraits/IsColumnMajorMatrix.h>
#include <blaze/math/typetraits/IsComputation.h>
#include <blaze/math/typetraits/IsDenseMatrix.h>
#include <blaze/math/typetraits/IsSparseMatrix.h>
#include <blaze/math/typetraits/IsTransExpr.h>
#include <blaze/math/views/Forward.h>
#include <blaze/util/InvalidType.h>
#include <blaze/util/mpl/If.h>
#include <blaze/util/mpl/Or.h>
#include <blaze/util/typetraits/IsConst.h>
#include <blaze/util/typetraits/IsReference.h>
#include <blaze/util/typetraits/IsVolatile.h>
#include <blaze/util/typetraits/RemoveCV.h>
#include <blaze/util/typetraits/RemoveReference.h>


namespace blaze {

//=================================================================================================
//
//  CLASS DEFINITION
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Evaluation of the expression type type of a submatrix operation.
// \ingroup math_traits
//
// Via this type trait it is possible to evaluate the return type of a submatrix operation.
// Given the dense or sparse matrix type \a MT and the alignment flag \a AF, the nested type
// \a Type corresponds to the resulting return type. In case the given type is neither a
// dense nor a sparse matrix type, the resulting data type \a Type is set to \a INVALID_TYPE.
*/
template< typename MT  // Type of the matrix operand
        , bool AF >    // Alignment flag
struct SubmatrixExprTrait
{
 private:
   //**struct Failure******************************************************************************
   /*! \cond BLAZE_INTERNAL */
   struct Failure { typedef INVALID_TYPE  Type; };
   /*! \endcond */
   //**********************************************************************************************

   //**struct DenseResult**************************************************************************
   /*! \cond BLAZE_INTERNAL */
   template< typename T >
   struct DenseResult { typedef DenseSubmatrix<T,AF,IsColumnMajorMatrix<T>::value>  Type; };
   /*! \endcond */
   //**********************************************************************************************

   //**struct SparseResult*************************************************************************
   /*! \cond BLAZE_INTERNAL */
   template< typename T >
   struct SparseResult { typedef SparseSubmatrix<T,AF,IsColumnMajorMatrix<T>::value>  Type; };
   /*! \endcond */
   //**********************************************************************************************

   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   typedef typename RemoveReference<MT>::Type  Tmp;
   /*! \endcond */
   //**********************************************************************************************

 public:
   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   typedef typename If< Or< IsComputation<Tmp>, IsTransExpr<Tmp> >
                      , typename If< Or< IsConst<Tmp>, IsVolatile<Tmp> >
                                   , SubmatrixExprTrait< typename RemoveCV<Tmp>::Type, AF >
                                   , Failure
                                   >::Type
                      , typename If< IsDenseMatrix<Tmp>
                                   , DenseResult<Tmp>
                                   , typename If< IsSparseMatrix<Tmp>
                                                , SparseResult<Tmp>
                                                , Failure
                                                >::Type
                                   >::Type
                      >::Type::Type  Type;
   /*! \endcond */
   //**********************************************************************************************
};
//*************************************************************************************************

} // namespace blaze

#endif
