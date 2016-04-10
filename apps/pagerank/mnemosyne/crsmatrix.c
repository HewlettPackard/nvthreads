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
#include "crsmatrix.h"
#include <mnemosyne.h>
#include <mtm.h>
#include <pmalloc.h>
#include <string.h>

//#ifndef PMALLOC
//#define pmalloc(sz) malloc(sz)
//#endif
#define prealloc(ptr, sz) prealloc_hook(ptr, sz)

void *prealloc_hook(void *ptr, size_t sz){
    void *buf;

//  fprintf(stderr, "prealloc(%p, %zu)\n", ptr, sz);
    buf = pmalloc(sz);
    memset(buf, 0, sz);
    if ( ptr != NULL ) {
//      fprintf(stderr, "ptr %p is not NULL\n", ptr);
//      memcpy(buf, ptr, sz);
        pfree(ptr);
    }
    return buf;
}

extern mcrs_err mcrs_f_init(matrix_crs_f *m, f_t empty, size_t col_sz, size_t row_sz) {
    m->allocd_row = row_sz;
    m->allocd_col = col_sz;
    m->empty = empty;
    m->sz_row = 0;
    m->sz_col = 0;
    m->n_col = 0;

    m->values = pmalloc(sizeof(m->values) * col_sz);
    m->col_ind = pmalloc(sizeof(m->col_ind) * col_sz);
    m->row_ptr = pmalloc(sizeof(m->row_ptr) * row_sz);

    if ( m->values && m->col_ind && m->row_ptr )
        return MCRS_ERR_NONE;
    else
        return MCRS_ERR_UNABLE_TO_ALLOC;
}

extern size_t mcrs_f_get_size(matrix_crs_f *m) {
    return (m->sz_row > m->n_col ? m->sz_row : m->n_col);
}

extern mcrs_err mcrs_f_free(matrix_crs_f *m) {
    m->allocd_row = 0;
    m->allocd_col = 0;
    m->empty = 0;
    m->sz_row = 0;
    m->sz_col = 0;
    m->n_col = 0;

    pfree(m->values);
    pfree(m->col_ind);
    pfree(m->row_ptr);

    return MCRS_ERR_NONE;
}

extern f_t mcrs_f_get(const matrix_crs_f *m, size_t x, size_t y) {
    if ( y >= m->sz_row )
        return m->empty;

    size_t ri = m->row_ptr[y];
    size_t ri_n = ((y + 1 < m->sz_row) ? m->row_ptr[y + 1] : m->sz_col);

    if ( ri == ri_n )
        return m->empty;

    size_t ci = 0;
    int found = 0;

    for (ci = ri; ci < ri_n; ci++) {
        if ( m->col_ind[ci] == x ) {
            found = 1;
            break;
        }
    }

    if ( !found )
        return m->empty;
    else
        return m->values[ci];
}

