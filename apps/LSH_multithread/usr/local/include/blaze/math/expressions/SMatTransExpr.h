//=================================================================================================
/*!
//  \file blaze/math/expressions/SMatTransExpr.h
//  \brief Header file for the sparse matrix transpose expression
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

#ifndef _BLAZE_MATH_EXPRESSIONS_SMATTRANSEXPR_H_
#define _BLAZE_MATH_EXPRESSIONS_SMATTRANSEXPR_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <iterator>
#include <blaze/math/constraints/SparseMatrix.h>
#include <blaze/math/constraints/StorageOrder.h>
#include <blaze/math/expressions/Computation.h>
#include <blaze/math/expressions/DMatTransposer.h>
#include <blaze/math/expressions/Forward.h>
#include <blaze/math/expressions/MatTransExpr.h>
#include <blaze/math/expressions/SMatTransposer.h>
#include <blaze/math/expressions/SparseMatrix.h>
#include <blaze/math/traits/ColumnExprTrait.h>
#include <blaze/math/traits/RowExprTrait.h>
#include <blaze/math/traits/SubmatrixExprTrait.h>
#include <blaze/math/traits/TransExprTrait.h>
#include <blaze/math/typetraits/IsComputation.h>
#include <blaze/math/typetraits/IsExpression.h>
#include <blaze/math/typetraits/RequiresEvaluation.h>
#include <blaze/util/Assert.h>
#include <blaze/util/constraints/Reference.h>
#include <blaze/util/EmptyType.h>
#include <blaze/util/EnableIf.h>
#include <blaze/util/logging/FunctionTrace.h>
#include <blaze/util/SelectType.h>
#include <blaze/util/Types.h>
#include <blaze/util/typetraits/RemoveReference.h>


namespace blaze {

//=================================================================================================
//
//  CLASS SMATTRANSEXPR
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Expression object for sparse matrix transpositions.
// \ingroup sparse_matrix_expression
//
// The SMatTransExpr class represents the compile time expression for transpositions of
// sparse matrices.
*/
template< typename MT  // Type of the sparse matrix
        , bool SO >    // Storage order
