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
#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <stdio.h>

#include <logger.h>
#include <matrix.h>
#include <crsmatrix.h>
#include <vector.h>

extern int float_comparator(const void* a, const void* b);

extern int compare_floats(float a, float b);

extern int int_comparator(const void *a, const void *b);

//extern void matrix_display(const matrix_f *m);

extern void matrix_f_display(const logd_lvl_t lvl, const matrix_f *m);

extern void matrix_i_display(const logd_lvl_t lvl, const matrix_i *m);

extern void mcrs_i_display(const logd_lvl_t lvl, const matrix_crs_i *m);

extern void mcrs_f_display(const logd_lvl_t lvl, const matrix_crs_f *m);

//extern void mcrs_f_display(const logd_lvl_t lvl, const matrix_crs_f *m);

//extern void vector_display(const vector_f *v);

extern void vector_f_display(const logd_lvl_t lvl, const vector_f *v);

extern void vector_i_display(const logd_lvl_t lvl, const vector_i *v);

extern void vector_sort(vector_f *v);

extern void vector_f_sort(vector_f *v);

extern void vector_i_sort(vector_i *v);

#endif // UTILS_H

