//=================================================================================================
/*!
//  \file blaze/math/expressions/SMatScalarDivExpr.h
//  \brief Header file for the sparse matrix/scalar division expression
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

#ifndef _BLAZE_MATH_EXPRESSIONS_SMATSCALARDIVEXPR_H_
#define _BLAZE_MATH_EXPRESSIONS_SMATSCALARDIVEXPR_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <iterator>
#include <blaze/math/constraints/SparseMatrix.h>
#include <blaze/math/constraints/StorageOrder.h>
#include <blaze/math/expressions/Computation.h>
#include <blaze/math/expressions/Forward.h>
#include <blaze/math/expressions/MatScalarDivExpr.h>
#include <blaze/math/expressions/SparseMatrix.h>
#include <blaze/math/shims/Serial.h>
#include <blaze/math/sparse/ValueIndexPair.h>
#include <blaze/math/traits/ColumnExprTrait.h>
#include <blaze/math/traits/DivExprTrait.h>
#include <blaze/math/traits/DivTrait.h>
#include <blaze/math/traits/MultExprTrait.h>
#include <blaze/math/traits/MultTrait.h>
#include <blaze/math/traits/RowExprTrait.h>
#include <blaze/math/traits/SubmatrixExprTrait.h>
#include <blaze/math/typetraits/BaseElementType.h>
#include <blaze/math/typetraits/IsComputation.h>
#include <blaze/math/typetraits/IsColumnMajorMatrix.h>
#include <blaze/math/typetraits/IsExpression.h>
#include <blaze/math/typetraits/IsMultExpr.h>
#include <blaze/math/typetraits/IsRowMajorMatrix.h>
#include <blaze/math/typetraits/IsSparseMatrix.h>
#include <blaze/math/typetraits/IsTemporary.h>
#include <blaze/math/typetraits/RequiresEvaluation.h>
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
#include <blaze/util/typetraits/RemoveReference.h>


namespace blaze {

//=================================================================================================
//
//  CLASS SMATSCALARDIVEXPR
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Expression object for sparse matrix-scalar divisions.
// \ingroup sparse_matrix_expression
//
// The SMatScalarMult class represents the compile time expression for divisions between
// a sparse matrix and a scalar value.
*/
template< typename MT  // Type of the left-hand side sparse matrix
        , typename ST  // Type of the right-hand side scalar value
        , bool SO >    // Storage order
