//=================================================================================================
/*!
//  \file blaze/math/Constraints.h
//  \brief Header file for all mathematical constraints
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

#ifndef _BLAZE_MATH_CONSTRAINTS_H_
#define _BLAZE_MATH_CONSTRAINTS_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <blaze/math/constraints/AbsExpr.h>
#include <blaze/math/constraints/AddExpr.h>
#include <blaze/math/constraints/Column.h>
#include <blaze/math/constraints/Computation.h>
#include <blaze/math/constraints/CrossExpr.h>
#include <blaze/math/constraints/DenseMatrix.h>
#include <blaze/math/constraints/DenseVector.h>
#include <blaze/math/constraints/DivExpr.h>
#include <blaze/math/constraints/EvalExpr.h>
#include <blaze/math/constraints/Expression.h>
#include <blaze/math/constraints/MatAbsExpr.h>
#include <blaze/math/constraints/MatEvalExpr.h>
#include <blaze/math/constraints/MatMatAddExpr.h>
#include <blaze/math/constraints/MatMatMultExpr.h>
#include <blaze/math/constraints/MatMatSubExpr.h>
#include <blaze/math/constraints/Matrix.h>
#include <blaze/math/constraints/MatScalarDivExpr.h>
#include <blaze/math/constraints/MatScalarMultExpr.h>
#include <blaze/math/constraints/MatSerialExpr.h>
#include <blaze/math/constraints/MatTransExpr.h>
#include <blaze/math/constraints/MatVecMultExpr.h>
#include <blaze/math/constraints/MultExpr.h>
#include <blaze/math/constraints/RequiresEvaluation.h>
#include <blaze/math/constraints/Resizable.h>
#include <blaze/math/constraints/Row.h>
#include <blaze/math/constraints/SerialExpr.h>
#include <blaze/math/constraints/SMPAssignable.h>
#include <blaze/math/constraints/SparseElement.h>
#include <blaze/math/constraints/SparseMatrix.h>
#include <blaze/math/constraints/SparseVector.h>
#include <blaze/math/constraints/StorageOrder.h>
#include <blaze/math/constraints/SubExpr.h>
#include <blaze/math/constraints/Submatrix.h>
#include <blaze/math/constraints/Subvector.h>
#include <blaze/math/constraints/TransExpr.h>
#include <blaze/math/constraints/TransposeFlag.h>
#include <blaze/math/constraints/TVecMatMultExpr.h>
#include <blaze/math/constraints/VecAbsExpr.h>
#include <blaze/math/constraints/VecEvalExpr.h>
#include <blaze/math/constraints/VecScalarDivExpr.h>
#include <blaze/math/constraints/VecScalarMultExpr.h>
#include <blaze/math/constraints/VecSerialExpr.h>
#include <blaze/math/constraints/Vector.h>
#include <blaze/math/constraints/VecTransExpr.h>
#include <blaze/math/constraints/VecTVecMultExpr.h>
#include <blaze/math/constraints/VecVecAddExpr.h>
#include <blaze/math/constraints/VecVecMultExpr.h>
#include <blaze/math/constraints/VecVecSubExpr.h>
#include <blaze/math/constraints/View.h>

#endif
