//=================================================================================================
/*!
//  \file blaze/math/expressions/DVecTransposer.h
//  \brief Header file for the dense vector transposer
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

#ifndef _BLAZE_MATH_EXPRESSIONS_DVECTRANSPOSER_H_
#define _BLAZE_MATH_EXPRESSIONS_DVECTRANSPOSER_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <blaze/math/constraints/Computation.h>
#include <blaze/math/constraints/DenseVector.h>
#include <blaze/math/constraints/TransposeFlag.h>
#include <blaze/math/expressions/DenseVector.h>
#include <blaze/math/intrinsics/IntrinsicTrait.h>
#include <blaze/math/traits/SubvectorTrait.h>
#include <blaze/util/Assert.h>
#include <blaze/util/EnableIf.h>
#include <blaze/util/Types.h>
#include <blaze/util/typetraits/IsNumeric.h>


namespace blaze {

//=================================================================================================
//
//  CLASS DVECTRANSPOSER
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Expression object for the transposition of a dense vector.
// \ingroup dense_vector_expression
//
// The DVecTransposer class is a wrapper object for the temporary transposition of a dense vector.
*/
template< typename VT  // Type of the dense vector
        , bool TF >    // Transpose flag
class DVecTransposer : public DenseVector< DVecTransposer<VT,TF>, TF >
{
 private:
   //**Type definitions****************************************************************************
   typedef IntrinsicTrait<typename VT::ElementType>  IT;  //!< Intrinsic trait for the vector element type.
   //**********************************************************************************************

 public:
   //**Type definitions****************************************************************************
   typedef DVecTransposer<VT,TF>        This;            //!< Type of this DVecTransposer instance.
   typedef typename VT::TransposeType   ResultType;      //!< Result type for expression template evaluations.
   typedef typename VT::ResultType      TransposeType;   //!< Transpose type for expression template evaluations.
   typedef typename VT::ElementType     ElementType;     //!< Type of the vector elements.
   typedef typename IT::Type            IntrinsicType;   //!< Intrinsic type of the vector elements.
   typedef typename VT::ReturnType      ReturnType;      //!< Return type for expression template evaluations.
   typedef const This&                  CompositeType;   //!< Data type for composite expression templates.
   typedef typename VT::Reference       Reference;       //!< Reference to a non-constant matrix value.
   typedef typename VT::ConstReference  ConstReference;  //!< Reference to a constant matrix value.
   typedef typename VT::Iterator        Iterator;        //!< Iterator over non-constant elements.
   typedef typename VT::ConstIterator   ConstIterator;   //!< Iterator over constant elements.
   //**********************************************************************************************

   //**Compilation flags***************************************************************************
   //! Compilation flag for intrinsic optimization.
   /*! The \a vectorizable compilation flag indicates whether expressions the vector is involved
       in can be optimized via intrinsics. In case the dense vector operand is vectorizable, the
       \a vectorizable compilation flag is set to \a true, otherwise it is set to \a false. */
   enum { vectorizable = VT::vectorizable };

   //! Compilation flag for SMP assignments.
   /*! The \a smpAssignable compilation flag indicates whether the vector can be used in SMP
       (shared memory parallel) assignments (both on the left-hand and right-hand side of the
       assignment). */
   enum { smpAssignable = VT::smpAssignable };
   //**********************************************************************************************

   //**Constructor*********************************************************************************
   /*!\brief Constructor for the DVecTransposer class.
   //
   // \param dv The dense vector operand.
   */
   explicit inline DVecTransposer( VT& dv )
      : dv_( dv )  // The dense vector operand
   {}
   //**********************************************************************************************

   //**Subscript operator**************************************************************************
   /*!\brief Subscript operator for the direct access to the vector elements.
   //
   // \param index Access index. The index has to be in the range \f$[0..N-1]\f$.
   // \return Reference to the accessed value.
   */
   inline Reference operator[]( size_t index ) {
      BLAZE_USER_ASSERT( index < dv_.size(), "Invalid vector access index" );
      return dv_[index];
   }
   //**********************************************************************************************

