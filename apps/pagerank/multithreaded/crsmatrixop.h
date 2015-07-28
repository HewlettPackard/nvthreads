#include <crsmatrix.h>

#include <matrix.h>
#include <vector.h>
#include <logger.h>

#include <pthread.h>

typedef struct thd_data {
	int thread_id;
	
	logd_lvl_t lvl;
	l_clock_t* tmr;
	
	vector_f *out;
	vector_f *loc;
	matrix_crs_f *m;
	vector_f *v;
	
	size_t rstart;
	size_t rend;
} thd_data_t;

extern mcrs_err mcrs_gmatrix_mult_vector_f_mt(logd_lvl_t lvl, vector_f *out, const matrix_crs_f *m, const vector_f *v, const size_t n_threads, const size_t n_iterations);
extern mcrs_err mcrs_gmatrix_mult_vector_f(vector_f *out, const matrix_crs_f *m, const vector_f *v);
mcrs_err mcrs_gmatrix_mult_vector_f_rng(vector_f *out, const matrix_crs_f *m, const vector_f *v, const size_t rstart, const size_t rend, const int thd_enabled);
extern mcrs_err check_gmatrix_integrity(const logd_lvl_t lvl, const matrix_crs_f *m);