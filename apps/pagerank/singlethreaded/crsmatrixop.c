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
#include <crsmatrixop.h>

extern mcrs_err mcrs_gmatrix_mult_vector_f(vector_f *out, const matrix_crs_f *m, const vector_f *v, const vector_i *linkv, const int empty) {
	if(out == NULL || m == NULL || v == NULL 
			|| out->size != v->size || out->size != m->n_col 
			|| m->n_col != m->sz_row) {
		logd_e(" Invalid parameters!\n");
		return MCRS_ERR_INVALID_PARAMS;
	}
	
        f_t sum = 0;
	size_t i, sz = m->sz_row;
	size_t j, ri, ri_n;
	
	for(i = 0; i < sz; i++)
		out->elements[i] = m->empty;
	
	size_t bl = 10000, bl_nxt = 0;

	for(i = 0; i < sz; i++) { // rows
		ri = m->row_ptr[i];
		ri_n = ((i + 1) < sz ? m->row_ptr[i + 1] : m->sz_col);

		if(i == bl_nxt) {
			logd(LOGD_L, " %d\t of %d\tdone...\n", bl_nxt, sz);
			bl_nxt += bl;
		}

		if(ri == ri_n) {
			sum += v->elements[i] / sz - v->elements[i] * m->empty;
		}
	
		for(j = ri; j < ri_n; j++) {
			out->elements[m->col_ind[j]] += v->elements[i] * m->values[j];
		}
	}

	for(i = 0; i < sz; i++) {
		out->elements[i] += sum;
	}

	return MCRS_ERR_NONE;
}


extern mcrs_err check_gmatrix_integrity(const logd_lvl_t lvl, const matrix_crs_f *m) {
	f_t highest = 0, lowest = 9999999, sum;

	size_t invalid = 0, processed = 0, skipped = 0, sz = m->sz_row, i, j, ri, ri_n;
	
	for(i = 0; i < sz; i++) {
		sum = 0.0;
		ri = m->row_ptr[i];
		ri_n = (i + 1 < sz ? m->row_ptr[i + 1] : m->sz_col);
		
		if(ri < ri_n) {
			for(j = ri; j < ri_n; j++) {
				sum += m->values[ri];
			}
			
			sum += (sz - (ri_n - ri)) * m->empty;
			
			highest = (highest < sum ? sum : highest);
			lowest = (lowest > sum ? sum : lowest);
			
			if(sum != 1.0) {
				invalid++;
			}
			
			processed++;
		}
		else {
			skipped++;
		}
	}
	
	logd(lvl, "INTEGRITY-CHECK DONE!\n Result:\n total=%d processed=%d skipped(empty)=%d invalid=%d\n lowest=%f highest=%f\n",
			sz, processed, skipped, invalid, lowest, highest);
	
	if(invalid > 0)
		return MCRS_ERR_FAILED_INTEGRITY_CHECK;
	else
		return MCRS_ERR_NONE;
}