   //**Subscript operator**************************************************************************
   /*!\brief Subscript operator for the direct access to the vector elements.
   //
   // \param index Access index. The index has to be in the range \f$[0..N-1]\f$.
   // \return Reference to the accessed value.
   */
   inline ConstReference operator[]( size_t index ) const {
      BLAZE_USER_ASSERT( index < dv_.size(), "Invalid vector access index" );
      return dv_[index];
   }
   //**********************************************************************************************

   //**Low-level data access***********************************************************************
   /*!\brief Low-level data access to the vector elements.
   //
   // \return Pointer to the internal element storage.
   */
   inline ElementType* data() {
      return dv_.data();
   }
   //**********************************************************************************************

   //**Begin function******************************************************************************
   /*!\brief Returns an iterator to the first element of the dense vector.
   //
   // \return Iterator to the first element of the dense vector.
   */
   inline ConstIterator begin() const {
      return dv_.cbegin();
   }
   //**********************************************************************************************

   //**Cbegin function*****************************************************************************
   /*!\brief Returns an iterator to the first element of the dense vector.
   //
   // \return Iterator to the first element of the dense vector.
   */
   inline ConstIterator cbegin() const {
      return dv_.cbegin();
   }
   //**********************************************************************************************

   //**End function********************************************************************************
   /*!\brief Returns an iterator just past the last element of the dense vector.
   //
   // \return Iterator just past the last element of the dense vector.
   */
   inline ConstIterator end() const {
      return dv_.cend();
   }
   //**********************************************************************************************

   //**Cend function*******************************************************************************
   /*!\brief Returns an iterator just past the last element of the dense vector.
   //
   // \return Iterator just past the last element of the dense vector.
   */
   inline ConstIterator cend() const {
      return dv_.cend();
   }
   //**********************************************************************************************

   //**Multiplication assignment operator**********************************************************
   /*!\brief Multiplication assignment operator for the multiplication between a vector and
   //        a scalar value (\f$ \vec{a}*=s \f$).
   //
   // \param rhs The right-hand side scalar value for the multiplication.
   // \return Reference to this DVecTransposer.
   */
   template< typename Other >  // Data type of the right-hand side scalar
   inline typename EnableIf< IsNumeric<Other>, DVecTransposer >::Type& operator*=( Other rhs )
   {
      (~dv_) *= rhs;
      return *this;
   }
   //**********************************************************************************************

   //**Division assignment operator****************************************************************
   /*!\brief Division assignment operator for the division of a vector by a scalar value
   //        (\f$ \vec{a}/=s \f$).
   //
   // \param rhs The right-hand side scalar value for the division.
   // \return Reference to this DVecTransposer.
   //
   // \b Note: A division by zero is only checked by an user assert.
   */
   template< typename Other >  // Data type of the right-hand side scalar
   inline typename EnableIf< IsNumeric<Other>, DVecTransposer >::Type& operator/=( Other rhs )
   {
      BLAZE_USER_ASSERT( rhs != Other(0), "Division by zero detected" );

      (~dv_) /= rhs;
      return *this;
   }
   //**********************************************************************************************

   //**Size function*******************************************************************************
   /*!\brief Returns the current size/dimension of the vector.
   //
   // \return The size of the vector.
   */
   inline size_t size() const {
      return dv_.size();
   }
   //**********************************************************************************************

   //**Reset function******************************************************************************
   /*!\brief Resets the vector elements.
   //
   // \return void
   */
   inline void reset() {
      return dv_.reset();
   }
   //**********************************************************************************************

   //**CanAliased function*************************************************************************
   /*!\brief Returns whether the vector can alias with the given address \a alias.
   //
   // \param alias The alias to be checked.
   // \return \a true in case the alias corresponds to this vector, \a false if not.
   */
   template< typename Other >  // Data type of the foreign expression
   inline bool canAlias( const Other* alias ) const
   {
      return dv_.canAlias( alias );
   }
   //**********************************************************************************************

