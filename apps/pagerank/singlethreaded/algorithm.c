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

#include <algorithm.h>

extern size_t calculate_links(const matrix_f *m, size_t row)
{
	assert(m != NULL);
	assert(row <= m->size);

	size_t counter = 0;
	for (size_t i = 0; i < m->size; ++i)
	{
		counter += m->elements[i][row]; 
	}

	if(counter == 0)
	{
		return m->size;
	}
	assert(counter <= m->size);

	return counter;
}

extern float calculate_probability(const matrix_f *m, size_t i, size_t j)
{
	assert(m != NULL);
	float p = 1.0;
	size_t r = calculate_links(m, i);

	float a = (p * m->elements[i][j] / r) + ((1 - p) / m->size);

	return a;
}

extern float calculate_probability_i(const matrix_i *adjm, const vector_i *linkv, size_t row, size_t col, const float damping_factor)
{
	assert(adjm != NULL);
	assert(linkv != NULL);

	return ((1 - damping_factor) * adjm->elements[row][col] / linkv->elements[row]) + (damping_factor / adjm->size);
}

// Generates a random adj-matrix
extern void gen_web_matrix(const matrix_f *m)
{
	assert (m != NULL);
	for (size_t i = 0; i < m->size; ++i)
	{
		for(size_t j = 0; j < m->size; ++j)
		{
			m->elements[i][j] = rand() % 2;
		}
	}
}

extern void gen_link_vector(vector_i *linkv, const matrix_i *adjm)
{
	assert(linkv != NULL);
	assert(adjm != NULL);
	assert(linkv->size == adjm->size);

	for (size_t i = 0; i < adjm->size; i++) {
		linkv->elements[i] = 0;

		for (size_t j = 0; j < adjm->size; j++) {
			linkv->elements[i] += adjm->elements[i][j];
		}

		if(linkv->elements[i] == 0) {
			linkv->elements[i] = adjm->size;
		}
	}
}

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

extern void* parallel_calculate(void* _parallel_info)
{
	assert(_parallel_info != NULL);

	parallel_info* info = (parallel_info*)_parallel_info;

	assert(info->in != NULL);
	assert(info->out != NULL);

	size_t n_threads = info->n_threads;

	size_t id = info->id;
	size_t size = info->size;
	size_t slice = size / n_threads;

	//assert(size % n_threads == 0);

	for (size_t i = 0; i < size; ++i)
	{
		for(size_t j = slice * id; j < slice * id + slice; ++j)
		{
			info->out->elements[i][j] = calculate_probability(info->in, i, j);
		}
	}

	free(_parallel_info); //_parallel_info has been alocated in gen_google_matrix

	return NULL;
}

extern size_t g_n_threads;
extern void gen_google_matrix(matrix_f *a, matrix_f *m)
{
	assert(a != 0);
	assert(m != 0);
	assert(a->size == m->size);

	size_t n_threads = g_n_threads;

	pthread_t* callThd = (pthread_t*)malloc(sizeof(pthread_t) * n_threads);

	for(size_t x = 0; x < n_threads; ++x)
	{
		parallel_info* info = (parallel_info*)malloc(sizeof(parallel_info));
		info->n_threads = n_threads;
		info->size = a->size;
		info->in = m;
		info->out = a;
		info->id = x;

		int ret = pthread_create(&callThd[x], NULL, parallel_calculate, (void*)info);
		assert(ret == 0);
	}

	int status = 0;
	for(size_t x = 0; x < n_threads; ++x)
	{
		int ret = pthread_join(callThd[x], (void**)&status);
		assert(ret == 0);
		assert(status == 0);
	}

	free(callThd);
}

extern void gen_google_matrix_i(matrix_f *gm, matrix_i *adjm, vector_i *linkv, const float damping_factor)
{
	assert(gm != 0);
	assert(adjm != 0);
	assert(linkv != 0);
	assert(gm->size == adjm->size);
	assert(linkv->size == adjm->size);
	
	for(size_t i = 0; i < gm->size; i++) {
		for(int j = 0; j < gm->size; j++) {
			gm->elements[i][j] = calculate_probability_i(adjm, linkv, i, j, damping_factor);
		}
	}
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

extern void matrix_solve(vector_f *v, const matrix_f *m)
{
	matrix_f_solve(v, m);
}

extern void matrix_f_solve(vector_f *v, const matrix_f *m)
{
	assert(v != NULL);
	assert(m != NULL);
	assert(v->size == m->size);

	size_t size = m->size;

	vector_f tmp_vec;
	vector_f_init_set(&tmp_vec, size, 0.0);

	for (size_t i = 0; i < size; ++i)
	{
		v->elements[i] = 1.0;
		//tmp_vec.elements[i] = 0.0;
	}

	for(size_t x = 0; x < 3; ++x)
	{
		matrix_f_multiply_vector_f(&tmp_vec, m, v);

		vector_f_normalize(&tmp_vec);

		for (size_t i = 0; i < size; ++i)
		{
			v->elements[i] = tmp_vec.elements[i];
			tmp_vec.elements[i] = 0.0;
		}
	}
	vector_f_free(&tmp_vec);
}

extern void page_rank(size_t size)
{
	matrix_f w;
	matrix_f_init(&w, size);
	gen_web_matrix(&w);

	matrix_f g;
	matrix_f_init(&g, size);
	gen_google_matrix(&g, &w);

	matrix_f_free(&w);

	vector_f p;
	vector_f_init(&p, size);

	matrix_f_transpose(&g);

	matrix_f_solve(&p, &g);

	vector_f_sort(&p);

	vector_f_save("page_rank.txt", &p);

	vector_f_free(&p);
	matrix_f_free(&g);
}

