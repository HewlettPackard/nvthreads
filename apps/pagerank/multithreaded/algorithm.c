#include <algorithm.h>

extern void gen_link_vector_crs(vector_i *linkv, int* empty, const matrix_crs_f *adjm)
{
	assert(linkv != NULL);
	assert(empty != NULL);
	assert(adjm != NULL);
	assert(linkv->size == adjm->n_row);
	
	(*empty) = 0;

	size_t i, j, ri_n, sz = adjm->sz_row, diff;
	for(i = 0; i < sz; i++) {
		linkv->elements[i] = 0.0;
		
		ri_n = (i + 1 < sz ? adjm->row_ptr[i + 1] : adjm->sz_col);
		
		for(j = adjm->row_ptr[i]; j < ri_n; j++)
			linkv->elements[i] += adjm->values[j];
		
		if(linkv->elements[i] == 0.0)
			(*empty)++;
		//	linkv->elements[i] = adjm->n_row;
	}
	
	printf(" LINKV empty=%d\n", (*empty));
	
	//linkv->elements[i] 
	//		= ((diff = adjm->row_ptr[i + 1] - adjm->row_ptr[i]) == 0 ? sz : diff);
	
	//linkv->elements[sz - 1] = ((diff = adjm->row_ptr[sz - 1] - adjm->sz_row) == 0 ? sz : diff);
}

extern void gen_google_matrix_crs(matrix_crs_f *m, const vector_i *linkv, const float damping_factor) {
	assert(m != NULL);
	assert(linkv != NULL);
	assert(m->n_row == linkv->size);
	assert(m->n_row = m->n_col);
	
	m->empty = damping_factor / (f_t)m->sz_row;

	printf(" m->empty*100000=%f\n", m->empty * 100000);

	//printf(" \nEmpty=%f should be %f/%d=%f\n", m->empty, damping_factor, m->sz_row, damping_factor / m->sz_row);
	
	size_t i, j, rp_n;
	for(i = 0; i < m->n_row; i++) { // loop through rows
		rp_n = (i + 1 < m->n_row ? m->row_ptr[i + 1] : m->sz_col);
		for(j = m->row_ptr[i]; j < rp_n; j++) {// loop through columns
			m->values[j] = (1.0 - damping_factor) * m->values[j] * 1.0 / linkv->elements[i];

			//m->values[j] = (1.0 - damping_factor) * m->values[j] / linkv->elements[i] + m->empty;
		}
	}
}
