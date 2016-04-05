#include "algorithm.h"
#include <mnemosyne.h>
#include <mtm.h>
#include <pmalloc.h>

//#ifndef PMALLOC
//#define pmalloc(sz) malloc(sz)
//#endif

pthread_mutex_t m_merge;
pthread_mutex_t m_log;

void *mcrs_gmatrix_worker(void* param) {
	thd_data_t* data = (thd_data_t*)param;
//  fprintf(stderr, "thread id %d\n", data->thread_id);
	data->e = mcrs_gmatrix_mult_vector_f_rng(data->loc, data->m, data->v, data->rstart, data->rend, 1);

	//pthread_mutex_lock(&m_log);
	//logd(data->lvl, "%d:%lu ", data->thread_id, timer_total_ms(data->tmr));
	//pthread_mutex_unlock(&m_log);
	
	size_t i;

    MNEMOSYNE_ATOMIC{
//      pthread_mutex_lock(&m_merge);
        for(i = 0; i < data->m->sz_row; i++) {
            data->out->elements[i] += data->loc->elements[i];
            data->loc->elements[i] = 0.0;
//          fprintf(stderr, "%d updating %d\n", data->thread_id, i);
        }
//      pthread_mutex_unlock(&m_merge);
    }
	
	pthread_exit(NULL);
}

extern mcrs_err mcrs_gmatrix_mult_vector_f_mt(logd_lvl_t lvl, vector_f *out, const matrix_crs_f *m, const vector_f *v, const size_t n_threads, const size_t n_iterations) {
	pthread_t threads[n_threads];
	thd_data_t data[n_threads];
	
	size_t n, i, sz = mcrs_f_get_size(m), assigned = 0, length;
	
	l_clock_t* tmr = timer_alloc();
	l_time_t time;
	
	logd(lvl, "Initializing threads (n=%d)...", n_threads);
	
	timer_start(tmr);

	pthread_mutex_init(&m_merge, NULL);
	pthread_mutex_init(&m_log, NULL);
	
	for(i = 0; i < n_threads; i++) {
		data[i].thread_id = i;
		data[i].e = MCRS_ERR_NONE;
		
		data[i].lvl = lvl;
		data[i].tmr = tmr;
		
		data[i].out = out;
        	
		//printf("allocating sz %zu for thread %d/%d\n", sizeof(vector_f) * sz, i, n_threads);
	    data[i].loc = pmalloc(sizeof(vector_f) * sz);
		
		vector_f_init(data[i].loc, sz);
		
		data[i].m = m;
		data[i].v = v;
		
		length = (sz - assigned) / (n_threads - i);
		
		data[i].rstart = assigned;
		data[i].rend = (assigned + length);
		
		assigned += length;
	}
	
	vector_f* tmp = out;
	
	time = timer_lap_ms(tmr);

	logd(lvl, " Done in %ld ms.\n", time);

	for(n = 0; n < n_iterations; n++) {
		for(i = 0; i < sz; i++) {
                        tmp->elements[i] = m->empty;//0.0;
                }
		
		logd(lvl, "(");
	
		for(i = 0; i < n_threads; i++) {
//          fprintf(stderr, "Creating %d thread\n", i);
			if(pthread_create(&threads[i], NULL, mcrs_gmatrix_worker, (void*)&data[i])) {
				logd_e("ERR CREATING THREADS\n");
				return MCRS_ERR_FAILED_CREATING_THREADS;
			}
		}
		
		//mcrs_err* e;
		
		for(i = 0; i < n_threads; i++) {
			if(pthread_join(threads[i], NULL)) {
				logd_e("ERR JOINING THREADS\n");
				return MCRS_ERR_FAILED_JOINING_THREADS;
			}
			
			tmp = data[i].v;
			data[i].v = data[i].out;
			data[i].out = tmp;
		}
		
		logd(lvl, ")\t");
		
		time = timer_lap_ms(tmr);
	
  		logd(lvl, "%d\t", n);
		logd(LOGD_X, "%ld", time);
		logd(lvl, "\tms");
		logd(LOGD_X, "\n");
		
		//if((*e) != MCRS_ERR_NONE) {
		//	return (*e);
		//}
	}
	
	for(i = 0; i < n_threads; i++) {
                vector_f_free(data[i].loc);
		
		pfree(data[i].loc);
	}
	
	return MCRS_ERR_NONE;
}