class SMatTransExpr : public SparseMatrix< SMatTransExpr<MT,SO>, SO >
                    , private MatTransExpr
                    , private SelectType< IsComputation<MT>::value, Computation, EmptyType >::Type
{
 private:
   //**Type definitions****************************************************************************
   typedef typename MT::ResultType     RT;  //!< Result type of the sparse matrix expression.
   typedef typename MT::CompositeType  CT;  //!< Composite type of the sparse matrix expression.
   //**********************************************************************************************

   //**Serial evaluation strategy******************************************************************
   //! Compilation switch for the serial evaluation strategy of the transposition expression.
   /*! The \a useAssign compile time constant expression represents a compilation switch for
       the serial evaluation strategy of the transposition expression. In case the given sparse
       matrix expression of type \a MT requires an intermediate evaluation, \a useAssign will be
       set to 1 and the transposition expression will be evaluated via the \a assign function
       family. Otherwise \a useAssign will be set to 0 and the expression will be evaluated via
       the subscript operator. */
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
       strategy. In case the target matrix is SMP assignable and the sparse matrix operand requires
       an intermediate evaluation, \a value is set to 1 and the expression specific evaluation
       strategy is selected. Otherwise \a value is set to 0 and the default strategy is chosen. */
   template< typename MT2 >
   struct UseSMPAssign {
      enum { value = MT2::smpAssignable && useAssign };
   };
   /*! \endcond */
   //**********************************************************************************************

 public:
   //**Type definitions****************************************************************************
   typedef SMatTransExpr<MT,SO>        This;           //!< Type of this SMatTransExpr instance.
   typedef typename MT::TransposeType  ResultType;     //!< Result type for expression template evaluations.
   typedef typename MT::OppositeType   OppositeType;   //!< Result type with opposite storage order for expression template evaluations.
   typedef typename MT::ResultType     TransposeType;  //!< Transpose type for expression template evaluations.
   typedef typename MT::ElementType    ElementType;    //!< Resulting element type.
   typedef typename MT::ReturnType     ReturnType;     //!< Return type for expression template evaluations.

   //! Data type for composite expression templates.
   typedef typename SelectType< useAssign, const ResultType, const SMatTransExpr& >::Type  CompositeType;

   //! Composite data type of the sparse matrix expression.
   typedef typename SelectType< IsExpression<MT>::value, const MT, const MT& >::Type  Operand;
   //**********************************************************************************************

   //**ConstIterator class definition**************************************************************
   /*!\brief Iterator over the elements of the sparse matrix transposition expression.
   */
   class ConstIterator
   {
    public:
      //**Type definitions*************************************************************************
      //! Iterator type of the sparse matrix expression.
      typedef typename RemoveReference<Operand>::Type::ConstIterator  IteratorType;

      typedef std::forward_iterator_tag                                     IteratorCategory;  //!< The iterator category.
      typedef typename std::iterator_traits<IteratorType>::value_type       ValueType;         //!< Type of the underlying pointers.
      typedef typename std::iterator_traits<IteratorType>::pointer          PointerType;       //!< Pointer return type.
      typedef typename std::iterator_traits<IteratorType>::reference        ReferenceType;     //!< Reference return type.
      typedef typename std::iterator_traits<IteratorType>::difference_type  DifferenceType;    //!< Difference between two iterators.

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
      inline ConstIterator( IteratorType it )
         : it_( it )  // Iterator over the elements of the sparse matrix expression
      {}
      //*******************************************************************************************

      //**Prefix increment operator****************************************************************
      /*!\brief Pre-increment operator.
      //
      // \return Reference to the incremented expression iterator.
      */
      inline ConstIterator& operator++() {
         ++it_;
         return *this;
      }
      //*******************************************************************************************

      //**Element access operator******************************************************************
      /*!\brief Direct access to the sparse matrix element at the current iterator position.
      //
      // \return The current value of the sparse element.
      */
      inline const ValueType operator*() const {
         return *it_;
      }
      //*******************************************************************************************

      //**Element access operator******************************************************************
      /*!\brief Direct access to the sparse matrix element at the current iterator position.
      //
      // \return Reference to the sparse matrix element at the current iterator position.
      */
      inline const IteratorType operator->() const {
         return it_;
      }
      //*******************************************************************************************

      //**Value function***************************************************************************
      /*!\brief Access to the current value of the sparse element.
      //
      // \return The current value of the sparse element.
      */
      inline ReturnType value() const {
         return it_->value();
      }
      //*******************************************************************************************

      //**Index function***************************************************************************
      /*!\brief Access to the current index of the sparse element.
      //
      // \return The current index of the sparse element.
      */
      inline size_t index() const {
         return it_->index();
      }
      //*******************************************************************************************

      //**Equality operator************************************************************************
      /*!\brief Equality comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side expression iterator.
      // \return \a true if the iterators refer to the same element, \a false if not.
      */
      inline bool operator==( const ConstIterator& rhs ) const {
         return it_ == rhs.it_;
      }
      //*******************************************************************************************

      //**Inequality operator**********************************************************************
      /*!\brief Inequality comparison between two ConstIterator objects.
      //
      // \param rhs The right-hand side expression iterator.
      // \return \a true if the iterators don't refer to the same element, \a false if they do.
      */
      inline bool operator!=( const ConstIterator& rhs ) const {
         return it_ != rhs.it_;
      }
      //*******************************************************************************************

      //**Subtraction operator*********************************************************************
      /*!\brief Calculating the number of elements between two expression iterators.
      //
      // \param rhs The right-hand side expression iterator.
      // \return The number of elements between the two expression iterators.
      */
      inline DifferenceType operator-( const ConstIterator& rhs ) const {
         return it_ - rhs.it_;
      }
      //*******************************************************************************************

    private:
      //**Member variables*************************************************************************
      IteratorType it_;  //!< Iterator over the elements of the sparse matrix expression.
      //*******************************************************************************************
   };
   //**********************************************************************************************

   //**Compilation flags***************************************************************************
   //! Compilation switch for the expression template assignment strategy.
   enum { smpAssignable = MT::smpAssignable };
   //**********************************************************************************************

   //**Constructor*********************************************************************************
   /*!\brief Constructor for the SMatTransExpr class.
   //
   // \param sm The sparse matrix operand of the transposition expression.
   */
   explicit inline SMatTransExpr( const MT& sm )
      : sm_( sm )
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
      BLAZE_INTERNAL_ASSERT( i < sm_.columns(), "Invalid row access index"    );
      BLAZE_INTERNAL_ASSERT( j < sm_.rows()   , "Invalid column access index" );
      return sm_(j,i);
   }
   //**********************************************************************************************

   //**Begin function******************************************************************************
   /*!\brief Returns an iterator to the first non-zero element of row/column \a i.
   //
   // \param i The row/column index.
   // \return Iterator to the first non-zero element of row/column \a i.
   */
   inline ConstIterator begin( size_t i ) const {
      return sm_.begin(i);
   }
   //**********************************************************************************************

   //**End function********************************************************************************
   /*!\brief Returns an iterator just past the last non-zero element of row/column \a i.
   //
   // \param i The row/column index.
   // \return Iterator just past the last non-zero element of row/column \a i.
   */
   inline ConstIterator end( size_t i ) const {
      return sm_.end(i);
   }
   //**********************************************************************************************

   //**Rows function*******************************************************************************
   /*!\brief Returns the current number of rows of the matrix.
   //
   // \return The number of rows of the matrix.
   */
   inline size_t rows() const {
      return sm_.columns();
   }
   //**********************************************************************************************

   //**Columns function****************************************************************************
   /*!\brief Returns the current number of columns of the matrix.
   //
   // \return The number of columns of the matrix.
   */
   inline size_t columns() const {
      return sm_.rows();
   }
   //**********************************************************************************************

   //**NonZeros function***************************************************************************
   /*!\brief Returns the number of non-zero elements in the sparse matrix.
   //
   // \return The number of non-zero elements in the sparse matrix.
   */
   inline size_t nonZeros() const {
      return sm_.nonZeros();
   }
   //**********************************************************************************************

   //**Operand access******************************************************************************
   /*!\brief Returns the sparse matrix operand.
   //
   // \return The sparse matrix operand.
   */
   inline Operand operand() const {
      return sm_;
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
      return sm_.canAlias( alias );
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
      return sm_.isAliased( alias );
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the expression can be used in SMP assignments.
   //
   // \return \a true in case the expression can be used in SMP assignments, \a false if not.
   */
   inline bool canSMPAssign() const {
      return sm_.canSMPAssign();
   }
   //**********************************************************************************************

 private:
   //**Member variables****************************************************************************
   Operand sm_;  //!< Sparse matrix of the transposition expression.
   //**********************************************************************************************

   //**Assignment to dense matrices****************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a sparse matrix transposition expression to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side transposition expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a sparse matrix
   // transposition expression to a dense matrix. Due to the explicit application of
   // the SFINAE principle, this operator can only be selected by the compiler in case
   // the operand requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      assign( DenseMatrix<MT2,SO2>& lhs, const SMatTransExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      DMatTransposer<MT2,!SO2> tmp( ~lhs );
      assign( tmp, rhs.sm_ );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Assignment to sparse matrices***************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a sparse matrix transposition expression to a sparse matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side transposition expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a sparse matrix
   // transposition expression to a sparse matrix. Due to the explicit application of
   // the SFINAE principle, this operator can only be selected by the compiler in case
   // the operand requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target sparse matrix
           , bool SO2 >    // Storage order of the target sparse matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      assign( SparseMatrix<MT2,SO2>& lhs, const SMatTransExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      SMatTransposer<MT2,!SO2> tmp( ~lhs );
      assign( tmp, rhs.sm_ );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Addition assignment to dense matrices*******************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Addition assignment of a sparse matrix transposition expression to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side transposition expression to be added.
   // \return void
   //
   // This function implements the performance optimized addition assignment of a sparse
   // matrix transposition expression to a dense matrix. Due to the explicit application
   // of the SFINAE principle, this operator can only be selected by the compiler in case
   // the operand requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      addAssign( DenseMatrix<MT2,SO2>& lhs, const SMatTransExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      DMatTransposer<MT2,!SO2> tmp( ~lhs );
      addAssign( tmp, rhs.sm_ );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Addition assignment to sparse matrices******************************************************
   // No special implementation for the addition assignment to sparse matrices.
   //**********************************************************************************************

   //**Subtraction assignment to dense matrices****************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Subtraction assignment of a sparse matrix transposition expression to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side transposition expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized subtraction assignment of a sparse
   // matrix transposition expression to a dense matrix. Due to the explicit application of
   // the SFINAE principle, this operator can only be selected by the compiler in case the
   // operand requires an intermediate evaluation.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseAssign<MT2> >::Type
      subAssign( DenseMatrix<MT2,SO2>& lhs, const SMatTransExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      DMatTransposer<MT2,!SO2> tmp( ~lhs );
      subAssign( tmp, rhs.sm_ );
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
   /*!\brief SMP assignment of a sparse matrix transposition expression to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side transposition expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized SMP assignment of a sparse matrix
   // transposition expression to a dense matrix. Due to the explicit application of the
   // SFINAE principle, this operator can only be selected by the compiler in case the
   // expression specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpAssign( DenseMatrix<MT2,SO2>& lhs, const SMatTransExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      DMatTransposer<MT2,!SO2> tmp( ~lhs );
      smpAssign( tmp, rhs.sm_ );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP assignment to sparse matrices***********************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP assignment of a sparse matrix transposition expression to a sparse matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side transposition expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized SMP assignment of a sparse matrix
   // transposition expression to a sparse matrix. Due to the explicit application of the
   // SFINAE principle, this operator can only be selected by the compiler in case the
   // expression specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target sparse matrix
           , bool SO2 >    // Storage order of the target sparse matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpAssign( SparseMatrix<MT2,SO2>& lhs, const SMatTransExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      SMatTransposer<MT2,!SO2> tmp( ~lhs );
      smpAssign( tmp, rhs.sm_ );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP addition assignment to dense matrices***************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP addition assignment of a sparse matrix transposition expression to a dense matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side transposition expression to be added.
   // \return void
   //
   // This function implements the performance optimized SMP addition assignment of a sparse
   // matrix transposition expression to a dense matrix. Due to the explicit application of
   // the SFINAE principle, this operator can only be selected by the compiler in case the
   // expression specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpAddAssign( DenseMatrix<MT2,SO2>& lhs, const SMatTransExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      DMatTransposer<MT2,!SO2> tmp( ~lhs );
      smpAddAssign( tmp, rhs.sm_ );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP addition assignment to sparse matrices**************************************************
   // No special implementation for the SMP addition assignment to sparse matrices.
   //**********************************************************************************************

   //**SMP subtraction assignment to dense matrices************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP subtraction assignment of a sparse matrix transposition expression to a dense
   //        matrix.
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side transposition expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized SMP subtraction assignment of a sparse
   // matrix transposition expression to a dense matrix. Due to the explicit application of the
   // SFINAE principle, this operator can only be selected by the compiler in case the expression
   // specific parallel evaluation strategy is selected.
   */
   template< typename MT2  // Type of the target dense matrix
           , bool SO2 >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT2> >::Type
      smpSubAssign( DenseMatrix<MT2,SO2>& lhs, const SMatTransExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      DMatTransposer<MT2,!SO2> tmp( ~lhs );
      smpSubAssign( tmp, rhs.sm_ );
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

   //**Trans function******************************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Calculating the transpose of a transpose sparse matrix.
   // \ingroup sparse_matrix
   //
   // \param sm The sparse matrix to be (re-)transposed.
   // \return The transpose of the transpose matrix.
   //
   // This function returns an expression representing the transpose of a transpose sparse matrix:

      \code
      blaze::DynamicMatrix<double,rowMajor> A, B;
      // ... Resizing and initialization
      B = trans( trans( A ) );
      \endcode
   */
   template< typename MT2  // Type of the sparse matrix
           , bool SO2 >    // Storage order of the sparse matrix
   friend inline Operand trans( const SMatTransExpr<MT2,SO2>& sm )
   {
      BLAZE_FUNCTION_TRACE;

      return sm.sm_;
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Compile time checks*************************************************************************
   /*! \cond BLAZE_INTERNAL */
   BLAZE_CONSTRAINT_MUST_BE_SPARSE_MATRIX_TYPE( MT );
   BLAZE_CONSTRAINT_MUST_BE_MATRIX_WITH_STORAGE_ORDER( MT, !SO );
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
/*!\brief Calculation of the transpose of the given sparse matrix.
// \ingroup sparse_matrix
//
// \param sm The sparse matrix to be transposed.
// \return The transpose of the matrix.
//
// This function returns an expression representing the transpose of the given sparse matrix:

   \code
   blaze::CompressedMatrix<double> A, B;
   // ... Resizing and initialization
   B = trans( A );
   \endcode
*/
template< typename MT  // Type of the sparse matrix
        , bool SO >    // Storage order
inline const SMatTransExpr<MT,!SO> trans( const SparseMatrix<MT,SO>& sm )
{
   BLAZE_FUNCTION_TRACE;

   return SMatTransExpr<MT,!SO>( ~sm );
}
//*************************************************************************************************




//=================================================================================================
//
//  EXPRESSION TRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, bool SO, bool AF >
struct SubmatrixExprTrait< SMatTransExpr<MT,SO>, AF >
{
 public:
   //**********************************************************************************************
   typedef typename TransExprTrait< typename SubmatrixExprTrait<const MT,AF>::Type >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, bool SO >
struct RowExprTrait< SMatTransExpr<MT,SO> >
{
 public:
   //**********************************************************************************************
   typedef typename TransExprTrait< typename ColumnExprTrait<const MT>::Type >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT, bool SO >
struct ColumnExprTrait< SMatTransExpr<MT,SO> >
{
 public:
   //**********************************************************************************************
   typedef typename TransExprTrait< typename RowExprTrait<const MT>::Type >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************

} // namespace blaze

#endif
