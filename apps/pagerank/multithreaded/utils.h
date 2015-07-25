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

