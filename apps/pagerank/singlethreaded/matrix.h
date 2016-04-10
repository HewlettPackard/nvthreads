/*
  Copyright 2015-2016 Hewlett Packard Enterprise Development LP
  
  This program is free software; you can redistribute it and/or modify 
  it under the terms of the GNU General Public License, version 2 as 
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License 
  along with this program; if not, write to the Free Software 
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/
#ifndef MATRIX_H
#define MATRIX_H

#include <malloc.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <logger.h>
#include <vector.h>

typedef struct matrix_f {
    float** elements;
    size_t size;
} matrix_f;

typedef struct matrix_i {
    int** elements;
    size_t size;
} matrix_i;

extern void matrix_f_init(matrix_f *m, size_t size);

extern void matrix_f_init_set(matrix_f *m, size_t size, float init);

extern void matrix_i_init(matrix_i *m, size_t size);

extern void matrix_i_init_set(matrix_i *m, size_t size, int init);

extern void matrix_f_copy(matrix_f *out, const matrix_f *in);

extern void matrix_i_to_f(const matrix_i *a, matrix_f *b);

extern int matrix_i_load(const char* path, matrix_i* m, const char col, const char row);

extern int matrix_f_realloc(matrix_f *m, size_t newsize, float init);

extern int matrix_i_realloc(matrix_i *m, size_t newsize, int init);

extern void matrix_f_free(matrix_f *m);

extern void matrix_i_free(matrix_i *m);

extern void matrix_f_transpose(const matrix_f *m);

extern void matrix_i_transpose(const matrix_i *m);

extern void matrix_f_multiply_matrix_f(matrix_f *out, const matrix_f *a, const matrix_f *b);

extern void matrix_f_multiply_matrix_f_rng(matrix_f *out, const matrix_f *a, const matrix_f *b, const size_t xoffs, const size_t xlength, const size_t yoffs, const size_t ylength);

extern void matrix_f_multiply_vector_f(vector_f *tmp, const matrix_f *m, const vector_f *v);

#endif // MATRIX_H