class SMatScalarDivExpr : public SparseMatrix< SMatScalarDivExpr<MT,ST,SO>, SO >
                        , private MatScalarDivExpr
                        , private Computation
{
 private:
   //**Type definitions****************************************************************************
   typedef typename MT::ResultType     RT;  //!< Result type of the sparse matrix expression.
   typedef typename MT::ReturnType     RN;  //!< Return type of the sparse matrix expression.
   typedef typename MT::CompositeType  CT;  //!< Composite type of the sparse matrix expression.
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
       the serial evaluation strategy of the division expression. In case the given sparse
       matrix expression of type \a MT requires an intermediate evaluation, \a useAssign will
       be set to 1 and the division expression will be evaluated via the \a assign function
       family. Otherwise Otherwise \a useAssign will be set to 0 and the expression will be
       evaluated via the subscript operator. */
   enum { useAssign = RequiresEvaluation<MT>::value };

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
       strategy. In case either the target matrix or the sparse matrix operand is not SMP
       assignable and the matrix operand requires an intermediate evaluation, \a value is set
       to 1 and the expression specific evaluation strategy is selected. Otherwise \a value is
       set to 0 and the default strategy is chosen. */
   template< typename MT2 >
   struct UseSMPAssign {
      enum { value = ( !MT2::smpAssignable || !MT::smpAssignable ) && useAssign };
   };
   /*! \endcond */
   //**********************************************************************************************

 public:
   //**Type definitions****************************************************************************
   typedef SMatScalarDivExpr<MT,ST,SO>         This;           //!< Type of this SMatScalarDivExpr instance.
   typedef typename MultTrait<RT,ST>::Type     ResultType;     //!< Result type for expression template evaluations.
   typedef typename ResultType::OppositeType   OppositeType;   //!< Result type with opposite storage order for expression template evaluations.
   typedef typename ResultType::TransposeType  TransposeType;  //!< Transpose type for expression template evaluations.
   typedef typename ResultType::ElementType    ElementType;    //!< Resulting element type.

   //! Return type for expression template evaluations.
   typedef const typename SelectType< returnExpr, ExprReturnType, ElementType >::Type  ReturnType;

   //! Data type for composite expression templates.
   typedef typename SelectType< useAssign, const ResultType, const SMatScalarDivExpr& >::Type  CompositeType;

   //! Composite data type of the sparse matrix expression.
   typedef typename SelectType< IsExpression<MT>::value, const MT, const MT& >::Type  LeftOperand;

   //! Composite type of the right-hand side scalar value.
   typedef ST  RightOperand;
   //**********************************************************************************************

   //**ConstIterator class definition**************************************************************
   /*!\brief Iterator over the elements of the sparse matrix/scalar division expression.
   */
   class ConstIterator
   {
    public:
      //**Type definitions*************************************************************************
      //! Element type of the sparse matrix expression.
      typedef ValueIndexPair<ElementType>  Element;

      //! Iterator type of the sparse matrix expression.
      typedef typename RemoveReference<LeftOperand>::Type::ConstIterator  IteratorType;

      typedef std::forward_iterator_tag  IteratorCategory;  //!< The iterator category.
      typedef Element                    ValueType;         //!< Type of the underlying pointers.
      typedef ValueType*                 PointerType;       //!< Pointer return type.
      typedef ValueType&                 ReferenceType;     //!< Reference return type.
      typedef ptrdiff_t                  DifferenceType;    //!< Difference between two iterators.

      // STL iterator requirements
      typedef IteratorCategory  iterator_category;  //!< The iterator category.
      typedef ValueType         value_type;         //!< Type of the underlying pointers.
      typedef PointerType       pointer;            //!< Pointer return type.
      typedef ReferenceType     reference;          //!< Reference return type.
      typedef DifferenceType    difference_type;    //!< Difference between two iterators.
      //*******************************************************************************************

      //**Constructor******************************************************************************
      /*!\brief Constructor for the ConstIterator class.
      */
      inline ConstIterator( IteratorType matrix, RightOperand scalar )
         : matrix_( matrix )  // Iterator over the elements of the left-hand side sparse matrix expression
         , scalar_( scalar )  // Right-hand side scalar of the division expression
      {}
      //*******************************************************************************************

      //**Prefix increment operator****************************************************************
      /*!\brief Pre-increment operator.
      //
      // \return Reference to the incremented expression iterator.
      */
      inline ConstIterator& operator++() {
         ++matrix_;
         return *this;
      }
      //*******************************************************************************************

      //**Element access operator******************************************************************
      /*!\brief Direct access to the sparse matrix element at the current iterator position.
      //
      // \return The element at the current iterator position.
      */
      inline const Element operator*() const {
         return Element( matrix_->value() / scalar_, matrix_->index() );
      }
      //*******************************************************************************************

      //**Element access operator******************************************************************
      /*!\brief Direct access to the sparse matrix element at the current iterator position.
      //
      // \return Reference to the sparse matrix element at the current iterator position.
      */
      inline const ConstIterator* operator->() const {
         return this;
      }
      //*******************************************************************************************

      //**Value function***************************************************************************
      /*!\brief Access to the current value of the sparse element.
      //
      // \return The current value of the sparse element.
      */
      inline ReturnType value() const {
         return matrix_->value() / scalar_;
      }
      //*******************************************************************************************

      //**Index function***************************************************************************
      /*!\brief Access to the current index of the sparse element.
      //
      // \return The current index of the sparse element.
      */
      inline size_t index() const {
         return matrix_->index();
      }
      //*******************************************************************************************

      //**Equality operator************************************************************************
      /*!\brief Equality comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side expression iterator.
      // \return \a true if the iterators refer to the same element, \a false if not.
      */
      inline bool operator==( const ConstIterator& rhs ) const {
         return matrix_ == rhs.matrix_;
      }
      //*******************************************************************************************

      //**Inequality operator**********************************************************************
      /*!\brief Inequality comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side expression iterator.
      // \return \a true if the iterators don't refer to the same element, \a false if they do.
      */
      inline bool operator!=( const ConstIterator& rhs ) const {
         return matrix_ != rhs.matrix_;
      }
      //*******************************************************************************************

      //**Subtraction operator*********************************************************************
      /*!\brief Calculating the number of elements between two expression iterators.
      //
      // \param rhs The right-hand side expression iterator.
      // \return The number of elements between the two expression iterators.
      */
      inline DifferenceType operator-( const ConstIterator& rhs ) const {
         return matrix_ - rhs.matrix_;
      }
      //*******************************************************************************************

    private:
      //**Member variables*************************************************************************
      IteratorType matrix_;  //!< Iterator over the elements of the left-hand side sparse matrix expression.
      RightOperand scalar_;  //!< Right-hand side scalar of the division expression.
      //*******************************************************************************************
   };
   //**********************************************************************************************

   //**Compilation flags***************************************************************************
   //! Compilation switch for the expression template assignment strategy.
   enum { smpAssignable = 0 };
   //**********************************************************************************************

   //**Constructor*********************************************************************************
   /*!\brief Constructor for the SMatScalarDivExpr class.
   //
   // \param matrix The left-hand side sparse matrix of the division expression.
   // \param scalar The right-hand side scalar of the division expression.
   */
   explicit inline SMatScalarDivExpr( const MT& matrix, ST scalar )
      : matrix_( matrix )  // Left-hand side sparse matrix of the division expression
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

   //**NonZeros function***************************************************************************
   /*!\brief Returns the number of non-zero elements in the sparse matrix.
   //
   // \return The number of non-zero elements in the sparse matrix.
   */
   inline size_t nonZeros() const {
      return matrix_.nonZeros();
   }
   //**********************************************************************************************

   //**NonZeros function***************************************************************************
   /*!\brief Returns the number of non-zero elements in the specified row.
   //
   // \param i The index of the row.
   // \return The number of non-zero elements of row \a i.
   */
   inline size_t nonZeros( size_t i ) const {
      return matrix_.nonZeros(i);
   }
   //**********************************************************************************************

   //**Left operand access*************************************************************************
   /*!\brief Returns the left-hand side sparse matrix operand.
   //
   // \return The left-hand side sparse matrix operand.
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
      return matrix_.canAlias( alias );
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

 private:
   //**Member variables****************************************************************************
   LeftOperand  matrix_;  //!< Left-hand side sparse matrix of the division expression.
   RightOperand scalar_;  //!< Right-hand side scalar of the division expression.
   //**********************************************************************************************

   //**Assignment to dense matrices****************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a sparse matrix-scalar division to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a sparse matrix-scalar
   // division expression to a dense matrix. Due to the explicit application of the SFINAE
   // principle, this operator can only be selected by the compiler in case the operand
   // requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      assign( DenseMatrix<MT2,SO2>& lhs, const SMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      assign( ~lhs, rhs.matrix_ );
      (~lhs) /= rhs.scalar_;
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Assignment to sparse matrices***************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a sparse matrix-scalar division to a sparse matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side division expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a sparse matrix-scalar
   // division expression to a sparse matrix. Due to the explicit application of the SFINAE
   // principle, this operator can only be selected by the compiler in case the operand
   // requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target sparse matrix
           , bool SO2 >    // Storage order of the target sparse matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      assign( SparseMatrix<MT2,SO2>& lhs, const SMatScalarDivExpr& rhs )
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
   /*!\brief Addition assignment of a sparse matrix-scalar division to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be added.
   // \return void
   //
   // This function implements the performance optimized addition assignment of a sparse
   // matrix-scalar division expression to a dense matrix. Due to the explicit application
   // of the SFINAE principle, this operator can only be selected by the compiler in case
   // the operand requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      addAssign( DenseMatrix<MT2,SO2>& lhs, const SMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_CONSTRAINT_MUST_BE_SPARSE_MATRIX_TYPE( ResultType );
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
   /*!\brief Subtraction assignment of a sparse matrix-scalar division to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized subtraction assignment of a sparse
   // matrix-scalar division expression to a dense matrix. Due to the explicit application
   // of the SFINAE principle, this operator can only be selected by the compiler in case
   // the operand requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      subAssign( DenseMatrix<MT2,SO2>& lhs, const SMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_CONSTRAINT_MUST_BE_SPARSE_MATRIX_TYPE( ResultType );
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
   // No special implementation for the SMP assignment to dense matrices.
   //**********************************************************************************************

   //**SMP assignment to sparse matrices***********************************************************
   // No special implementation for the SMP assignment to sparse matrices.
   //**********************************************************************************************

   //**SMP addition assignment to dense matrices***************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP addition assignment of a sparse matrix-scalar division to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be added.
   // \return void
   //
   // This function implements the performance optimized SMP addition assignment of a sparse
   // matrix-scalar division expression to a dense matrix. Due to the explicit application
   // of the SFINAE principle, this operator can only be selected by the compiler in case the
   // expression specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpAddAssign( DenseMatrix<MT2,SO2>& lhs, const SMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_CONSTRAINT_MUST_BE_SPARSE_MATRIX_TYPE( ResultType );
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
   /*!\brief SMP subtraction assignment of a sparse matrix-scalar division to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side division expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized SMP subtraction assignment of a sparse
   // matrix-scalar division expression to a dense matrix. Due to the explicit application
   // of the SFINAE principle, this operator can only be selected by the compiler in case the
   // expression specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpSubAssign( DenseMatrix<MT2,SO2>& lhs, const SMatScalarDivExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_CONSTRAINT_MUST_BE_SPARSE_MATRIX_TYPE( ResultType );
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

   //**Multiplication assignment to dense matrices*************************************************
   // No special implementation for the SMP multiplication assignment to dense matrices.
   //**********************************************************************************************

   //**SMP multiplication assignment to sparse matrices********************************************
   // No special implementation for the SMP multiplication assignment to sparse matrices.
   //**********************************************************************************************

   //**Compile time checks*************************************************************************
   /*! \cond BLAZE_INTERNAL */
   BLAZE_CONSTRAINT_MUST_BE_SPARSE_MATRIX_TYPE( MT );
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
/*!\brief Division operator for the division of a sparse matrix by a scalar value (\f$ A=B/s \f$).
// \ingroup sparse_matrix
//
// \param mat The left-hand side sparse matrix for the division.
// \param scalar The right-hand side scalar value for the division.
// \return The scaled result matrix.
//
// This operator represents the division of a sparse matrix by a scalar value:

   \code
   blaze::CompressedMatrix<double> A, B;
   // ... Resizing and initialization
   B = A / 0.24;
   \endcode

// The operator returns an expression representing a sparse matrix of the higher-order element
// type of the involved data types \a T1::ElementType and \a T2. Note that this operator only
// works for scalar values of built-in data type.
//
// \b Note: A division by zero is only checked by a user assert.
*/
template< typename T1    // Type of the left-hand side sparse matrix
        , bool SO        // Storage order of the left-hand side sparse matrix
        , typename T2 >  // Type of the right-hand side scalar
inline const typename EnableIf< IsNumeric<T2>, typename DivExprTrait<T1,T2>::Type >::Type
   operator/( const SparseMatrix<T1,SO>& mat, T2 scalar )
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
/*!\brief Multiplication operator for the multiplication of a sparse matrix-scalar division
//        expression and a scalar value (\f$ A=(B/s1)*s2 \f$).
// \ingroup sparse_matrix
//
// \param mat The left-hand side sparse matrix-scalar division.
// \param scalar The right-hand side scalar value for the multiplication.
// \return The scaled result matrix.
//
// This operator implements a performance optimized treatment of the multiplication of a
// sparse matrix-scalar division expression and a scalar value.
*/
template< typename MT     // Type of the sparse matrix of the left-hand side expression
        , typename ST1    // Type of the scalar of the left-hand side expression
        , bool SO         // Storage order of the sparse matrix
        , typename ST2 >  // Type of the right-hand side scalar
inline const typename EnableIf< IsFloatingPoint< typename DivTrait<ST2,ST1>::Type >
                              , typename MultExprTrait< SMatScalarDivExpr<MT,ST1,SO>, ST2 >::Type >::Type
   operator*( const SMatScalarDivExpr<MT,ST1,SO>& mat, ST2 scalar )
{
   BLAZE_FUNCTION_TRACE;

   return mat.leftOperand() * ( scalar / mat.rightOperand() );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Multiplication operator for the multiplication of a scalar value and a sparse matrix-
//        scalar division expression (\f$ A=s2*(B/s1) \f$).
// \ingroup sparse_matrix
//
// \param scalar The left-hand side scalar value for the multiplication.
// \param mat The right-hand side sparse matrix-scalar division.
// \return The scaled result matrix.
//
// This operator implements a performance optimized treatment of the multiplication of a
// scalar value and a sparse matrix-scalar division expression.
*/
template< typename ST1  // Type of the left-hand side scalar
        , typename MT   // Type of the sparse matrix of the right-hand side expression
        , typename ST2  // Type of the scalar of the right-hand side expression
        , bool SO >     // Storage order of the sparse matrix
inline const typename EnableIf< IsFloatingPoint< typename DivTrait<ST1,ST2>::Type >
                              , typename MultExprTrait< ST1, SMatScalarDivExpr<MT,ST2,SO> >::Type >::Type
   operator*( ST1 scalar, const SMatScalarDivExpr<MT,ST2,SO>& mat )
{
   BLAZE_FUNCTION_TRACE;

   return mat.leftOperand() * ( scalar / mat.rightOperand() );
}
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Division operator for the division of a sparse matrix-scalar division expression
//        and a scalar value (\f$ A=(B/s1)/s2 \f$).
// \ingroup sparse_matrix
//
// \param mat The left-hand side sparse matrix-scalar division.
// \param scalar The right-hand side scalar value for the division.
// \return The scaled result matrix.
//
// This operator implements a performance optimized treatment of the division of a sparse
// matrix-scalar division expression and a scalar value.
*/
template< typename MT     // Type of the sparse matrix of the left-hand side expression
        , typename ST1    // Type of the scalar of the left-hand side expression
        , bool SO         // Storage order of the sparse matrix
        , typename ST2 >  // Type of the right-hand side scalar
inline const typename EnableIf< IsNumeric<ST2>
                              , typename DivExprTrait<MT,typename MultTrait<ST1,ST2>::Type>::Type >::Type
   operator/( const SMatScalarDivExpr<MT,ST1,SO>& mat, ST2 scalar )
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
//  SMATSCALARMULTEXPRTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, typename ST1, typename ST2 >
struct SMatScalarMultExprTrait< SMatScalarDivExpr<MT,ST1,false>, ST2 >
{
 private:
   //**********************************************************************************************
   enum { condition = IsFloatingPoint<typename DivTrait<ST1,ST2>::Type>::value };
   //**********************************************************************************************

   //**********************************************************************************************
   typedef typename SMatScalarMultExprTrait<MT,typename DivTrait<ST1,ST2>::Type>::Type  T1;
   typedef SMatScalarMultExpr< SMatScalarDivExpr<MT,ST1,false>, ST2, false >            T2;
   //**********************************************************************************************

 public:
   //**********************************************************************************************
   typedef typename SelectType< IsSparseMatrix<MT>::value && IsRowMajorMatrix<MT>::value &&
                                IsNumeric<ST1>::value && IsNumeric<ST2>::value
                              , typename SelectType<condition,T1,T2>::Type
                              , INVALID_TYPE >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  TSMATSCALARMULTEXPRTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, typename ST1, typename ST2 >
struct TSMatScalarMultExprTrait< SMatScalarDivExpr<MT,ST1,true>, ST2 >
{
 private:
   //**********************************************************************************************
   enum { condition = IsFloatingPoint<typename DivTrait<ST1,ST2>::Type>::value };
   //**********************************************************************************************

   //**********************************************************************************************
   typedef typename SMatScalarMultExprTrait<MT,typename DivTrait<ST1,ST2>::Type>::Type  T1;
   typedef SMatScalarMultExpr< SMatScalarDivExpr<MT,ST1,true>, ST2, true >              T2;
   //**********************************************************************************************

 public:
   //**********************************************************************************************
   typedef typename SelectType< IsSparseMatrix<MT>::value && IsColumnMajorMatrix<MT>::value &&
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
struct SubmatrixExprTrait< SMatScalarDivExpr<MT,ST,SO>, AF >
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
struct RowExprTrait< SMatScalarDivExpr<MT,ST,SO> >
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
struct ColumnExprTrait< SMatScalarDivExpr<MT,ST,SO> >
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