   //**IsAliased function**************************************************************************
   /*!\brief Returns whether the vector is aliased with the given address \a alias.
   //
   // \param alias The alias to be checked.
   // \return \a true in case the alias corresponds to this vector, \a false if not.
   */
   template< typename Other >  // Data type of the foreign expression
   inline bool isAliased( const Other* alias ) const
   {
      return dv_.isAliased( alias );
   }
   //**********************************************************************************************

   //**IsAligned function**************************************************************************
   /*!\brief Returns whether the vector is properly aligned in memory.
   //
   // \return \a true in case the vector is aligned, \a false if not.
   */
   inline bool isAligned() const
   {
      return dv_.isAligned();
   }
   //**********************************************************************************************

   //**Load function*******************************************************************************
   /*!\brief Aligned load of an intrinsic element of the vector.
   //
   // \param index Access index. The index must be smaller than the number of vector elements.
   // \return The loaded intrinsic element.
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors.
   */
   inline IntrinsicType load( size_t index ) const
   {
      return dv_.load( index );
   }
   //**********************************************************************************************

   //**Loadu function******************************************************************************
   /*!\brief Unaligned load of an intrinsic element of the vector.
   //
   // \param index Access index. The index must be smaller than the number of vector elements.
   // \return The loaded intrinsic element.
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors.
   */
   inline IntrinsicType loadu( size_t index ) const
   {
      return dv_.loadu( index );
   }
   //**********************************************************************************************

   //**Store function******************************************************************************
   /*!\brief Aligned store of an intrinsic element of the vector.
   //
   // \param index Access index. The index must be smaller than the number of vector elements.
   // \param value The intrinsic element to be stored.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors.
   */
   inline void store( size_t index, const IntrinsicType& value )
   {
      dv_.store( index, value );
   }
   //**********************************************************************************************

   //**Storeu function*****************************************************************************
   /*!\brief Unaligned store of an intrinsic element of the vector.
   //
   // \param index Access index. The index must be smaller than the number of vector elements.
   // \param value The intrinsic element to be stored.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors.
   */
   inline void storeu( size_t index, const IntrinsicType& value )
   {
      dv_.storeu( index, value );
   }
   //**********************************************************************************************

   //**Stream function*****************************************************************************
   /*!\brief Aligned, non-temporal store of an intrinsic element of the vector.
   //
   // \param index Access index. The index must be smaller than the number of vector elements.
   // \param value The intrinsic element to be stored.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors.
   */
   inline void stream( size_t index, const IntrinsicType& value )
   {
      dv_.stream( index, value );
   }
   //**********************************************************************************************

   //**Transpose assignment of dense vectors*******************************************************
   /*!\brief Implementation of the transpose assignment of a dense vector.
   //
   // \param rhs The right-hand side dense vector to be assigned.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors. Instead of using this function use the
   // assignment operator.
   */
   template< typename VT2 >  // Type of the right-hand side dense vector
   inline void assign( const DenseVector<VT2,TF>& rhs )
   {
      BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG( VT2, TF );

      BLAZE_INTERNAL_ASSERT( dv_.size() == (~rhs).size(), "Invalid vector sizes" );

      const size_t n( size() );

      BLAZE_INTERNAL_ASSERT( ( n - ( n % 2UL ) ) == ( n & size_t(-2) ), "Invalid end calculation" );
      const size_t iend( n & size_t(-2) );

      for( size_t i=0UL; i<iend; i+=2UL ) {
         dv_[i    ] = (~rhs)[i    ];
         dv_[i+1UL] = (~rhs)[i+1UL];
      }
      if( iend < n )
         dv_[iend] = (~rhs)[iend];
   }
   //**********************************************************************************************

