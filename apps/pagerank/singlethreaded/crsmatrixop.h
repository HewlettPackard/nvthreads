#include <crsmatrix.h>

#include <matrix.h>
#include <vector.h>
#include <logger.h>

extern mcrs_err mcrs_gmatrix_mult_vector_f(vector_f *out, const matrix_crs_f *m, const vector_f *v, const vector_i *linkv, const int empty);
extern mcrs_err check_gmatrix_integrity(const logd_lvl_t lvl, const matrix_crs_f *m);
