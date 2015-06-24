//=================================================================================================
/*!
//  \file blaze/math/expressions/DMatScalarDivExpr.h
//  \brief Header file for the dense matrix/scalar division expression
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

#ifndef _BLAZE_MATH_EXPRESSIONS_DMATSCALARDIVEXPR_H_
#define _BLAZE_MATH_EXPRESSIONS_DMATSCALARDIVEXPR_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <iterator>
#include <blaze/math/constraints/DenseMatrix.h>
#include <blaze/math/constraints/StorageOrder.h>
#include <blaze/math/expressions/Computation.h>
#include <blaze/math/expressions/DenseMatrix.h>
#include <blaze/math/expressions/Forward.h>
#include <blaze/math/expressions/MatScalarDivExpr.h>
#include <blaze/math/Intrinsics.h>
#include <blaze/math/shims/Serial.h>
#include <blaze/math/traits/ColumnExprTrait.h>
#include <blaze/math/traits/DivExprTrait.h>
#include <blaze/math/traits/DivTrait.h>
#include <blaze/math/traits/MultExprTrait.h>
#include <blaze/math/traits/MultTrait.h>
#include <blaze/math/traits/RowExprTrait.h>
#include <blaze/math/traits/SubmatrixExprTrait.h>
#include <blaze/math/typetraits/BaseElementType.h>
#include <blaze/math/typetraits/IsColumnMajorMatrix.h>
#include <blaze/math/typetraits/IsComputation.h>
#include <blaze/math/typetraits/IsDenseMatrix.h>
#include <blaze/math/typetraits/IsExpression.h>
#include <blaze/math/typetraits/IsMultExpr.h>
#include <blaze/math/typetraits/IsRowMajorMatrix.h>
#include <blaze/math/typetraits/IsTemporary.h>
#include <blaze/math/typetraits/RequiresEvaluation.h>
#include <blaze/system/Thresholds.h>
#include <blaze/util/Assert.h>
#include <blaze/util/constraints/FloatingPoint.h>
#include <blaze/util/constraints/Numeric.h>
#include <blaze/util/constraints/Reference.h>
#include <blaze/util/constraints/SameType.h>
#include <blaze/util/EnableIf.h>
#include <blaze/util/InvalidType.h>
#include <blaze/util/logging/FunctionTrace.h>
#include <blaze/util/SelectType.h>
#include <blaze/util/Types.h>
#include <blaze/util/typetraits/IsFloatingPoint.h>
#include <blaze/util/typetraits/IsNumeric.h>
#include <blaze/util/typetraits/IsSame.h>


namespace blaze {

//=================================================================================================
//
//  CLASS DMATSCALARDIVEXPR
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Expression object for divisions of a dense matrix by a scalar.
// \ingroup dense_matrix_expression
//
// The DMatScalarDivExpr class represents the compile time expression for divisions of dense
// matrices and by scalar values.
*/
template< typename MT  // Type of the left-hand side dense matrix
        , typename ST  // Type of the right-hand side scalar value
        , bool SO >    // Storage order