extern mcrs_err mcrs_gmatrix_mult_vector_f(vector_f *out, const matrix_crs_f *m, const vector_f *v) {
	size_t i;
	for(i = 0; i < m->sz_row; i++)
		v->elements[i] = m->empty;
	
	return mcrs_gmatrix_mult_vector_f_rng(out, m, v, 0, mcrs_f_get_size(m), 0);
}

mcrs_err mcrs_gmatrix_mult_vector_f_rng(vector_f *out, const matrix_crs_f *m, const vector_f *v, const size_t rstart, const size_t rend, const int thd_enabled) {
	if(out == NULL || m == NULL || v == NULL) { 
		logd_e(" Invalid parameters (NULL)!\n");
		return MCRS_ERR_INVALID_PARAMS;
	}
	
	size_t sz = mcrs_f_get_size(m);
	
	if(out->size != sz || v->size != sz) {
		logd_e(" Invalid parameters (Invalid size)!\n");
		return MCRS_ERR_INVALID_PARAMS;
	}
	
        f_t sum = 0;
	size_t i;
	size_t j, ri, ri_n;
	
	size_t bl = 10000, bl_nxt = 0;

	for(i = rstart; i < sz && i < rend; i++) { // rows
		ri = m->row_ptr[i];
		ri_n = ((i + 1) < sz ? m->row_ptr[i + 1] : m->sz_col);

		if(i == bl_nxt) {
			logd(LOGD_ALL, " TX:%d\t of %d\tdone...\n", bl_nxt, sz);
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

	size_t invalid = 0, processed = 0, skipped = 0, sz = mcrs_f_get_size(m), i, j, ri, ri_n;
	
	for(i = 0; i < sz && i < m->sz_row; i++) {
		sum = 0.0;
		ri = m->row_ptr[i];
		ri_n = (i + 1 < m->sz_row ? m->row_ptr[i + 1] : m->sz_col);
		
		if(ri < ri_n) {
			for(j = ri; j < ri_n; j++) {
				sum += m->values[ri];
			}
			
			sum += (sz - (ri_n - ri)) * m->empty;
			
			highest = (highest < sum ? sum : highest);
			lowest = (lowest > sum ? sum : lowest);
		
			if(isnan(highest) || isnan(lowest) || isinf(highest) || isinf(lowest)) {
				logd_e(" Invalid sum in row=%lu val[row][0]=%f (highest=%f lowest=%f)\n", i, m->values[ri], highest, lowest);
				return MCRS_ERR_FAILED_INTEGRITY_CHECK;
			}
	
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

extern void gen_link_vector_crs(vector_i *linkv, int* empty, const matrix_crs_f *adjm)
{
        assert(linkv != NULL);
        assert(empty != NULL);
        assert(adjm != NULL);
	
	size_t sz = mcrs_f_get_size(adjm);
	
        assert(linkv->size == sz);

        (*empty) = 0;

        size_t i, j, ri_n, diff;
        for(i = 0; i < sz && i < adjm->sz_row; i++) {
                linkv->elements[i] = 0;

                ri_n = (i + 1 < adjm->sz_row ? adjm->row_ptr[i + 1] : adjm->sz_col);

                for(j = adjm->row_ptr[i]; j < ri_n; j++)
                        linkv->elements[i] += adjm->values[j];

                if(linkv->elements[i] == 0)
                        (*empty)++;
        }
}

extern void gen_google_matrix_crs(matrix_crs_f *m, const vector_i *linkv, const float damping_factor) {
        assert(m != NULL);
        assert(linkv != NULL);

        fprintf(stderr, "gen_google_matrix_crs, m %p\n", m); 
	
	size_t sz = mcrs_f_get_size(m);
	
        assert(sz == linkv->size);

        m->empty = damping_factor / (f_t)sz;

        size_t i, j, rp_n;
        for(i = 0; i < sz && i < m->sz_row; i++) { // loop through rows
                rp_n = (i + 1 < m->sz_row ? m->row_ptr[i + 1] : m->sz_col);
                for(j = m->row_ptr[i]; j < rp_n; j++) {// loop through columns
                        m->values[j] = (1.0 - damping_factor) * m->values[j] * 1.0 / linkv->elements[i];
                }
        }
	
	size_t sz_row_old = m->sz_row;
	size_t sz_col_old = m->sz_col;
	
	for(i = m->sz_row; i < sz; i++) {

		mcrs_f_set(m, 0, i, m->empty + 1, MCRS_SET);
		
		m->row_ptr[i] = sz_col_old;
	}
	
	m->sz_col = sz_col_old;
}