extern mcrs_err mcrs_f_set(matrix_crs_f *m, size_t x, size_t y, f_t val, mcrs_set_mode md) {
    m->n_col = ((x + 1) > m->n_col ? (x + 1) : m->n_col);

//  fprintf(stderr, "mcrs_f_set, m %p\n", m);

    if ( val == m->empty )
        return MCRS_ERR_NONE;

    size_t ci = 0;
    size_t ci_n = 0;
    int newrow = 0;
    int newcol = 0;
    int found = 0;

    int ri_old;

    if ( y >= m->sz_row ) { // resize row_ptr
        if ( y >= m->allocd_row ) {
		size_t sz_prev = m->allocd_row;
		
            	while (y >= m->allocd_row) {
                	m->allocd_row += MCRS_ALLOC_BLOCK;
            	}

//          	fprintf(stderr, "mcrs_f_set, before prealloc, m %p\n", m);
            	void* tmp = pmalloc(sizeof(m->row_ptr) * m->allocd_row);
		memcpy(m->row_ptr, tmp, sz_prev);
		pfree(m->row_ptr);
		m->row_ptr = tmp;//prealloc(m->row_ptr, sizeof(m->row_ptr) * m->allocd_row);
		
            	logd(LOGD_H, " row_ptr reallocd to %d\n", m->allocd_row);
        }

        ri_old = m->sz_row;

        m->sz_row = y + 1;

        for (; ri_old < y; ri_old++) {
            m->row_ptr[ri_old] = m->sz_col;
        }

        if ( m->row_ptr == NULL ) {
            logd_e("ERROR: row_ptr = null sz_row=%d\n", m->sz_row);
            return MCRS_ERR_UNABLE_TO_ALLOC;
        }

        ci = m->sz_col;

        m->row_ptr[y] = ci;

        newrow = 1;
        found = 1;
    } else {
        ci = m->row_ptr[y];
        ci_n = (y + 1 >= m->sz_row ? m->sz_col : m->row_ptr[y + 1]);

        if ( ci == ci_n )
            return MCRS_ERR_NONE;
    }

    if ( !found ) {
        for (; (ci < ci_n); ci++) {
            if ( (m->col_ind[ci]) == x ) {
                found = 1;
                break;
            }
        }
    }

    if ( !found || newrow ) { // resize col_ind
        if ( !newrow && m->col_ind[m->sz_col - 1] > x ) {
            logd_e("ERROR: no seq access on rows!\n");
            return MCRS_ERR_NO_SEQ_ACCESS;
        }

        if ( (m->sz_col + 1) > m->allocd_col ) {
		size_t sz_prev = m->allocd_col;
		m->allocd_col += MCRS_ALLOC_BLOCK;
		
		void* tmp = pmalloc(sizeof(m->col_ind) * m->allocd_col);
		memcpy(m->col_ind, tmp, sz_prev);
		pfree(m->col_ind);
		m->col_ind = tmp;
		
		tmp = pmalloc(sizeof(m->values) * m->allocd_col);
		memcpy(m->values, tmp, sz_prev);
		pfree(m->values);
		m->values = tmp;
//            m->col_ind = realloc(m->col_ind, sizeof(m->col_ind) * (m->allocd_col += MCRS_ALLOC_BLOCK));
//            m->values = prealloc(m->values, sizeof(m->values) * (m->allocd_col));

            logd(LOGD_H, " col_ind, values reallocd to %d\n", m->allocd_col);
        }

        if ( m->col_ind == NULL ) {
            logd_e("ERROR: col_ind = null sz_col=%d\n", m->sz_col);
            return MCRS_ERR_UNABLE_TO_ALLOC;
        }
        if ( m->values == NULL ) {
            logd_e("ERROR: values = null sz_col=%d\n", m->sz_col);
            return MCRS_ERR_UNABLE_TO_ALLOC;
        }

        ci = m->sz_col;
        m->col_ind[ci] = x;

        m->sz_col++;

        newcol = 1;
    }

    if ( md == MCRS_SET || newcol )
        m->values[ci] = val;
    else {
        if ( md == MCRS_ADD )
            m->values[ci] = m->values[ci] + val;
        else if ( md == MCRS_SUB )
            m->values[ci] -= val;
        else
            return MCRS_ERR_INVALID_MODE;
    }

    return MCRS_ERR_NONE;
}

extern mcrs_err mcrs_f_load(matrix_crs_f *m, const char *path, const char col, const char row) {
    FILE *f = fopen(path, "rb");

    if ( !f ) {
        return MCRS_ERR_UNABLE_TO_OPEN_FILE;
    }

    mcrs_err e;

//      char *line = malloc(sizeof(char) * 200);
    char line[1024];
    char sep1[8] = ',';
    char sep2[8] = '\n';
    size_t *n;
    ssize_t sz;

    char *el1 = 0;
    char *el2 = 0;
    int i1 = 0;
    int i2 = 0;

    size_t count = 0;

    //while((sz = getline(&line, n, f)) != -1) {
    while (fgets(&line, 1024, f) != NULL) {
        el1 = strtok(line, &sep1);
        el2 = strtok(NULL, &sep2);

        if ( el2 == NULL )
            return MCRS_ERR_INVALID_FILE;

        i1 = atof(el1);
        i2 = atof(el2);

        if ( (e = mcrs_f_set(m, i2, i1, 1, MCRS_ADD)) != MCRS_ERR_NONE ) {
            logd_e(" An error occured while setting i1(row)=%d i2(col)=%d: %d\n", i1, i2, e);
            return e;
        }

        //count++;
    }

//  free(line);
    return MCRS_ERR_NONE;
}