class DMatScalarDivExpr : public DenseMatrix< DMatScalarDivExpr<MT,ST,SO>, SO >
                        , private MatScalarDivExpr
                        , private Computation
{
 private:
   //**Type definitions****************************************************************************
   typedef typename MT::ResultType     RT;  //!< Result type of the dense matrix expression.
   typedef typename MT::ReturnType     RN;  //!< Return type of the dense matrix expression.
   typedef typename MT::ElementType    ET;  //!< Element type of the dense matrix expression.
   typedef typename MT::CompositeType  CT;  //!< Composite type of the dense matrix expression.
   //**********************************************************************************************

   //**Return type evaluation**********************************************************************
   //! Compilation switch for the selection of the subscript operator return type.
   /*! The \a returnExpr compile time constant expression is a compilation switch for the
       selection of the \a ReturnType. If the matrix operand returns a temporary vector
       or matrix, \a returnExpr will be set to \a false and the subscript operator will
       return it's result by value. Otherwise \a returnExpr will be set to \a true and
       the subscript operator may return it's result as an expression. */
   enum { returnExpr = !IsTemporary<RN>::value };

   //! Expression return type for the subscript operator.
   typedef typename DivExprTrait<RN,ST>::Type  ExprReturnType;
   //**********************************************************************************************

   //**Serial evaluation strategy******************************************************************
   //! Compilation switch for the serial evaluation strategy of the division expression.
   /*! The \a useAssign compile time constant expression represents a compilation switch for
       the serial evaluation strategy of the division expression. In case the given dense
       matrix expression of type \a MT is a computation expression and requires an intermediate
       evaluation, \a useAssign will be set to 1 and the division expression will be evaluated
       via the \a assign function family. Otherwise \a useAssign will be set to 0 and the
       expression will be evaluated via the subscript operator. */
   enum { useAssign = IsComputation<MT>::value && RequiresEvaluation<MT>::value };

   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename MT2 >
   struct UseAssign {
      enum { value = useAssign };
   };
   /*! \endcond */
   //**********************************************************************************************

   //**Parallel evaluation strategy****************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   /*! The UseSMPAssign struct is a helper struct for the selection of the parallel evaluation
       strategy. In case either the target matrix or the dense matrix operand is not SMP assignable
       and the matrix operand is a computation expression that requires an intermediate evaluation,
       \a value is set to 1 and the expression specific evaluation strategy is selected. Otherwise
       \a value is set to 0 and the default strategy is chosen. */
   template< typename MT2 >
   struct UseSMPAssign {
      enum { value = ( !MT2::smpAssignable || !MT::smpAssignable ) && useAssign };
   };
   /*! \endcond */
   //**********************************************************************************************

 public:
   //**Type definitions****************************************************************************
   typedef DMatScalarDivExpr<MT,ST,SO>                 This;           //!< Type of this DMatScalarDivExpr instance.
   typedef typename DivTrait<RT,ST>::Type              ResultType;     //!< Result type for expression template evaluations.
   typedef typename ResultType::OppositeType           OppositeType;   //!< Result type with opposite storage order for expression template evaluations.
   typedef typename ResultType::TransposeType          TransposeType;  //!< Transpose type for expression template evaluations.
   typedef typename ResultType::ElementType            ElementType;    //!< Resulting element type.
   typedef typename IntrinsicTrait<ElementType>::Type  IntrinsicType;  //!< Resulting intrinsic element type.

   //! Return type for expression template evaluations.
   typedef const typename SelectType< returnExpr, ExprReturnType, ElementType >::Type  ReturnType;

   //! Data type for composite expression templates.
   typedef typename SelectType< useAssign, const ResultType, const DMatScalarDivExpr& >::Type  CompositeType;

   //! Composite type of the left-hand side dense matrix expression.
   typedef typename SelectType< IsExpression<MT>::value, const MT, const MT& >::Type  LeftOperand;

   //! Composite type of the right-hand side scalar value.
   typedef ST  RightOperand;
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

      //! ConstIterator type of the dense matrix expression.
      typedef typename MT::ConstIterator  IteratorType;
      //*******************************************************************************************

      //**Constructor******************************************************************************
      /*!\brief Constructor for the ConstIterator class.
      //
      // \param iterator Iterator to the initial element.
      // \param scalar Scalar of the multiplication expression.
      */
      explicit inline ConstIterator( IteratorType iterator, RightOperand scalar )
         : iterator_( iterator )  // Iterator to the current element
         , scalar_  ( scalar   )  // Scalar of the multiplication expression
      {}
      //*******************************************************************************************

      //**Addition assignment operator*************************************************************
      /*!\brief Addition assignment operator.
      //
      // \param inc The increment of the iterator.
      // \return The incremented iterator.
      */
      inline ConstIterator& operator+=( size_t inc ) {
         iterator_ += inc;
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
         iterator_ -= dec;
         return *this;
      }
      //*******************************************************************************************

      //**Prefix increment operator****************************************************************
      /*!\brief Pre-increment operator.
      //
      // \return Reference to the incremented iterator.
      */
      inline ConstIterator& operator++() {
         ++iterator_;
         return *this;
      }
      //*******************************************************************************************

      //**Postfix increment operator***************************************************************
      /*!\brief Post-increment operator.
      //
      // \return The previous position of the iterator.
      */
      inline const ConstIterator operator++( int ) {
         return ConstIterator( iterator_++ );
      }
      //*******************************************************************************************

      //**Prefix decrement operator****************************************************************
      /*!\brief Pre-decrement operator.
      //
      // \return Reference to the decremented iterator.
      */
      inline ConstIterator& operator--() {
         --iterator_;
         return *this;
      }
      //*******************************************************************************************

      //**Postfix decrement operator***************************************************************
      /*!\brief Post-decrement operator.
      //
      // \return The previous position of the iterator.
      */
      inline const ConstIterator operator--( int ) {
         return ConstIterator( iterator_-- );
      }
      //*******************************************************************************************

      //**Element access operator******************************************************************
      /*!\brief Direct access to the element at the current iterator position.
      //
      // \return The resulting value.
      */
      inline ReturnType operator*() const {
         return *iterator_ / scalar_;
      }
      //*******************************************************************************************

      //**Load function****************************************************************************
      /*!\brief Access to the intrinsic elements of the matrix.
      //
      // \return The resulting intrinsic value.
      */
      inline IntrinsicType load() const {
         return iterator_.load() / set( scalar_ );
      }
      //*******************************************************************************************

      //**Equality operator************************************************************************
      /*!\brief Equality comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the iterators refer to the same element, \a false if not.
      */
      inline bool operator==( const ConstIterator& rhs ) const {
         return iterator_ == rhs.iterator_;
      }
      //*******************************************************************************************

      //**Inequality operator**********************************************************************
      /*!\brief Inequality comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the iterators don't refer to the same element, \a false if they do.
      */
      inline bool operator!=( const ConstIterator& rhs ) const {
         return iterator_ != rhs.iterator_;
      }
      //*******************************************************************************************

      //**Less-than operator***********************************************************************
      /*!\brief Less-than comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the left-hand side iterator is smaller, \a false if not.
      */
      inline bool operator<( const ConstIterator& rhs ) const {
         return iterator_ < rhs.iterator_;
      }
      //*******************************************************************************************

      //**Greater-than operator********************************************************************
      /*!\brief Greater-than comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the left-hand side iterator is greater, \a false if not.
      */
      inline bool operator>( const ConstIterator& rhs ) const {
         return iterator_ > rhs.iterator_;
      }
      //*******************************************************************************************

      //**Less-or-equal-than operator**************************************************************
      /*!\brief Less-than comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the left-hand side iterator is smaller or equal, \a false if not.
      */
      inline bool operator<=( const ConstIterator& rhs ) const {
         return iterator_ <= rhs.iterator_;
      }
      //*******************************************************************************************

      //**Greater-or-equal-than operator***********************************************************
      /*!\brief Greater-than comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side iterator.
      // \return \a true if the left-hand side iterator is greater or equal, \a false if not.
      */
      inline bool operator>=( const ConstIterator& rhs ) const {
         return iterator_ >= rhs.iterator_;
      }
      //*******************************************************************************************

      //**Subtraction operator*********************************************************************
      /*!\brief Calculating the number of elements between two iterators.
      //
      // \param rhs The right-hand side iterator.
      // \return The number of elements between the two iterators.
      */
      inline DifferenceType operator-( const ConstIterator& rhs ) const {
         return iterator_ - rhs.iterator_;
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
         return ConstIterator( it.iterator_ + inc );
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
         return ConstIterator( it.iterator_ + inc );
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
         return ConstIterator( it.iterator_ - dec );
      }
      //*******************************************************************************************

    private:
      //**Member variables*************************************************************************
      IteratorType iterator_;  //!< Iterator to the current element.
      RightOperand scalar_;    //!< Scalar of the multiplication expression.
      //*******************************************************************************************
   };
   //**********************************************************************************************

   //**Compilation flags***************************************************************************
   //! Compilation switch for the expression template evaluation strategy.
   enum { vectorizable = MT::vectorizable &&
                         IsSame<ET,RightOperand>::value &&
                         IntrinsicTrait<ET>::division };

   //! Compilation switch for the expression template assignment strategy.
   enum { smpAssignable = MT::smpAssignable };
   //**********************************************************************************************

   //**Constructor*********************************************************************************
   /*!\brief Constructor for the DMatScalarDivExpr class.
   //
   // \param matrix The left-hand side dense matrix of the division expression.
   // \param scalar The right-hand side scalar of the division expression.
   */
   explicit inline DMatScalarDivExpr( const MT& matrix, ST scalar )
      : matrix_( matrix )  // Left-hand side dense matrix of the division expression
      , scalar_( scalar )  // Right-hand side scalar of the division expression
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
      BLAZE_INTERNAL_ASSERT( i < matrix_.rows()   , "Invalid row access index"    );
      BLAZE_INTERNAL_ASSERT( j < matrix_.columns(), "Invalid column access index" );
      return matrix_(i,j) / scalar_;
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
      BLAZE_INTERNAL_ASSERT( i < matrix_.rows()   , "Invalid row access index"    );
      BLAZE_INTERNAL_ASSERT( j < matrix_.columns(), "Invalid column access index" );
      BLAZE_INTERNAL_ASSERT( !SO || ( i % IT::size == 0UL ), "Invalid row access index"    );
      BLAZE_INTERNAL_ASSERT( SO  || ( j % IT::size == 0UL ), "Invalid column access index" );
      const IntrinsicType xmm1( matrix_.load(i,j) );
      const IntrinsicType xmm2( set( scalar_ ) );
      return xmm1 / xmm2;
   }
   //**********************************************************************************************

   //**Begin function******************************************************************************
   /*!\brief Returns an iterator to the first non-zero element of row \a i.
   //
   // \param i The row index.
   // \return Iterator to the first non-zero element of row \a i.
   */
   inline ConstIterator begin( size_t i ) const {
      return ConstIterator( matrix_.begin(i), scalar_ );
   }
   //**********************************************************************************************

   //**End function********************************************************************************
   /*!\brief Returns an iterator just past the last non-zero element of row \a i.
   //
   // \param i The row index.
   // \return Iterator just past the last non-zero element of row \a i.
   */
   inline ConstIterator end( size_t i ) const {
      return ConstIterator( matrix_.end(i), scalar_ );
   }
   //**********************************************************************************************

   //**Rows function*******************************************************************************
   /*!\brief Returns the current number of rows of the matrix.
   //
   // \return The number of rows of the matrix.
   */
   inline size_t rows() const {
      return matrix_.rows();
   }
   //**********************************************************************************************

   //**Columns function****************************************************************************
   /*!\brief Returns the current number of columns of the matrix.
   //
   // \return The number of columns of the matrix.
   */
   inline size_t columns() const {
      return matrix_.columns();
   }
   //**********************************************************************************************

   //**Left operand access*************************************************************************
   /*!\brief Returns the left-hand side dense matrix operand.
   //
   // \return The left-hand side dense matrix operand.
   */
   inline LeftOperand leftOperand() const {
      return matrix_;
   }
   //**********************************************************************************************

   //**Right operand access************************************************************************
   /*!\brief Returns the right-hand side scalar operand.
   //
   // \return The right-hand side scalar operand.
   */
   inline RightOperand rightOperand() const {
      return scalar_;
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
      return IsComputation<MT>::value && matrix_.canAlias( alias );
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
      return matrix_.isAliased( alias );
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the operands of the expression are properly aligned in memory.
   //
   // \return \a true in case the operands are aligned, \a false if not.
   */
   inline bool isAligned() const {
      return matrix_.isAligned();
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the expression can be used in SMP assignments.
   //
   // \return \a true in case the expression can be used in SMP assignments, \a false if not.
   */
   inline bool canSMPAssign() const {
      return matrix_.canSMPAssign() ||
             ( ( ( SO == rowMajor ) ? rows() : columns() ) > SMP_DMATSCALARMULT_THRESHOLD );
   }
   //**********************************************************************************************

 private:
   //**Member variables****************************************************************************
   LeftOperand  matrix_;  //!< Left-hand side dense matrix of the division expression.
   RightOperand scalar_;  //!< Right-hand side scalar of the division expression.
   //**********************************************************************************************

   //**Assignment to dense matrices****************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a dense matrix-scalar division to a dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a dense matrix-scalar
   // division expression to a dense matrix. Due to the explicit application of the SFINAE
   // principle, this operator can only be selected by the compiler in case the matrix
   // operand is a computation expression and requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      assign( DenseMatrix<MT2,SO2>& lhs, const DMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      assign( ~lhs, rhs.matrix_ );
      assign( ~lhs, (~lhs) / rhs.scalar_ );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Assignment to sparse matrices***************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a dense matrix-scalar division to a sparse matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side division expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a dense matrix-scalar
   // division expression to a sparse matrix. Due to the explicit application of the SFINAE
   // principle, this operator can only be selected by the compiler in case the matrix
   // operand is a computation expression and requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target sparse matrix
           , bool SO2 >    // Storage order of the target sparse matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      assign( SparseMatrix<MT2,SO2>& lhs, const DMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      assign( ~lhs, rhs.matrix_ );
      (~lhs) /= rhs.scalar_;
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Addition assignment to dense matrices*******************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Addition assignment of a dense matrix-scalar division to a dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be added.
   // \return void
   //
   // This function implements the performance optimized addition assignment of a dense matrix-
   // scalar division expression to a dense matrix. Due to the explicit application of the
   // SFINAE principle, this operator can only be selected by the compiler in case the matrix
   // operand is a computation expression and requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      addAssign( DenseMatrix<MT2,SO2>& lhs, const DMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_CONSTRAINT_MUST_BE_DENSE_MATRIX_TYPE( ResultType );
      BLAZE_CONSTRAINT_MUST_BE_MATRIX_WITH_STORAGE_ORDER( ResultType, SO );
      BLAZE_CONSTRAINT_MUST_BE_REFERENCE_TYPE( typename ResultType::CompositeType );

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      const ResultType tmp( serial( rhs ) );
      addAssign( ~lhs, tmp );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Addition assignment to sparse matrices******************************************************
   // No special implementation for the addition assignment to sparse matrices.
   //**********************************************************************************************

   //**Subtraction assignment to dense matrices****************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Subtraction assignment of a dense matrix-scalar division to a dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized subtraction assignment of a dense matrix-
   // scalar division expression to a dense matrix. Due to the explicit application of the SFINAE
   // principle, this operator can only be selected by the compiler in case the matrix operand is
   // a computation expression and requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      subAssign( DenseMatrix<MT2,SO2>& lhs, const DMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_CONSTRAINT_MUST_BE_DENSE_MATRIX_TYPE( ResultType );
      BLAZE_CONSTRAINT_MUST_BE_MATRIX_WITH_STORAGE_ORDER( ResultType, SO );
      BLAZE_CONSTRAINT_MUST_BE_REFERENCE_TYPE( typename ResultType::CompositeType );

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      const ResultType tmp( serial( rhs ) );
      subAssign( ~lhs, tmp );
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
   /*!\brief SMP assignment of a dense matrix-scalar division to a dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized SMP assignment of a dense matrix-scalar
   // division expression to a dense matrix. Due to the explicit application of the SFINAE
   // principle, this operator can only be selected by the compiler in case the expression
   // specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpAssign( DenseMatrix<MT2,SO2>& lhs, const DMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      smpAssign( ~lhs, rhs.matrix_ );
      smpAssign( ~lhs, (~lhs) / rhs.scalar_ );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP assignment to sparse matrices***********************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP assignment of a dense matrix-scalar division to a sparse matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side division expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized SMP assignment of a dense matrix-scalar
   // division expression to a sparse matrix. Due to the explicit application of the SFINAE
   // principle, this operator can only be selected by the compiler in case the expression
   // specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target sparse matrix
           , bool SO2 >    // Storage order of the target sparse matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpAssign( SparseMatrix<MT2,SO2>& lhs, const DMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      smpAssign( ~lhs, rhs.matrix_ );
      (~lhs) /= rhs.scalar_;
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP addition assignment to dense matrices***************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP addition assignment of a dense matrix-scalar division to a dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be added.
   // \return void
   //
   // This function implements the performance optimized SMP addition assignment of a dense
   // matrix-scalar division expression to a dense matrix. Due to the explicit application
   // of the SFINAE principle, this operator can only be selected by the compiler in case
   // the expression specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpAddAssign( DenseMatrix<MT2,SO2>& lhs, const DMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_CONSTRAINT_MUST_BE_DENSE_MATRIX_TYPE( ResultType );
      BLAZE_CONSTRAINT_MUST_BE_MATRIX_WITH_STORAGE_ORDER( ResultType, SO );
      BLAZE_CONSTRAINT_MUST_BE_REFERENCE_TYPE( typename ResultType::CompositeType );

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      const ResultType tmp( rhs );
      smpAddAssign( ~lhs, tmp );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP addition assignment to sparse matrices**************************************************
   // No special implementation for the SMP addition assignment to sparse matrices.
   //**********************************************************************************************

   //**SMP subtraction assignment to dense matrices************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP subtraction assignment of a dense matrix-scalar division to a dense matrix.
   // \ingroup dense_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized SMP subtraction assignment of a dense
   // matrix-scalar division expression to a dense matrix. Due to the explicit application of
   // the SFINAE principle, this operator can only be selected by the compiler in case the
   // expression specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpSubAssign( DenseMatrix<MT2,SO2>& lhs, const DMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_CONSTRAINT_MUST_BE_DENSE_MATRIX_TYPE( ResultType );
      BLAZE_CONSTRAINT_MUST_BE_MATRIX_WITH_STORAGE_ORDER( ResultType, SO );
      BLAZE_CONSTRAINT_MUST_BE_REFERENCE_TYPE( typename ResultType::CompositeType );

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      const ResultType tmp( rhs );
      smpSubAssign( ~lhs, tmp );
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
   BLAZE_CONSTRAINT_MUST_BE_DENSE_MATRIX_TYPE( MT );
   BLAZE_CONSTRAINT_MUST_BE_MATRIX_WITH_STORAGE_ORDER( MT, SO );
   BLAZE_CONSTRAINT_MUST_BE_NUMERIC_TYPE( ST );
   BLAZE_CONSTRAINT_MUST_NOT_BE_FLOATING_POINT_TYPE( ST );
   BLAZE_CONSTRAINT_MUST_NOT_BE_FLOATING_POINT_TYPE( ElementType );
   BLAZE_CONSTRAINT_MUST_BE_SAME_TYPE( ST, RightOperand );
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
/*!\brief Division operator for the division of a dense matrix by a scalar value (\f$ A=B/s \f$).
// \ingroup dense_matrix
//
// \param mat The left-hand side dense matrix for the division.
// \param scalar The right-hand side scalar value for the division.
// \return The scaled result matrix.
//
// This operator represents the division of a dense matrix by a scalar value:

   \code
   blaze::DynamicMatrix<double> A, B;
   // ... Resizing and initialization
   B = A / 0.24;
   \endcode

// The operator returns an expression representing a dense matrix of the higher-order element
// type of the involved data types \a T1::ElementType and \a T2. Note that this operator only
// works for scalar values of built-in data type.
//
// \b Note: A division by zero is only checked by an user assert.
*/
template< typename T1    // Type of the left-hand side dense matrix
        , bool SO        // Storage order of the left-hand side dense matrix
        , typename T2 >  // Type of the right-hand side scalar
inline const typename EnableIf< IsNumeric<T2>, typename DivExprTrait<T1,T2>::Type >::Type
   operator/( const DenseMatrix<T1,SO>& mat, T2 scalar )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_USER_ASSERT( scalar != T2(0), "Division by zero detected" );

   typedef typename DivExprTrait<T1,T2>::Type  ReturnType;
   typedef typename ReturnType::RightOperand   ScalarType;

   if( IsMultExpr<ReturnType>::value ) {
      return ReturnType( ~mat, ScalarType(1)/ScalarType(scalar) );
   }
   else {
      return ReturnType( ~mat, scalar );
   }
}
//*************************************************************************************************




//=================================================================================================
//
//  GLOBAL RESTRUCTURING BINARY ARITHMETIC OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Multiplication operator for the multiplication of a dense matrix-scalar division
//        expression and a scalar value (\f$ A=(B/s1)*s2 \f$).
// \ingroup dense_matrix
//
// \param mat The left-hand side dense matrix-scalar division.
// \param scalar The right-hand side scalar value for the multiplication.
// \return The scaled result matrix.
//
// This operator implements a performance optimized treatment of the multiplication of a
// dense matrix-scalar division expression and a scalar value.
*/
template< typename MT     // Type of the dense matrix of the left-hand side expression
        , typename ST1    // Type of the scalar of the left-hand side expression
        , bool SO         // Storage order of the dense matrix
        , typename ST2 >  // Type of the right-hand side scalar
inline const typename EnableIf< IsFloatingPoint< typename DivTrait<ST2,ST1>::Type >
                              , typename MultExprTrait< DMatScalarDivExpr<MT,ST1,SO>, ST2 >::Type >::Type
   operator*( const DMatScalarDivExpr<MT,ST1,SO>& mat, ST2 scalar )
{
   BLAZE_FUNCTION_TRACE;

   return mat.leftOperand() * ( scalar / mat.rightOperand() );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Multiplication operator for the multiplication of a scalar value and a dense matrix-
//        scalar division expression (\f$ A=s2*(B/s1) \f$).
// \ingroup dense_matrix
//
// \param scalar The left-hand side scalar value for the multiplication.
// \param mat The right-hand side dense matrix-scalar division.
// \return The scaled result matrix.
//
// This operator implements a performance optimized treatment of the multiplication of a
// scalar value and a dense matrix-scalar division expression.
*/
template< typename ST1  // Type of the left-hand side scalar
        , typename MT   // Type of the dense matrix of the right-hand side expression
        , typename ST2  // Type of the scalar of the right-hand side expression
        , bool SO >     // Storage order of the dense matrix
inline const typename EnableIf< IsFloatingPoint< typename DivTrait<ST1,ST2>::Type >
                              , typename MultExprTrait< ST1, DMatScalarDivExpr<MT,ST2,SO> >::Type >::Type
   operator*( ST1 scalar, const DMatScalarDivExpr<MT,ST2,SO>& mat )
{
   BLAZE_FUNCTION_TRACE;

   return mat.leftOperand() * ( scalar / mat.rightOperand() );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Division operator for the division of a dense matrix-scalar division expression
//        and a scalar value (\f$ A=(B/s1)/s2 \f$).
// \ingroup dense_matrix
//
// \param mat The left-hand side dense matrix-scalar division.
// \param scalar The right-hand side scalar value for the division.
// \return The scaled result matrix.
//
// This operator implements a performance optimized treatment of the division of a dense
// matrix-scalar division expression and a scalar value.
*/
template< typename MT     // Type of the dense matrix of the left-hand side expression
        , typename ST1    // Type of the scalar of the left-hand side expression
        , bool SO         // Storage order of the dense matrix
        , typename ST2 >  // Type of the right-hand side scalar
inline const typename EnableIf< IsNumeric<ST2>
                              , typename DivExprTrait<MT,typename MultTrait<ST1,ST2>::Type>::Type >::Type
   operator/( const DMatScalarDivExpr<MT,ST1,SO>& mat, ST2 scalar )
{
   BLAZE_FUNCTION_TRACE;

   BLAZE_USER_ASSERT( scalar != ST2(0), "Division by zero detected" );

   typedef typename MultTrait<ST1,ST2>::Type         MultType;
   typedef typename DivExprTrait<MT,MultType>::Type  ReturnType;
   typedef typename ReturnType::RightOperand         ScalarType;

   if( IsMultExpr<ReturnType>::value ) {
      return ReturnType( mat.leftOperand(), ScalarType(1)/( mat.rightOperand() * scalar ) );
   }
   else {
      return ReturnType( mat.leftOperand(), mat.rightOperand() * scalar );
   }
}
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  DMATSCALARMULTEXPRTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, typename ST1, typename ST2 >
struct DMatScalarMultExprTrait< DMatScalarDivExpr<MT,ST1,false>, ST2 >
{
 private:
   //**********************************************************************************************
   enum { condition = IsFloatingPoint<typename DivTrait<ST1,ST2>::Type>::value };
   //**********************************************************************************************

   //**********************************************************************************************
   typedef typename DMatScalarMultExprTrait<MT,typename DivTrait<ST1,ST2>::Type>::Type  T1;
   typedef DMatScalarMultExpr< DMatScalarDivExpr<MT,ST1,false>, ST2, false >            T2;
   //**********************************************************************************************

 public:
   //**********************************************************************************************
   typedef typename SelectType< IsDenseMatrix<MT>::value && IsRowMajorMatrix<MT>::value &&
                                IsNumeric<ST1>::value && IsNumeric<ST2>::value
                              , typename SelectType<condition,T1,T2>::Type
                              , INVALID_TYPE >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  TDMATSCALARMULTEXPRTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, typename ST1, typename ST2 >
struct TDMatScalarMultExprTrait< DMatScalarDivExpr<MT,ST1,true>, ST2 >
{
 private:
   //**********************************************************************************************
   enum { condition = IsFloatingPoint<typename DivTrait<ST1,ST2>::Type>::value };
   //**********************************************************************************************

   //**********************************************************************************************
   typedef typename DMatScalarMultExprTrait<MT,typename DivTrait<ST1,ST2>::Type>::Type  T1;
   typedef DMatScalarMultExpr< DMatScalarDivExpr<MT,ST1,true>, ST2, true >              T2;
   //**********************************************************************************************

 public:
   //**********************************************************************************************
   typedef typename SelectType< IsDenseMatrix<MT>::value && IsColumnMajorMatrix<MT>::value &&
                                IsNumeric<ST1>::value && IsNumeric<ST2>::value
                              , typename SelectType<condition,T1,T2>::Type
                              , INVALID_TYPE >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  SUBMATRIXEXPRTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, typename ST, bool SO, bool AF >
struct SubmatrixExprTrait< DMatScalarDivExpr<MT,ST,SO>, AF >
{
 public:
   //**********************************************************************************************
   typedef typename DivExprTrait< typename SubmatrixExprTrait<const MT,AF>::Type, ST >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  ROWEXPRTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, typename ST, bool SO >
struct RowExprTrait< DMatScalarDivExpr<MT,ST,SO> >
{
 public:
   //**********************************************************************************************
   typedef typename DivExprTrait< typename RowExprTrait<const MT>::Type, ST >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  COLUMNEXPRTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, typename ST, bool SO >
struct ColumnExprTrait< DMatScalarDivExpr<MT,ST,SO> >
{
 public:
   //**********************************************************************************************
   typedef typename DivExprTrait< typename ColumnExprTrait<const MT>::Type, ST >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************

} // namespace blaze

#endif
