//=================================================================================================
/*!
//  \file blaze/math/expressions/DVecTDVecMultExpr.h
//  \brief Header file for the dense vector/dense vector outer product expression
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

#ifndef _BLAZE_MATH_EXPRESSIONS_DVECTDVECMULTEXPR_H_
#define _BLAZE_MATH_EXPRESSIONS_DVECTDVECMULTEXPR_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <blaze/math/constraints/DenseMatrix.h>
#include <blaze/math/constraints/DenseVector.h>
#include <blaze/math/constraints/StorageOrder.h>
#include <blaze/math/constraints/TransposeFlag.h>
#include <blaze/math/expressions/Computation.h>
#include <blaze/math/expressions/DenseMatrix.h>
#include <blaze/math/expressions/Forward.h>
#include <blaze/math/expressions/VecTVecMultExpr.h>
#include <blaze/math/Intrinsics.h>
#include <blaze/math/shims/Serial.h>
#include <blaze/math/traits/ColumnExprTrait.h>
#include <blaze/math/traits/MultExprTrait.h>
#include <blaze/math/traits/MultTrait.h>
#include <blaze/math/traits/RowExprTrait.h>
#include <blaze/math/traits/SubmatrixExprTrait.h>
#include <blaze/math/traits/SubvectorExprTrait.h>
#include <blaze/math/typetraits/IsComputation.h>
#include <blaze/math/typetraits/IsExpression.h>
#include <blaze/math/typetraits/IsTemporary.h>
#include <blaze/system/Thresholds.h>
#include <blaze/util/Assert.h>
#include <blaze/util/constraints/Reference.h>
#include <blaze/util/EnableIf.h>
#include <blaze/util/logging/FunctionTrace.h>
#include <blaze/util/SelectType.h>
#include <blaze/util/Types.h>
#include <blaze/util/typetraits/IsReference.h>


namespace blaze {

//=================================================================================================
//
//  CLASS DVECTDVECMULTEXPR
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Expression object for outer products between two dense vectors.
// \ingroup dense_matrix_expression
//
// The DVecTDVecMultExpr class represents the compile time expression for outer products
// between dense vectors.
*/
template< typename VT1    // Type of the left-hand side dense vector
        , typename VT2 >  // Type of the right-hand side dense vector