   //**Transpose assignment of sparse vectors******************************************************
   /*!\brief Implementation of the transpose assignment of a sparse vector.
   //
   // \param rhs The right-hand side sparse vector to be assigned.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors. Instead of using this function use the
   // assignment operator.
   */
   template< typename VT2 >  // Type of the right-hand side sparse vector
   inline void assign( const SparseVector<VT2,TF>& rhs )
   {
      BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG( VT2, TF );

      BLAZE_INTERNAL_ASSERT( dv_.size() == (~rhs).size(), "Invalid vector sizes" );

      typedef typename VT2::ConstIterator  RhsConstIterator;

      for( RhsConstIterator element=(~rhs).begin(); element!=(~rhs).end(); ++element )
         dv_[element->index()] = element->value();
   }
   //**********************************************************************************************

   //**Transpose addition assignment of dense vectors**********************************************
   /*!\brief Implementation of the transpose addition assignment of a dense vector.
   //
   // \param rhs The right-hand side dense vector to be added.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors. Instead of using this function use the
   // assignment operator.
   */
   template< typename VT2 >  // Type of the right-hand side dense vector
   inline void addAssign( const DenseVector<VT2,TF>& rhs )
   {
      BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG( VT2, TF );

      BLAZE_INTERNAL_ASSERT( dv_.size() == (~rhs).size(), "Invalid vector sizes" );

      const size_t n( size() );

      BLAZE_INTERNAL_ASSERT( ( n - ( n % 2UL ) ) == ( n & size_t(-2) ), "Invalid end calculation" );
      const size_t iend( n & size_t(-2) );

      for( size_t i=0UL; i<iend; i+=2UL ) {
         dv_[i    ] += (~rhs)[i    ];
         dv_[i+1UL] += (~rhs)[i+1UL];
      }
      if( iend < n )
         dv_[iend] += (~rhs)[iend];
   }
   //**********************************************************************************************

   //**Transpose addition assignment of sparse vectors*********************************************
   /*!\brief Implementation of the transpose addition assignment of a sparse vector.
   //
   // \param rhs The right-hand side sparse vector to be added.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors. Instead of using this function use the
   // assignment operator.
   */
   template< typename VT2 >  // Type of the right-hand side sparse vector
   inline void addAssign( const SparseVector<VT2,TF>& rhs )
   {
      BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG( VT2, TF );

      BLAZE_INTERNAL_ASSERT( dv_.size() == (~rhs).size(), "Invalid vector sizes" );

      typedef typename VT2::ConstIterator  RhsConstIterator;

      for( RhsConstIterator element=(~rhs).begin(); element!=(~rhs).end(); ++element )
         dv_[element->index()] += element->value();
   }
   //**********************************************************************************************

   //**Transpose subtraction assignment of dense vectors*******************************************
   /*!\brief Implementation of the transpose subtraction assignment of a dense vector.
   //
   // \param rhs The right-hand side dense vector to be subtracted.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors. Instead of using this function use the
   // assignment operator.
   */
   template< typename VT2 >  // Type of the right-hand side dense vector
   inline void subAssign( const DenseVector<VT2,TF>& rhs )
   {
      BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG( VT2, TF );

      BLAZE_INTERNAL_ASSERT( dv_.size() == (~rhs).size(), "Invalid vector sizes" );

      const size_t n( size() );

      BLAZE_INTERNAL_ASSERT( ( n - ( n % 2UL ) ) == ( n & size_t(-2) ), "Invalid end calculation" );
      const size_t iend( n & size_t(-2) );

      for( size_t i=0UL; i<iend; i+=2UL ) {
         dv_[i    ] -= (~rhs)[i    ];
         dv_[i+1UL] -= (~rhs)[i+1UL];
      }
      if( iend < n )
         dv_[iend] -= (~rhs)[iend];
   }
   //**********************************************************************************************

