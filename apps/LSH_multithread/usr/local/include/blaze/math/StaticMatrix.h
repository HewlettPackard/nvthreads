//=================================================================================================
/*!
//  \file blaze/math/StaticMatrix.h
//  \brief Header file for the complete StaticMatrix implementation
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

#ifndef _BLAZE_MATH_STATICMATRIX_H_
#define _BLAZE_MATH_STATICMATRIX_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <blaze/math/dense/StaticMatrix.h>
#include <blaze/math/DenseMatrix.h>
#include <blaze/math/HybridMatrix.h>
#include <blaze/math/StaticVector.h>
#include <blaze/system/Precision.h>
#include <blaze/util/Random.h>


namespace blaze {

//=================================================================================================
//
//  RAND SPECIALIZATION
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Specialization of the Rand class template for StaticMatrix.
// \ingroup random
//
// This specialization of the Rand class creates random instances of StaticMatrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
class Rand< StaticMatrix<Type,M,N,SO> >
{
 public:
   //**Generate functions**************************************************************************
   /*!\name Generate functions */
   //@{
   inline const StaticMatrix<Type,M,N,SO> generate() const;

   template< typename Arg >
   inline const StaticMatrix<Type,M,N,SO> generate( const Arg& min, const Arg& max ) const;
   //@}
   //**********************************************************************************************

   //**Randomize functions*************************************************************************
   /*!\name Randomize functions */
   //@{
   inline void randomize( StaticMatrix<Type,M,N,SO>& matrix ) const;

   template< typename Arg >
   inline void randomize( StaticMatrix<Type,M,N,SO>& matrix, const Arg& min, const Arg& max ) const;
   //@}
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Generation of a random StaticMatrix.
//
// \return The generated random matrix.
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline const StaticMatrix<Type,M,N,SO> Rand< StaticMatrix<Type,M,N,SO> >::generate() const
{
   StaticMatrix<Type,M,N,SO> matrix;
   randomize( matrix );
   return matrix;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Generation of a random StaticMatrix.
//
// \param min The smallest possible value for a matrix element.
// \param max The largest possible value for a matrix element.
// \return The generated random matrix.
*/
template< typename Type   // Data type of the matrix
        , size_t M        // Number of rows
        , size_t N        // Number of columns
        , bool SO >       // Storage order
template< typename Arg >  // Min/max argument type
inline const StaticMatrix<Type,M,N,SO>
   Rand< StaticMatrix<Type,M,N,SO> >::generate( const Arg& min, const Arg& max ) const
{
   StaticMatrix<Type,M,N,SO> matrix;
   randomize( matrix, min, max );
   return matrix;
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Randomization of a StaticMatrix.
//
// \param matrix The matrix to be randomized.
// \return void
*/
template< typename Type  // Data type of the matrix
        , size_t M       // Number of rows
        , size_t N       // Number of columns
        , bool SO >      // Storage order
inline void Rand< StaticMatrix<Type,M,N,SO> >::randomize( StaticMatrix<Type,M,N,SO>& matrix ) const
{
   using blaze::randomize;

   for( size_t i=0UL; i<M; ++i ) {
      for( size_t j=0UL; j<N; ++j ) {
         randomize( matrix(i,j) );
      }
   }
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Randomization of a StaticMatrix.
//
// \param matrix The matrix to be randomized.
// \param min The smallest possible value for a matrix element.
// \param max The largest possible value for a matrix element.
// \return void
*/
template< typename Type   // Data type of the matrix
        , size_t M        // Number of rows
        , size_t N        // Number of columns
        , bool SO >       // Storage order
template< typename Arg >  // Min/max argument type
inline void Rand< StaticMatrix<Type,M,N,SO> >::randomize( StaticMatrix<Type,M,N,SO>& matrix,
                                                          const Arg& min, const Arg& max ) const
{
   using blaze::randomize;

   for( size_t i=0UL; i<M; ++i ) {
      for( size_t j=0UL; j<N; ++j ) {
         randomize( matrix(i,j), min, max );
      }
   }
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  TYPE DEFINITIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief 2x2 single precision matrix.
// \ingroup static_matrix_2x2
*/
typedef StaticMatrix<float,2UL,2UL,false>  Mat2x2f;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 2x2 double precision matrix.
// \ingroup static_matrix_2x2
*/
typedef StaticMatrix<double,2UL,2UL,false>  Mat2x2d;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 2x2 matrix with system-specific precision.
// \ingroup static_matrix_2x2
*/
typedef StaticMatrix<real,2UL,2UL,false>  Mat2x2;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 3x3 single precision matrix.
// \ingroup static_matrix_3x3
*/
typedef StaticMatrix<float,3UL,3UL,false>  Mat3x3f;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 3x3 double precision matrix.
// \ingroup static_matrix_3x3
*/
typedef StaticMatrix<double,3UL,3UL,false>  Mat3x3d;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 3x3 matrix with system-specific precision.
// \ingroup static_matrix_3x3
*/
typedef StaticMatrix<real,3UL,3UL,false>  Mat3x3;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 4x4 single precision matrix.
// \ingroup static_matrix_4x4
*/
typedef StaticMatrix<float,4UL,4UL,false>  Mat4x4f;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 4x4 double precision matrix.
// \ingroup static_matrix_4x4
*/
typedef StaticMatrix<double,4UL,4UL,false>  Mat4x4d;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 4x4 matrix with system-specific precision.
// \ingroup static_matrix_4x4
*/
typedef StaticMatrix<real,4UL,4UL,false>  Mat4x4;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 5x5 single precision matrix.
// \ingroup static_matrix_5x5
*/
typedef StaticMatrix<float,5UL,5UL,false>  Mat5x5f;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 5x5 double precision matrix.
// \ingroup static_matrix_5x5
*/
typedef StaticMatrix<double,5UL,5UL,false>  Mat5x5d;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 5x5 matrix with system-specific precision.
// \ingroup static_matrix_5x5
*/
typedef StaticMatrix<real,5UL,5UL,false>  Mat5x5;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 6x6 single precision matrix.
// \ingroup static_matrix_6x6
*/
typedef StaticMatrix<float,6UL,6UL,false>  Mat6x6f;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 6x6 double precision matrix.
// \ingroup static_matrix_6x6
*/
typedef StaticMatrix<double,6UL,6UL,false>  Mat6x6d;
//*************************************************************************************************


//*************************************************************************************************
/*!\brief 6x6 matrix with system-specific precision.
// \ingroup static_matrix_6x6
*/
typedef StaticMatrix<real,6UL,6UL,false>  Mat6x6;
//*************************************************************************************************

} // namespace blaze

#endif
