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
#include <utils.h>

extern int float_comparator(const void* a, const void* b)
{
    float* arg1 = (float*) a;
    float* arg2 = (float*) b;

    if(*arg1 < *arg2)
    {
	return -1;
    }
    else if(*arg1 == *arg2)
    {
	return 0;
    }
    else
    {
	return 1;
    }
}

extern int compare_floats(float a, float b)
{
    if (fabs(a - b) < 0.1) // just for unit testing, good enough
    {
	return 1;
    }
    return 0;
}

extern int int_comparator(const void *a, const void *b)
{
    int *arg1 = (int*)a;
    int *arg2 = (int*)b;

    if(*arg1 < *arg2)
        return -1;
    else if(*arg1 == *arg2)
        return 0;
    else
        return 1;
}

extern void matrix_f_display(const logd_lvl_t lvl, const matrix_f *m)
{
        assert(m != NULL);
        for (size_t i = 0; i < m->size; ++i)
        {
                for(size_t j = 0; j < m->size; ++j)
                {
                        logd(lvl, "%.4f ", m->elements[j][i]);
                }
                logd(lvl, "\n");
        }
}

extern void matrix_i_display(const logd_lvl_t lvl, const matrix_i *m)
{
        assert(m != NULL);

        for(size_t i = 0; i < m-> size; i++) {
                for(size_t j = 0; j < m->size; j++) {
                        logd(lvl, "%d ", m->elements[i][j]);
                }
                logd(lvl, "\n");
        }
}

extern void mcrs_i_display(const logd_lvl_t lvl, const matrix_crs_i *m)
{
	assert(m != NULL);
	assert(m->values != NULL);
	assert(m->col_ind != NULL);
	assert(m->row_ptr != NULL);

	logd(lvl, "VALUES:\n");
	for(size_t i = 0; i < m->sz_col; i++)
		logd(lvl, "[%d]%d ", i, m->values[i]);
	logd(lvl, "\n");	
	
	logd(lvl, "COL_IND:\n");
	for(size_t i = 0; i < m->sz_col; i++)
                logd(lvl, "[%d]%d ", i, m->col_ind[i]);
	logd(lvl, "\n");
	
	logd(lvl, "ROW_PTR:\n");
	for(size_t i = 0; i < m->sz_row; i++)
                logd(lvl, "[%d]%d ", i, m->row_ptr[i]);
	logd(lvl, "\n");
}
	
extern void mcrs_f_display(const logd_lvl_t lvl, const matrix_crs_f *m)
{
        assert(m != NULL);
        assert(m->values != NULL);
        assert(m->col_ind != NULL);
        assert(m->row_ptr != NULL);

        logd(lvl, "VALUES:\n");
        for(size_t i = 0; i < m->sz_col; i++)
                logd(lvl, "[%d]%f ", i, m->values[i]);
        logd(lvl, "\n");

        logd(lvl, "COL_IND:\n");
        for(size_t i = 0; i < m->sz_col; i++)
                logd(lvl, "[%d]%d ", i, m->col_ind[i]);
        logd(lvl, "\n");

        logd(lvl, "ROW_PTR:\n");
        for(size_t i = 0; i < m->sz_row; i++)
                logd(lvl, "[%d]%d ", i, m->row_ptr[i]);
        logd(lvl, "\n");
}

extern void vector_f_display(const logd_lvl_t lvl, const vector_f *v)
{
	assert (v != NULL);
	for (size_t i = 0; i < v->size; ++i)
	{
        	    logd(lvl, "%.4f ", v->elements[i]);
    	}
	logd(lvl, "\n");
}

extern void vector_i_display(const logd_lvl_t lvl, const vector_i *v)
{
    	assert(v != NULL);

    	for(size_t i = 0; i < v->size; i++)
    		logd(lvl, "%d ", v->elements[i]);
	
	logd(lvl, "\n");
}

extern void vector_f_sort(vector_f *v)
{
        assert(v != NULL);
        qsort(v->elements, v->size, sizeof(float), float_comparator);
}

extern void vector_i_sort(vector_i *v)
{
    assert(v != NULL);

    qsort(v->elements, v->size, sizeof(int), int_comparator);
}

