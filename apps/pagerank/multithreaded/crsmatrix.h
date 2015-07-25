#ifndef CRSMATRIX_H
#define CRSMATRIX_H

#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <types.h>
#include <matrix.h>

static const size_t MCRS_ALLOC_BLOCK = 100000;

typedef enum {
	MCRS_ERR_NONE = 0,
	MCRS_ERR_UNABLE_TO_ALLOC,
	MCRS_ERR_NO_SEQ_ACCESS,
	MCRS_ERR_UNABLE_TO_OPEN_FILE,
	MCRS_ERR_INVALID_FILE,
	MCRS_ERR_INVALID_MODE,
	MCRS_ERR_INVALID_PARAMS,
	MCRS_ERR_FAILED_INTEGRITY_CHECK,
	MCRS_ERR_FAILED_CREATING_THREADS,
	MCRS_ERR_FAILED_JOINING_THREADS
} mcrs_err;

typedef enum {
	MCRS_SET,
	MCRS_ADD,
	MCRS_SUB
} mcrs_set_mode;

typedef struct matrix_crs_i {
	size_t allocd_row;
	size_t allocd_col;
	
	i_t empty;

	size_t n_row;
	size_t n_col;
	
	size_t sz_row;
	size_t sz_col;
	
	i_t *values; // sz_col
	size_t *col_ind; // sz_col
	size_t *row_ptr; // sz_row
} matrix_crs_i;

typedef struct matrix_crs_f {
	size_t allocd_row;
	size_t allocd_col;
	
	f_t empty;
	
	size_t n_row;
	size_t n_col;
	
	size_t sz_row;
	size_t sz_col;
	
	f_t *values;
	size_t *col_ind;
	size_t *row_ptr;
} matrix_crs_f;

extern mcrs_err mcrs_i_init(matrix_crs_i *m, i_t empty);
extern mcrs_err mcrs_f_init(matrix_crs_f *m, f_t empty);

extern mcrs_err mcrs_i_free(matrix_crs_i *m);
extern mcrs_err mcrs_f_free(matrix_crs_f *m);

extern i_t mcrs_i_get(const matrix_crs_i *m, size_t x, size_t y);
extern f_t mcrs_f_get(const matrix_crs_f *m, size_t x, size_t y);

extern mcrs_err mcrs_i_set(matrix_crs_i *m, size_t x, size_t y, i_t val, mcrs_set_mode md);
extern mcrs_err mcrs_f_set(matrix_crs_f *m, size_t x, size_t y, f_t val, mcrs_set_mode md);

extern mcrs_err mcrs_i_from_matrix_i(matrix_crs_i *dst, const matrix_i *src);//, size_t xoffs, size_t xlength, size_t yoffs, size_t ylength);
//extern mcrs_err mcrs_f_from_matrix_f(matrix_crs_f *dst, const matrix_f *src, size_t xoffs, size_t xlength, size_t yoffs, size_t ylength);

extern mcrs_err matrix_f_from_mcrs_i(matrix_f *dst, const matrix_crs_i *src);
extern mcrs_err matrix_f_from_mcrs_f(matrix_f *dst, const matrix_crs_f *src);

extern mcrs_err mcrs_i_load(matrix_crs_i *m, const char *path, const char col, const char row);
extern mcrs_err mcrs_f_load(matrix_crs_f *m, const char *path, const char col, const char row);

#endif // CRSMATRIX_H