class DVecTDVecMultExpr : public DenseMatrix< DVecTDVecMultExpr<VT1,VT2>, false >
                        , private VecTVecMultExpr
                        , private Computation
{
 private:
   //**Type definitions****************************************************************************
   typedef typename VT1::ResultType     RT1;  //!< Result type of the left-hand side dense vector expression.
   typedef typename VT2::ResultType     RT2;  //!< Result type of the right-hand side dense vector expression.
   typedef typename RT1::ElementType    ET1;  //!< Element type of the left-hand side dense vector expression.
   typedef typename RT2::ElementType    ET2;  //!< Element type of the right-hand side dense vector expression.
   typedef typename VT1::ReturnType     RN1;  //!< Return type of the left-hand side dense vector expression.
   typedef typename VT2::ReturnType     RN2;  //!< Return type of the right-hand side dense vector expression.
   typedef typename VT1::CompositeType  CT1;  //!< Composite type of the left-hand side dense vector expression.
   typedef typename VT2::CompositeType  CT2;  //!< Composite type of the right-hand side dense vector expression.
   //**********************************************************************************************

   //**********************************************************************************************
   //! Compilation switch for the composite type of the left-hand side dense vector expression.
   enum { evaluateLeft = IsComputation<VT1>::value || RequiresEvaluation<VT1>::value };
   //**********************************************************************************************

   //**********************************************************************************************
   //! Compilation switch for the composite type of the right-hand side dense vector expression.
   enum { evaluateRight = IsComputation<VT2>::value || RequiresEvaluation<VT2>::value };
   //**********************************************************************************************

   //**Return type evaluation**********************************************************************
   //! Compilation switch for the selection of the subscript operator return type.
   /*! The \a returnExpr compile time constant expression is a compilation switch for the
       selection of the \a ReturnType. If either vector operand returns a temporary vector
       or matrix, \a returnExpr will be set to \a false and the subscript operator will
       return it's result by value. Otherwise \a returnExpr will be set to \a true and
       the subscript operator may return it's result as an expression. */
   enum { returnExpr = !IsTemporary<RN1>::value && !IsTemporary<RN2>::value };

   //! Expression return type for the subscript operator.
   typedef typename MultExprTrait<RN1,RN2>::Type  ExprReturnType;
   //**********************************************************************************************

   //**Serial evaluation strategy******************************************************************
   //! Compilation switch for the serial evaluation strategy of the outer product expression.
   /*! The \a useAssign compile time constant expression represents a compilation switch for
       the serial evaluation strategy of the outer product expression. In case either of the
       two dense vector operands requires an evaluation, \a useAssign will be set to \a true
       and the outer product expression will be evaluated via the \a assign function family.
       Otherwise \a useAssign will be set to \a false and the expression will be evaluated
       via the function call operator. */
   enum { useAssign = ( evaluateLeft || evaluateRight ) };

   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename VT >
   struct UseAssign {
      enum { value = useAssign };
   };
   /*! \endcond */
   //**********************************************************************************************

   //**Parallel evaluation strategy****************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   /*! The UseSMPAssign struct is a helper struct for the selection of the parallel evaluation
       strategy. In case the right-hand side vector operand requires an intermediate evaluation,
       the nested \value will be set to 1, otherwise it will be 0. */
   template< typename VT >
   struct UseSMPAssign {
      enum { value = evaluateRight };
   };
   /*! \endcond */
   //**********************************************************************************************

   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   /*! In case all three involved data types are suited for a vectorized computation of the
       outer product, the nested \value will be set to 1, otherwise it will be 0. */
   template< typename T1, typename T2, typename T3 >
   struct UseVectorizedKernel {
      enum { value = T1::vectorizable && T2::vectorizable && T3::vectorizable &&
                     IsSame<typename T1::ElementType,typename T2::ElementType>::value &&
                     IsSame<typename T1::ElementType,typename T3::ElementType>::value &&
                     IntrinsicTrait<typename T1::ElementType>::multiplication };
   };
   /*! \endcond */
   //**********************************************************************************************

   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   /*! In case no vectorized computation is possible, the nested \value will be set to 1,
       otherwise it will be 0. */
   template< typename T1, typename T2, typename T3 >
   struct UseDefaultKernel {
      enum { value = !UseVectorizedKernel<T1,T2,T3>::value };
   };
   /*! \endcond */
   //**********************************************************************************************

 public:
   //**Type definitions****************************************************************************
   typedef DVecTDVecMultExpr<VT1,VT2>                  This;           //!< Type of this DVecTDVecMultExpr instance.
   typedef typename MultTrait<RT1,RT2>::Type           ResultType;     //!< Result type for expression template evaluations.
   typedef typename ResultType::OppositeType           OppositeType;   //!< Result type with opposite storage order for expression template evaluations.
   typedef typename ResultType::TransposeType          TransposeType;  //!< Transpose type for expression template evaluations.
   typedef typename ResultType::ElementType            ElementType;    //!< Resulting element type.
   typedef typename IntrinsicTrait<ElementType>::Type  IntrinsicType;  //!< Resulting intrinsic element type.

   //! Return type for expression template evaluations.
   typedef const typename SelectType< returnExpr, ExprReturnType, ElementType >::Type  ReturnType;

   //! Data type for composite expression templates.
   typedef typename SelectType< useAssign, const ResultType, const DVecTDVecMultExpr& >::Type  CompositeType;

   //! Composite type of the left-hand side dense vector expression.
   typedef typename SelectType< IsExpression<VT1>::value, const VT1, const VT1& >::Type  LeftOperand;

   //! Composite type of the right-hand side dense vector expression.
   typedef typename SelectType< IsExpression<VT2>::value, const VT2, const VT2& >::Type  RightOperand;

   //! Type for the assignment of the left-hand side dense vector operand.
   typedef typename SelectType< evaluateLeft, const RT1, CT1 >::Type  LT;

   //! Type for the assignment of the right-hand side dense vector operand.
   typedef typename SelectType< evaluateRight, const RT2, CT2 >::Type  RT;
   //**********************************************************************************************

   //**ConstIterator class definition**************************************************************
   /*!\brief Iterator over the elements of the dense matrix.
   */
   class ConstIterator
   {
    public:
      //**Type definitions*************************************************************************
      typedef std::random_access_iterator_tag  IteratorCategory;  //!< The iterator category.
      typedef ElementType                      ValueType;         //!< Type of the underlying elements.
      typedef ElementType*                     PointerType;       //!< Pointer return type.
      typedef ElementType&                     ReferenceType;     //!< Reference return type.
      typedef ptrdiff_t                        DifferenceType;    //!< Difference between two iterators.

      // STL iterator requirements
      typedef IteratorCategory  iterator_category;  //!< The iterator category.
      typedef ValueType         value_type;         //!< Type of the underlying elements.
      typedef PointerType       pointer;            //!< Pointer return type.
      typedef ReferenceType     reference;          //!< Reference return type.
      typedef DifferenceType    difference_type;    //!< Difference between two iterators.

      //! ConstIterator type of the left-hand side dense matrix expression.
      typedef typename VT1::ConstIterator  LeftIteratorType;

      //! ConstIterator type of the right-hand side dense matrix expression.
      typedef typename VT2::ConstIterator  RightIteratorType;
      //*******************************************************************************************

      //**Constructor******************************************************************************
      /*!\brief Constructor for the ConstIterator class.
      //
      // \param left Iterator to the initial left-hand side element.
      // \param right Iterator to the initial right-hand side element.
      */
      explicit inline ConstIterator( LeftIteratorType left, RightIteratorType right )
         : left_ ( left  )  // Iterator to the current left-hand side element
         , right_( right )  // Iterator to the current right-hand side element
      {}
      //*******************************************************************************************

      //**Addition assignment operator*************************************************************
      /*!\brief Addition assignment operator.
      //
      // \param inc The increment of the iterator.
      // \return The incremented iterator.
      */
      inline ConstIterator& operator+=( size_t inc ) {
         right_ += inc;
         return *this;
      }
      //*******************************************************************************************

      //**Subtraction assignment operator**********************************************************
      /*!\brief Subtraction assignment operator.
      //
      // \param dec The decrement of the iterator.
      // \return The decremented iterator.
      */
      inline ConstIterator& operator-=( size_t dec ) {
         right_ -= dec;
         return *this;
      }
      //*******************************************************************************************

      //**Prefix increment operator****************************************************************
      /*!\brief Pre-increment operator.
      //
      // \return Reference to the incremented iterator.
      */
      inline ConstIterator& operator++() {
         ++right_;
         return *this;
      }
      //*******************************************************************************************

      //**Postfix increment operator***************************************************************
      /*!\brief Post-increment operator.
      //
      // \return The previous position of the iterator.
      */
      inline const ConstIterator operator++( int ) {
         return ConstIterator( left_, right_++ );
      }
      //*******************************************************************************************

      //**Prefix decrement operator****************************************************************
      /*!\brief Pre-decrement operator.
      //
      // \return Reference to the decremented iterator.
      */
      inline ConstIterator& operator--() {
         --right_;
         return *this;
      }
      //*******************************************************************************************

      //**Postfix decrement operator***************************************************************
      /*!\brief Post-decrement operator.
      //
      // \return The previous position of the iterator.
      */
      inline const ConstIterator operator--( int ) {
         return ConstIterator( left_, right_-- );
      }
      //*******************************************************************************************

      //**Element access operator******************************************************************
      /*!\brief Direct access to the element at the current iterator position.
      //
      // \return The resulting value.
      */
      inline ReturnType operator*() const {
         return (*left_) * (*right_);
      }
      //*******************************************************************************************

      //**Load function****************************************************************************
      /*!\brief Access to the intrinsic elements of the matrix.
      //
      // \return The resulting intrinsic value.
      */
      inline IntrinsicType load() const {
         return set( *left_ ) * right_.load();
      }
      //*******************************************************************************************

      //**Equality operator************************************************************************
      /*!\brief Equality comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the iterators refer to the same element, \a false if not.
      */
      inline bool operator==( const ConstIterator& rhs ) const {
         return right_ == rhs.right_;
      }
      //*******************************************************************************************

      //**Inequality operator**********************************************************************
      /*!\brief Inequality comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the iterators don't refer to the same element, \a false if they do.
      */
      inline bool operator!=( const ConstIterator& rhs ) const {
         return right_ != rhs.right_;
      }
      //*******************************************************************************************

      //**Less-than operator***********************************************************************
      /*!\brief Less-than comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the left-hand side iterator is smaller, \a false if not.
      */
      inline bool operator<( const ConstIterator& rhs ) const {
         return right_ < rhs.right_;
      }
      //*******************************************************************************************

      //**Greater-than operator********************************************************************
      /*!\brief Greater-than comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the left-hand side iterator is greater, \a false if not.
      */
      inline bool operator>( const ConstIterator& rhs ) const {
         return right_ > rhs.right_;
      }
      //*******************************************************************************************

      //**Less-or-equal-than operator**************************************************************
      /*!\brief Less-than comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the left-hand side iterator is smaller or equal, \a false if not.
      */
      inline bool operator<=( const ConstIterator& rhs ) const {
         return right_ <= rhs.right_;
      }
      //*******************************************************************************************

      //**Greater-or-equal-than operator***********************************************************
      /*!\brief Greater-than comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the left-hand side iterator is greater or equal, \a false if not.
      */
      inline bool operator>=( const ConstIterator& rhs ) const {
         return right_ >= rhs.right_;
      }
      //*******************************************************************************************

      //**Subtraction operator*********************************************************************
      /*!\brief Calculating the number of elements between two iterators.
      //
      // \param rhs The right-hand side iterator.
      // \return The number of elements between the two iterators.
      */
      inline DifferenceType operator-( const ConstIterator& rhs ) const {
         return right_ - rhs.right_;
      }
      //*******************************************************************************************

      //**Addition operator************************************************************************
      /*!\brief Addition between a ConstIterator and an integral value.
      //
      // \param it The iterator to be incremented.
      // \param inc The number of elements the iterator is incremented.
      // \return The incremented iterator.
      */
      friend inline const ConstIterator operator+( const ConstIterator& it, size_t inc ) {
         return ConstIterator( it.left_, it.right_ + inc );
      }
      //*******************************************************************************************

      //**Addition operator************************************************************************
      /*!\brief Addition between an integral value and a ConstIterator.
      //
      // \param inc The number of elements the iterator is incremented.
      // \param it The iterator to be incremented.
      // \return The incremented iterator.
      */
      friend inline const ConstIterator operator+( size_t inc, const ConstIterator& it ) {
         return ConstIterator( it.left_, it.right_ + inc );
      }
      //*******************************************************************************************

      //**Subtraction operator*********************************************************************
      /*!\brief Subtraction between a ConstIterator and an integral value.
      //
      // \param it The iterator to be decremented.
      // \param dec The number of elements the iterator is decremented.
      // \return The decremented iterator.
      */
      friend inline const ConstIterator operator-( const ConstIterator& it, size_t dec ) {
         return ConstIterator( it.left_, it.right_ - dec );
      }
      //*******************************************************************************************

    private:
      //**Member variables*************************************************************************
      LeftIteratorType  left_;   //!< Iterator to the current left-hand side element.
      RightIteratorType right_;  //!< Iterator to the current right-hand side element.
      //*******************************************************************************************
   };
   //**********************************************************************************************

   //**Compilation flags***************************************************************************
   //! Compilation switch for the expression template evaluation strategy.
   enum { vectorizable = VT1::vectorizable && VT2::vectorizable &&
                         IsSame<ET1,ET2>::value &&
                         IntrinsicTrait<ET1>::multiplication };

   //! Compilation switch for the expression template assignment strategy.
   enum { smpAssignable = VT1::smpAssignable && !evaluateRight };
   //**********************************************************************************************

   //**Constructor*********************************************************************************
   /*!\brief Constructor for the DVecTDVecMultExpr class.
   //
   // \param lhs The left-hand side dense vector operand of the multiplication expression.
   // \param rhs The right-hand side dense vector operand of the multiplication expression.
   */
   explicit inline DVecTDVecMultExpr( const VT1& lhs, const VT2& rhs )
      : lhs_( lhs )  // Left-hand side dense vector of the multiplication expression
      , rhs_( rhs )  // Right-hand side dense vector of the multiplication expression
   {}
   //**********************************************************************************************

   //**Access operator*****************************************************************************
   /*!\brief 2D-access to the matrix elements.
   //
   // \param i Access index for the row. The index has to be in the range \f$[0..M-1]\f$.
   // \param j Access index for the column. The index has to be in the range \f$[0..N-1]\f$.
   // \return The resulting value.
   */
   inline ReturnType operator()( size_t i, size_t j ) const {
      BLAZE_INTERNAL_ASSERT( i < lhs_.size(), "Invalid row access index"    );
      BLAZE_INTERNAL_ASSERT( j < rhs_.size(), "Invalid column access index" );

      return lhs_[i] * rhs_[j];
   }
   //**********************************************************************************************

   //**Load function*******************************************************************************
   /*!\brief Access to the intrinsic elements of the matrix.
   //
   // \param i Access index for the row. The index has to be in the range \f$[0..M-1]\f$.
   // \param j Access index for the column. The index has to be in the range \f$[0..N-1]\f$.
   // \return Reference to the accessed values.
   */
   inline IntrinsicType load( size_t i, size_t j ) const {
      typedef IntrinsicTrait<ElementType>  IT;
      BLAZE_INTERNAL_ASSERT( i < lhs_.size()    , "Invalid row access index"    );
      BLAZE_INTERNAL_ASSERT( j < rhs_.size()    , "Invalid column access index" );
      BLAZE_INTERNAL_ASSERT( j % IT::size == 0UL, "Invalid column access index" );
      const IntrinsicType xmm1( set( lhs_[i] ) );
      const IntrinsicType xmm2( rhs_.load( j ) );
      return xmm1 * xmm2;
   }
   //**********************************************************************************************

   //**Begin function******************************************************************************
   /*!\brief Returns an iterator to the first non-zero element of row \a i.
   //
   // \param i The row index.
   // \return Iterator to the first non-zero element of row \a i.
   */
   inline ConstIterator begin( size_t i ) const {
      BLAZE_INTERNAL_ASSERT( i < lhs_.size(), "Invalid row access index" );
      return ConstIterator( lhs_.begin()+i, rhs_.begin() );
   }
   //**********************************************************************************************

   //**End function********************************************************************************
   /*!\brief Returns an iterator just past the last non-zero element of row \a i.
   //
   // \param i The row index.
   // \return Iterator just past the last non-zero element of row \a i.
   */
   inline ConstIterator end( size_t i ) const {
      BLAZE_INTERNAL_ASSERT( i < lhs_.size(), "Invalid row access index" );
      return ConstIterator( lhs_.begin()+i, rhs_.end() );
   }
   //**********************************************************************************************

   //**Rows function*******************************************************************************
   /*!\brief Returns the current number of rows of the matrix.
   //
   // \return The number of rows of the matrix.
   */
   inline size_t rows() const {
      return lhs_.size();
   }
   //**********************************************************************************************

   //**Columns function****************************************************************************
   /*!\brief Returns the current number of columns of the matrix.
   //
   // \return The number of columns of the matrix.
   */
   inline size_t columns() const {
      return rhs_.size();
   }
   //**********************************************************************************************

   //**Left operand access*************************************************************************
   /*!\brief Returns the left-hand side dense vector operand.
   //
   // \return The left-hand side dense vector operand.
   */
   inline LeftOperand leftOperand() const {
      return lhs_;
   }
   //**********************************************************************************************

   //**Right operand access************************************************************************
   /*!\brief Returns the right-hand side dense vector operand.
   //
   // \return The right-hand side dense vector operand.
   */
   inline RightOperand rightOperand() const {
      return rhs_;
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the expression can alias with the given address \a alias.
   //
   // \param alias The alias to be checked.
   // \return \a true in case the expression can alias, \a false otherwise.
   */
   template< typename T >
   inline bool canAlias( const T* alias ) const {
      return ( lhs_.canAlias( alias ) || rhs_.canAlias( alias ) );
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the expression is aliased with the given address \a alias.
   //
   // \param alias The alias to be checked.
   // \return \a true in case an alias effect is detected, \a false otherwise.
   */
   template< typename T >
   inline bool isAliased( const T* alias ) const {
      return ( lhs_.isAliased( alias ) || rhs_.isAliased( alias ) );
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the operands of the expression are properly aligned in memory.
   //
   // \return \a true in case the operands are aligned, \a false if not.
   */
   inline bool isAligned() const {
      return lhs_.isAligned() && rhs_.isAligned();
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the expression can be used in SMP assignments.
   //
   // \return \a true in case the expression can be used in SMP assignments, \a false if not.
   */
   inline bool canSMPAssign() const {
      return ( rows() > SMP_DVECTDVECMULT_THRESHOLD );
   }
   //**********************************************************************************************

 private:
   //**Member variables****************************************************************************
   LeftOperand  lhs_;  //!< Left-hand side dense vector of the multiplication expression.
   RightOperand rhs_;  //!< Right-hand side dense vector of the multiplication expression.
   //**********************************************************************************************

   //**Assignment to row-major dense matrices******************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a dense vector-dense vector outer product to a row-major dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side outer product expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a dense vector-dense
   // vector outer product expression to a row-major dense matrix. Due to the explicit
   // application of the SFINAE principle, this operator can only be selected by the compiler
   // in case either of the two operands requires an intermediate evaluation.
   */
   template< typename MT >  // Type of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT> >::Type
      assign( DenseMatrix<MT,false>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      LT x( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side dense vector operand
      RT y( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side dense vector operand

      BLAZE_INTERNAL_ASSERT( x.size() == rhs.lhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == rhs.rhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( x.size() == (~lhs).rows()   , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == (~lhs).columns(), "Invalid vector size" );

      DVecTDVecMultExpr::selectAssignKernel( ~lhs, x, y );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Default assignment to row-major dense matrices**********************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Default assignment of a dense vector-dense vector outer product to a row-major dense
   //        matrix (\f$ A=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the default assignment kernel for the dense vector-dense vector
   // outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseDefaultKernel<MT,VT3,VT4> >::Type
      selectAssignKernel( DenseMatrix<MT,false>& A, const VT3& x, const VT4& y )
   {
      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      const size_t jend( n & size_t(-2) );
      BLAZE_INTERNAL_ASSERT( ( n - ( n % 2UL ) ) == jend, "Invalid end calculation" );

      for( size_t i=0UL; i<m; ++i ) {
         for( size_t j=0UL; j<jend; j+=2UL ) {
            (~A)(i,j    ) = x[i] * y[j  ];
            (~A)(i,j+1UL) = x[i] * y[j+1];
         }
         if( jend < n ) {
            (~A)(i,jend) = x[i] * y[jend];
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Vectorized assignment to row-major dense matrices*******************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Vectorized assignment of a dense vector-dense vector outer product to a row-major
   //        dense matrix (\f$ A=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the vectorized assignment kernel for the dense vector-dense vector
   // outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseVectorizedKernel<MT,VT3,VT4> >::Type
      selectAssignKernel( DenseMatrix<MT,false>& A, const VT3& x, const VT4& y )
   {
      typedef IntrinsicTrait<ElementType>  IT;

      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      for( size_t i=0UL; i<m; ++i )
      {
         const IntrinsicType x1( set( x[i] ) );

         for( size_t j=0UL; j<n; j+=IT::size ) {
            (~A).store( i, j, x1 * y.load(j) );
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Assignment to column-major dense matrices***************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a dense vector-dense vector outer product to a column-major dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side outer product expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a dense vector-dense
   // vector outer product expression to a column-major dense matrix.
   */
   template< typename MT >  // Type of the target dense matrix
   friend inline void assign( DenseMatrix<MT,true>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      LT x( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side dense vector operand
      RT y( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side dense vector operand

      BLAZE_INTERNAL_ASSERT( x.size() == rhs.lhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == rhs.rhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( x.size() == (~lhs).rows()   , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == (~lhs).columns(), "Invalid vector size" );

      DVecTDVecMultExpr::selectAssignKernel( ~lhs, x, y );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Default assignment to column-major dense matrices*******************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Default assignment of a dense vector-dense vector outer product to a column-major
   //        dense matrix (\f$ A=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the default assignment kernel for the dense vector-dense vector
   // outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseDefaultKernel<MT,VT3,VT4> >::Type
      selectAssignKernel( DenseMatrix<MT,true>& A, const VT3& x, const VT4& y )
   {
      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      const size_t iend( m & size_t(-2) );
      BLAZE_INTERNAL_ASSERT( ( m - ( m % 2UL ) ) == iend, "Invalid end calculation" );

      for( size_t j=0UL; j<n; ++j ) {
         for( size_t i=0UL; i<iend; i+=2UL ) {
            (~A)(i    ,j) = x[i  ] * y[j];
            (~A)(i+1UL,j) = x[i+1] * y[j];
         }
         if( iend < m ) {
            (~A)(iend,j) = x[iend] * y[j];
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Vectorized assignment to column-major dense matrices****************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Vectorized assignment of a dense vector-dense vector outer product to a column-major
   //        dense matrix (\f$ A=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the vectorized assignment kernel for the dense vector-dense vector
   // outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseVectorizedKernel<MT,VT3,VT4> >::Type
      selectAssignKernel( DenseMatrix<MT,true>& A, const VT3& x, const VT4& y )
   {
      typedef IntrinsicTrait<ElementType>  IT;

      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      for( size_t j=0UL; j<n; ++j )
      {
         const IntrinsicType y1( set( y[j] ) );

         for( size_t i=0UL; i<m; i+=IT::size ) {
            (~A).store( i, j, x.load(i) * y1 );
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Assignment to sparse matrices***************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a dense vector-dense vector outer product to a sparse matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side outer product expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a dense vector-dense
   // vector outer product expression to a sparse matrix.
   */
   template< typename MT  // Type of the target sparse matrix
           , bool SO >    // Storage order of the target sparse matrix
   friend inline void assign( SparseMatrix<MT,SO>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      typedef typename SelectType< SO, OppositeType, ResultType >::Type  TmpType;

      BLAZE_CONSTRAINT_MUST_BE_DENSE_MATRIX_TYPE( ResultType );
      BLAZE_CONSTRAINT_MUST_BE_DENSE_MATRIX_TYPE( OppositeType );
      BLAZE_CONSTRAINT_MUST_BE_ROW_MAJOR_MATRIX_TYPE( ResultType );
      BLAZE_CONSTRAINT_MUST_BE_COLUMN_MAJOR_MATRIX_TYPE( OppositeType );
      BLAZE_CONSTRAINT_MATRICES_MUST_HAVE_SAME_STORAGE_ORDER( MT, TmpType );
      BLAZE_CONSTRAINT_MUST_BE_REFERENCE_TYPE( typename TmpType::CompositeType );

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      const TmpType tmp( serial( rhs ) );
      assign( ~lhs, tmp );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Addition assignment to row-major dense matrices*********************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Addition assignment of a dense vector-dense vector outer product to a row-major
   //        dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side outer product expression to be added.
   // \return void
   //
   // This function implements the performance optimized addition assignment of a dense vector-
   // dense vector outer product expression to a row-major dense matrix. Due to the explicit
   // application of the SFINAE principle, this operator can only be selected by the compiler
   // in case either of the two operands requires an intermediate evaluation.
   */
   template< typename MT >  // Type of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT> >::Type
      addAssign( DenseMatrix<MT,false>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      LT x( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side dense vector operand
      RT y( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side dense vector operand

      BLAZE_INTERNAL_ASSERT( x.size() == rhs.lhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == rhs.rhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( x.size() == (~lhs).rows()   , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == (~lhs).columns(), "Invalid vector size" );

      DVecTDVecMultExpr::selectAddAssignKernel( ~lhs, x, y );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Default addition assignment to row-major dense matrices*************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Default addition assignment of a dense vector-dense vector outer product to a
   //        row-major dense matrix (\f$ A+=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the default addition assignment kernel for the dense vector-dense
   // vector outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseDefaultKernel<MT,VT3,VT4> >::Type
      selectAddAssignKernel( DenseMatrix<MT,false>& A, const VT3& x, const VT4& y )
   {
      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      const size_t jend( n & size_t(-2) );
      BLAZE_INTERNAL_ASSERT( ( n - ( n % 2UL ) ) == jend, "Invalid end calculation" );

      for( size_t i=0UL; i<m; ++i ) {
         for( size_t j=0UL; j<jend; j+=2UL ) {
            (~A)(i,j    ) += x[i] * y[j    ];
            (~A)(i,j+1UL) += x[i] * y[j+1UL];
         }
         if( jend < n ) {
            (~A)(i,jend) += x[i] * y[jend];
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Vectorized addition assignment to row-major dense matrices**********************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Vectorized addition assignment of a dense vector-dense vector outer product to a
   //        row-major dense matrix (\f$ A+=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the vectorized addition assignment kernel for the dense vector-
   // dense vector outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseVectorizedKernel<MT,VT3,VT4> >::Type
      selectAddAssignKernel( DenseMatrix<MT,false>& A, const VT3& x, const VT4& y )
   {
      typedef IntrinsicTrait<ElementType>  IT;

      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      for( size_t i=0UL; i<m; ++i )
      {
         const IntrinsicType x1( set( x[i] ) );

         for( size_t j=0UL; j<n; j+=IT::size ) {
            (~A).store( i, j, (~A).load(i,j) + x1 * y.load(j) );
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Addition assignment to column-major dense matrices******************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Addition assignment of a dense vector-dense vector outer product to a column-major
   //        dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side outer product expression to be added.
   // \return void
   //
   // This function implements the performance optimized addition assignment of a dense vector-
   // dense vector outer product expression to a column-major dense matrix.
   */
   template< typename MT >  // Type of the target dense matrix
   friend inline void addAssign( DenseMatrix<MT,true>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      LT x( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side dense vector operand
      RT y( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side dense vector operand

      BLAZE_INTERNAL_ASSERT( x.size() == rhs.lhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == rhs.rhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( x.size() == (~lhs).rows()   , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == (~lhs).columns(), "Invalid vector size" );

      DVecTDVecMultExpr::selectAddAssignKernel( ~lhs, x, y );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Default addition assignment to column dense matrices****************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Default addition assignment of a dense vector-dense vector outer product to a
   //        column-major dense matrix (\f$ A+=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the default addition assignment kernel for the dense vector-dense
   // vector outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseDefaultKernel<MT,VT3,VT4> >::Type
      selectAddAssignKernel( DenseMatrix<MT,true>& A, const VT3& x, const VT4& y )
   {
      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      const size_t iend( m & size_t(-2) );
      BLAZE_INTERNAL_ASSERT( ( m - ( m % 2UL ) ) == iend, "Invalid end calculation" );

      for( size_t j=0UL; j<n; ++j ) {
         for( size_t i=0UL; i<iend; i+=2UL ) {
            (~A)(i    ,j) += x[i    ] * y[j];
            (~A)(i+1UL,j) += x[i+1UL] * y[j];
         }
         if( iend < m ) {
            (~A)(iend,j) += x[iend] * y[j];
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Vectorized addition assignment to column-major dense matrices*******************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Vectorized addition assignment of a dense vector-dense vector outer product to a
   //        column-major dense matrix (\f$ A+=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the vectorized addition assignment kernel for the dense vector-
   // dense vector outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseVectorizedKernel<MT,VT3,VT4> >::Type
      selectAddAssignKernel( DenseMatrix<MT,true>& A, const VT3& x, const VT4& y )
   {
      typedef IntrinsicTrait<ElementType>  IT;

      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      for( size_t j=0UL; j<n; ++j )
      {
         const IntrinsicType y1( set( y[j] ) );

         for( size_t i=0UL; i<m; i+=IT::size ) {
            (~A).store( i, j, (~A).load(i,j) + x.load(i) * y1 );
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Addition assignment to sparse matrices******************************************************
   // No special implementation for the addition assignment to sparse matrices.
   //**********************************************************************************************

   //**Subtraction assignment to row-major dense matrices******************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Subtraction assignment of a dense vector-dense vector outer product to a row-major
   //        dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side outer product expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized subtraction assignment of a dense vector-
   // dense vector outer product expression to a row-major dense matrix. Due to the explicit
   // application of the SFINAE principle, this operator can only be selected by the compiler
   // in case either of the two operands requires an intermediate evaluation.
   */
   template< typename MT >  // Type of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT> >::Type
      subAssign( DenseMatrix<MT,false>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      LT x( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side dense vector operand
      RT y( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side dense vector operand

      BLAZE_INTERNAL_ASSERT( x.size() == rhs.lhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == rhs.rhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( x.size() == (~lhs).rows()   , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == (~lhs).columns(), "Invalid vector size" );

      DVecTDVecMultExpr::selectSubAssignKernel( ~lhs, x, y );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Default subtraction assignment to row-major dense matrices**********************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Default subtraction assignment of a dense vector-dense vector outer product to a
   //        row-major dense matrix (\f$ A-=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the default subtraction assignment kernel for the dense vector-
   // dense vector outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseDefaultKernel<MT,VT3,VT4> >::Type
      selectSubAssignKernel( DenseMatrix<MT,false>& A, const VT3& x, const VT4& y )
   {
      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      const size_t jend( n & size_t(-2) );
      BLAZE_INTERNAL_ASSERT( ( n - ( n % 2UL ) ) == jend, "Invalid end calculation" );

      for( size_t i=0UL; i<m; ++i ) {
         for( size_t j=0UL; j<jend; j+=2UL ) {
            (~A)(i,j    ) -= x[i] * y[j    ];
            (~A)(i,j+1UL) -= x[i] * y[j+1UL];
         }
         if( jend < n ) {
            (~A)(i,jend) -= x[i] * y[jend];
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Vectorized subtraction assignment to row-major dense matrices*******************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Vectorized subtraction assignment of a dense vector-dense vector outer product to a
   //        row-major dense matrix (\f$ A-=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the vectorized subtraction assignment kernel for the dense vector-
   // dense vector outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseVectorizedKernel<MT,VT3,VT4> >::Type
      selectSubAssignKernel( DenseMatrix<MT,false>& A, const VT3& x, const VT4& y )
   {
      typedef IntrinsicTrait<ElementType>  IT;

      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      for( size_t i=0UL; i<m; ++i )
      {
         const IntrinsicType x1( set( x[i] ) );

         for( size_t j=0UL; j<n; j+=IT::size ) {
            (~A).store( i, j, (~A).load(i,j) - x1 * y.load(j) );
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Subtraction assignment to column-major dense matrices***************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Subtraction assignment of a dense vector-dense vector outer product to a column-major
   //        dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side outer product expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized subtraction assignment of a dense vector-
   // dense vector outer product expression to a column-major dense matrix.
   */
   template< typename MT >  // Type of the target dense matrix
   friend inline void subAssign( DenseMatrix<MT,true>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      LT x( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side dense vector operand
      RT y( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side dense vector operand

      BLAZE_INTERNAL_ASSERT( x.size() == rhs.lhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == rhs.rhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( x.size() == (~lhs).rows()   , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == (~lhs).columns(), "Invalid vector size" );

      DVecTDVecMultExpr::selectSubAssignKernel( ~lhs, x, y );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Default subtraction assignment to column dense matrices*************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Default subtraction assignment of a dense vector-dense vector outer product to a
   //        column-major dense matrix (\f$ A-=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the default subtraction assignment kernel for the dense vector-
   // dense vector outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseDefaultKernel<MT,VT3,VT4> >::Type
      selectSubAssignKernel( DenseMatrix<MT,true>& A, const VT3& x, const VT4& y )
   {
      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      const size_t iend( m & size_t(-2) );
      BLAZE_INTERNAL_ASSERT( ( m - ( m % 2UL ) ) == iend, "Invalid end calculation" );

      for( size_t j=0UL; j<n; ++j ) {
         for( size_t i=0UL; i<iend; i+=2UL ) {
            (~A)(i    ,j) -= x[i    ] * y[j];
            (~A)(i+1UL,j) -= x[i+1UL] * y[j];
         }
         if( iend < m ) {
            (~A)(iend,j) -= x[iend] * y[j];
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Vectorized subtraction assignment to column-major dense matrices****************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Vectorized subtraction assignment of a dense vector-dense vector outer product to a
   //        column-major dense matrix (\f$ A-=\vec{x}*\vec{y}^T \f$).
   // \ingroup dense_matrix
   //
   // \param A The target left-hand side dense matrix.
   // \param x The left-hand side dense vector operand.
   // \param y The right-hand side dense vector operand.
   // \return void
   //
   // This function implements the vectorized subtraction assignment kernel for the dense vector-
   // dense vector outer product.
   */
   template< typename MT     // Type of the left-hand side target matrix
           , typename VT3    // Type of the left-hand side vector operand
           , typename VT4 >  // Type of the right-hand side vector operand
   static inline typename EnableIf< UseVectorizedKernel<MT,VT3,VT4> >::Type
      selectSubAssignKernel( DenseMatrix<MT,true>& A, const VT3& x, const VT4& y )
   {
      typedef IntrinsicTrait<ElementType>  IT;

      const size_t m( (~A).rows() );
      const size_t n( (~A).columns() );

      for( size_t j=0UL; j<n; ++j )
      {
         const IntrinsicType y1( set( y[j] ) );

         for( size_t i=0UL; i<m; i+=IT::size ) {
            (~A).store( i, j, (~A).load(i,j) - x.load(i) * y1 );
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Subtraction assignment to sparse matrices***************************************************
   // No special implementation for the subtraction assignment to sparse matrices.
   //**********************************************************************************************

   //**Multiplication assignment to dense matrices*************************************************
   // No special implementation for the multiplication assignment to dense matrices.
   //**********************************************************************************************

   //**Multiplication assignment to sparse matrices************************************************
   // No special implementation for the multiplication assignment to sparse matrices.
   //**********************************************************************************************

   //**SMP assignment to dense matrices************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP assignment of a dense vector-dense vector outer product to a dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side outer product expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized SMP assignment of a dense vector-dense
   // vector outer product expression to a dense matrix. Due to the explicit application of the
   // SFINAE principle, this operator can only be selected by the compiler in case the expression
   // specific parallel evaluation strategy is selected.
   */
   template< typename MT  // Type of the target dense matrix
           , bool SO >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT> >::Type
      smpAssign( DenseMatrix<MT,SO>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      LT x( rhs.lhs_ );  // Evaluation of the left-hand side dense vector operand
      RT y( rhs.rhs_ );  // Evaluation of the right-hand side dense vector operand

      BLAZE_INTERNAL_ASSERT( x.size() == rhs.lhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == rhs.rhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( x.size() == (~lhs).rows()   , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == (~lhs).columns(), "Invalid vector size" );

      smpAssign( ~lhs, x * y );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP assignment to sparse matrices***********************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP assignment of a dense vector-dense vector outer product to a sparse matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side outer product expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized SMP assignment of a dense vector-dense
   // vector outer product expression to a sparse matrix. Due to the explicit application of the
   // SFINAE principle, this operator can only be selected by the compiler in case the expression
   // specific parallel evaluation strategy is selected.
   */
   template< typename MT  // Type of the target sparse matrix
           , bool SO >    // Storage order of the target sparse matrix
   friend inline typename EnableIf< UseSMPAssign<MT> >::Type
      smpAssign( SparseMatrix<MT,SO>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      typedef typename SelectType< SO, OppositeType, ResultType >::Type  TmpType;

      BLAZE_CONSTRAINT_MUST_BE_DENSE_MATRIX_TYPE( ResultType );
      BLAZE_CONSTRAINT_MUST_BE_DENSE_MATRIX_TYPE( OppositeType );
      BLAZE_CONSTRAINT_MUST_BE_ROW_MAJOR_MATRIX_TYPE( ResultType );
      BLAZE_CONSTRAINT_MUST_BE_COLUMN_MAJOR_MATRIX_TYPE( OppositeType );
      BLAZE_CONSTRAINT_MATRICES_MUST_HAVE_SAME_STORAGE_ORDER( MT, TmpType );
      BLAZE_CONSTRAINT_MUST_BE_REFERENCE_TYPE( typename TmpType::CompositeType );

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      const TmpType tmp( rhs );
      smpAssign( ~lhs, tmp );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP addition assignment to dense matrices***************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP addition assignment of a dense vector-dense vector outer product to a dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side outer product expression to be added.
   // \return void
   //
   // This function implements the performance optimized SMP addition assignment of a dense
   // vector-dense vector outer product expression to a dense matrix. Due to the explicit
   // application of the SFINAE principle, this operator can only be selected by the compiler
   // in case the expression specific parallel evaluation strategy is selected.
   */
   template< typename MT >  // Type of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT> >::Type
      smpAddAssign( DenseMatrix<MT,false>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      LT x( rhs.lhs_ );  // Evaluation of the left-hand side dense vector operand
      RT y( rhs.rhs_ );  // Evaluation of the right-hand side dense vector operand

      BLAZE_INTERNAL_ASSERT( x.size() == rhs.lhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == rhs.rhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( x.size() == (~lhs).rows()   , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == (~lhs).columns(), "Invalid vector size" );

      smpAddAssign( ~lhs, x * y );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP addition assignment to sparse matrices**************************************************
   // No special implementation for the SMP addition assignment to sparse matrices.
   //**********************************************************************************************

   //**SMP subtraction assignment to dense matrices************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP subtraction assignment of a dense vector-dense vector outer product to a dense
   //        matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side outer product expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized SMP subtraction assignment of a dense
   // vector-dense vector outer product expression to a dense matrix. Due to the explicit
   // application of the SFINAE principle, this operator can only be selected by the compiler
   // in case the expression specific parallel evaluation strategy is selected.
   */
   template< typename MT >  // Type of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT> >::Type
      smpSubAssign( DenseMatrix<MT,false>& lhs, const DVecTDVecMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      LT x( rhs.lhs_ );  // Evaluation of the left-hand side dense vector operand
      RT y( rhs.rhs_ );  // Evaluation of the right-hand side dense vector operand

      BLAZE_INTERNAL_ASSERT( x.size() == rhs.lhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == rhs.rhs_.size() , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( x.size() == (~lhs).rows()   , "Invalid vector size" );
      BLAZE_INTERNAL_ASSERT( y.size() == (~lhs).columns(), "Invalid vector size" );

      smpSubAssign( ~lhs, x * y );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP subtraction assignment to sparse matrices***********************************************
   // No special implementation for the SMP subtraction assignment to sparse matrices.
   //**********************************************************************************************

   //**SMP multiplication assignment to dense matrices*********************************************
   // No special implementation for the SMP multiplication assignment to dense matrices.
   //**********************************************************************************************

   //**SMP multiplication assignment to sparse matrices********************************************
   // No special implementation for the SMP multiplication assignment to sparse matrices.
   //**********************************************************************************************

   //**Compile time checks*************************************************************************
   /*! \cond BLAZE_INTERNAL */
   BLAZE_CONSTRAINT_MUST_BE_DENSE_VECTOR_TYPE ( VT1 );
   BLAZE_CONSTRAINT_MUST_BE_DENSE_VECTOR_TYPE ( VT2 );
   BLAZE_CONSTRAINT_MUST_BE_COLUMN_VECTOR_TYPE( VT1 );
   BLAZE_CONSTRAINT_MUST_BE_ROW_VECTOR_TYPE   ( VT2 );
   /*! \endcond */
   //**********************************************************************************************
};
//*************************************************************************************************




//=================================================================================================
//
//  GLOBAL BINARY ARITHMETIC OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Multiplication operator for the outer product of two dense vectors
//        (\f$ A=\vec{b}*\vec{c}^T \f$).
// \ingroup dense_matrix
//
// \param lhs The left-hand side dense vector for the outer product.
// \param rhs The right-hand side transpose dense vector for the outer product.
// \return The resulting dense matrix.
//
// This operator represents the outer product between a dense vector and a transpose dense
// vector:

   \code
   using blaze::columnVector;
   using blaze::rowMajor;

   blaze::DynamicVector<double,columnVector> a, b;
   blaze::DynamicMatrix<double,rowMajor> A;
   // ... Resizing and initialization
   A = a * trans(b);
   \endcode

// The operator returns an expression representing a dense matrix of the higher-order element
// type of the two involved element types \a T1::ElementType and \a T2::ElementType. Both
// dense vector types \a T1 and \a T2 as well as the two element types \a T1::ElementType
// and \a T2::ElementType have to be supported by the MultTrait class template.
*/
template< typename T1    // Type of the left-hand side dense vector
        , typename T2 >  // Type of the right-hand side dense vector
inline const DVecTDVecMultExpr<T1,T2>
   operator*( const DenseVector<T1,false>& lhs, const DenseVector<T2,true>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   return DVecTDVecMultExpr<T1,T2>( ~lhs, ~rhs );
}
//*************************************************************************************************




//=================================================================================================
//
//  EXPRESSION TRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename VT1, typename VT2, bool AF >
struct SubmatrixExprTrait< DVecTDVecMultExpr<VT1,VT2>, AF >
{
 public:
   //**********************************************************************************************
   typedef typename MultExprTrait< typename SubvectorExprTrait<const VT1,AF>::Type
                                 , typename SubvectorExprTrait<const VT2,AF>::Type >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename VT1, typename VT2 >
struct RowExprTrait< DVecTDVecMultExpr<VT1,VT2> >
{
 public:
   //**********************************************************************************************
   typedef typename MultExprTrait< typename VT1::ReturnType, VT2 >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename VT1, typename VT2 >
struct ColumnExprTrait< DVecTDVecMultExpr<VT1,VT2> >
{
 public:
   //**********************************************************************************************
   typedef typename MultExprTrait< VT1, typename VT2::ReturnType >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************

} // namespace blaze

#endif