   //**Transpose subtraction assignment of sparse vectors******************************************
   /*!\brief Implementation of the transpose subtraction assignment of a sparse vector.
   //
   // \param rhs The right-hand side sparse vector to be subtracted.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors. Instead of using this function use the
   // assignment operator.
   */
   template< typename VT2 >  // Type of the right-hand side sparse vector
   inline void subAssign( const SparseVector<VT2,TF>& rhs )
   {
      BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG( VT2, TF );

      BLAZE_INTERNAL_ASSERT( dv_.size() == (~rhs).size(), "Invalid vector sizes" );

      typedef typename VT2::ConstIterator  RhsConstIterator;

      for( RhsConstIterator element=(~rhs).begin(); element!=(~rhs).end(); ++element )
         dv_[element->index()] -= element->value();
   }
   //**********************************************************************************************

   //**Transpose multiplication assignment of dense vectors****************************************
   /*!\brief Implementation of the transpose multiplication assignment of a dense vector.
   //
   // \param rhs The right-hand side dense vector to be multiplied.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors. Instead of using this function use the
   // assignment operator.
   */
   template< typename VT2 >  // Type of the right-hand side dense vector
   inline void multAssign( const DenseVector<VT2,TF>& rhs )
   {
      BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG( VT2, TF );

      BLAZE_INTERNAL_ASSERT( dv_.size() == (~rhs).size(), "Invalid vector sizes" );

      const size_t n( size() );

      BLAZE_INTERNAL_ASSERT( ( n - ( n % 2UL ) ) == ( n & size_t(-2) ), "Invalid end calculation" );
      const size_t iend( n & size_t(-2) );

      for( size_t i=0UL; i<iend; i+=2UL ) {
         dv_[i    ] *= (~rhs)[i    ];
         dv_[i+1UL] *= (~rhs)[i+1UL];
      }
      if( iend < n )
         dv_[iend] *= (~rhs)[iend];
   }
   //**********************************************************************************************

   //**Transpose multiplication assignment of sparse vectors***************************************
   /*!\brief Implementation of the transpose multiplication assignment of a sparse vector.
   //
   // \param rhs The right-hand side sparse vector to be multiplied.
   // \return void
   //
   // This function must \b NOT be called explicitly! It is used internally for the performance
   // optimized evaluation of expression templates. Calling this function explicitly might result
   // in erroneous results and/or in compilation errors. Instead of using this function use the
   // assignment operator.
   */
   template< typename VT2 >  // Type of the right-hand side dense vector
   inline void multAssign( const SparseVector<VT2,TF>& rhs )
   {
      BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG( VT2, TF );

      BLAZE_INTERNAL_ASSERT( dv_.size() == (~rhs).size(), "Invalid vector sizes" );

      typedef typename VT2::ConstIterator  RhsConstIterator;

      const VT tmp( dv_ );
      dv_.reset();

      for( RhsConstIterator element=(~rhs).begin(); element!=(~rhs).end(); ++element )
         dv_[element->index()] = tmp[element->index()] * element->value();
   }
   //**********************************************************************************************

 private:
   //**Member variables****************************************************************************
   VT& dv_;  //!< The dense vector operand.
   //**********************************************************************************************

   //**Compile time checks*************************************************************************
   /*! \cond BLAZE_INTERNAL */
   BLAZE_CONSTRAINT_MUST_BE_DENSE_VECTOR_TYPE( VT );
   BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG( VT, !TF );
   BLAZE_CONSTRAINT_MUST_NOT_BE_COMPUTATION_TYPE( VT );
   /*! \endcond */
   //**********************************************************************************************
};
//*************************************************************************************************




//=================================================================================================
//
//  GLOBAL OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Resetting the dense vector contained in a DVecTransposer.
// \ingroup dense_vector_expression
//
// \param v The dense vector to be resetted.
// \return void
*/
template< typename VT  // Type of the dense vector
        , bool TF >    // Transpose flag
inline void reset( DVecTransposer<VT,TF>& v )
{
   v.reset();
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  SUBVECTORTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename VT, bool TF >
struct SubvectorTrait< DVecTransposer<VT,TF> >
{
   typedef typename SubvectorTrait< typename DVecTransposer<VT,TF>::ResultType >::Type  Type;
};
/*! \endcond */
//*************************************************************************************************

} // namespace blaze

#endif
