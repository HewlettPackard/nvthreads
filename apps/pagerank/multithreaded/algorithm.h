#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <matrix.h>
#include <vector.h>
#include <math.h>
#include <pthread.h>
#include <utils.h>

extern void gen_link_vector_crs(vector_i *linkv, int* empty, const matrix_crs_f *adjm);

extern void gen_google_matrix_crs(matrix_crs_f *m, const vector_i *linkv, const float damping_factor);

#endif // ALGORITHM_H

