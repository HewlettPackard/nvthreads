#include <cstdlib>
#include <string>
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <limits>
#include <execinfo.h>

#include "logger.h"

#define USE_LOCKS

typedef int i_t;
typedef struct vec_i {
	size_t size;
	i_t* el;
} vec_i;
typedef struct vec2_i {
	size_t size;
	vec_i** el;
} vec2_i;

typedef double f_t;
typedef struct vec_d {
	size_t size;
	f_t* el;
} vec_d;
typedef struct vec2_d {
	size_t size;
	vec_d** el;
} vec2_d;

const double POS_INF = std::numeric_limits<double>::infinity();

/* ==== CONSTRUCTOR-FUNCTIONS ==== */

vec_i* construct_vec_i(size_t sz, i_t init) {
	vec_i* res = (vec_i*)malloc(sizeof(res));
	res->size = sz;
	res->el = (i_t*)malloc(sizeof(res->el) * sz);
	
	size_t i;
	for(i = 0; i < sz; i++) {
		res->el[i] = init;
	}
	
	return res;
}

vec2_i* construct_vec2_i(size_t szx, size_t szy, i_t init) {
	vec2_i* res = (vec2_i*)malloc(sizeof(res));
	res->size = szx;
	res->el = (vec_i**)malloc(sizeof(res->el) * szx);

	size_t i;
	for(i = 0; i < szx; i++) {
		res->el[i] = construct_vec_i(szy, init);
	}

	return res;
}

vec_d* construct_vec_d(size_t sz, f_t init) {
        vec_d* res = (vec_d*)malloc(sizeof(res));
        res->size = sz;
        res->el = (f_t*)malloc(sizeof(res->el) * sz);

        size_t i;
        for(i = 0; i < sz; i++) {
                res->el[i] = init;
        }
	
	return res;
}

vec2_d* construct_vec2_d(size_t szx, size_t szy, f_t init) {
        vec2_d* res = (vec2_d*)malloc(sizeof(res));
        res->size = szx;
        res->el = (vec_d**)malloc(sizeof(res->el) * szx);

        size_t i;
        for(i = 0; i < szx; i++) {
                res->el[i] = construct_vec_d(szy, init);
        }

        return res;
}

/* ==== STRUCTS AND STUFF ==== */

struct t_data {
	int thread_id;

	l_clock_t* tmr;

	vec2_d *x;

	int x_start;
	int x_length;

	vec_d *x_norms;

	vec_i *lbls_loc;
	vec_i *lbls;

	vec2_d *c;

	int c_n_start;
	int c_n_length;

	vec_d *c_norms;

	vec2_d *c_sum_loc;
	vec2_d *c_sum;

	vec_i *c_count_loc;
	vec_i *c_count;
};

/* ==== FUNCTIONS FOR DATA-LOADING AND -SAVING ==== */

void load_data(vec2_d* res, const char* path, const char delim) {
	std::ifstream data_file;
	std::string line;
	std::string itm;

	data_file.open(path);

	size_t i = 0, j = 0;

	if(data_file.is_open()) {
		while(std::getline(data_file, line) && i < res->size) {
			std::stringstream ss(line);
			j = 0;
			
			while(std::getline(ss, itm, delim) && j < res->el[i]->size) {
				res->el[i]->el[j] = std::atof(itm.c_str());
				
				j++;
			}
			
			i++;
		}

		data_file.close();
	}
}

void dump_data(vec_i* data, const char* path, const char delim) {
	std::ofstream data_file;

	data_file.open(path);

	if(data_file.is_open()) {
		for(int i = 0; i < data->size; i++) {
			data_file << data->el[i] << (i == data->size - 1 ? ' ' : delim);
		}
	}

	data_file.close();
}

void dump_data(vec2_d* data, const char* path, const char delim) {
	std::ofstream data_file;

	data_file.open(path);

	if(data_file.is_open()) {
		for(int i = 0; i < data->size; i++) {
			for(int j = 0; j < data->el[i]->size; j++) {
				data_file << data->el[i]->el[j] << (j == data->el[i]->size - 1 ? ' ' : delim);
			}

			data_file << "\n";
		}
	}

	data_file.close();
}

/* ==== SOME HELPER-FUNCTIONS ==== */

void calc_norm(vec_d* norms, const vec2_d *x) {
	if(norms->size != x->size) return;
	
	for(int i = 0; i < x->size; i++) {
		for(int j = 0; j < x->el[i]->size; j++)
			norms->el[i] += x->el[i]->el[j] * x->el[i]->el[j];

		norms->el[i] = sqrt(norms->el[i]);
	}
}

// balances the workload: x = no. of datapoints, t = no. of threads
// returns an vector of 2-dim-arrays for each thread
void balance_x(int x, const int t, int *res) {
	int assigned = 0;

	for(int i = 0; i < t; i++) {
		int length = ((x - assigned) / (t - i));

		int start = assigned;

		(*(res + i * 2 + 0)) = start;
		(*(res + i * 2 + 1)) = length;

		assigned += length;
	}
}

vec2_i* balance_x(int x, const int t) {
	int assigned = 0;

	vec2_i *res = construct_vec2_i(t, 2, 0);

	for(int i = 0; i < t; i++) {
		int length = ((x - assigned) / (t - i));

		int start = assigned;

		res->el[i]->el[0] = start;
		res->el[i]->el[1] = length;

		assigned += length;
	}

	return res;
}

bool stob(std::string s, std::string s_t, std::string s_f) {
	if(s == s_t)
		return true;
	else if(s == s_f)
		return false;
	else
		return false;
}

/* ==== MULTI-THREADED K-MEANS ==== */

pthread_mutex_t mutex_lbls;

void *kmeans_worker(void *p_data) {
	t_data *data = (t_data*)p_data;
	
	#ifdef ENABLE_PROFILING
	logd(LOGD_X, "%d\tspawned\t%lu\n", data->thread_id, timer_total_ms(data->tmr));
	#endif

	//int x_sz = (*(*data).x).size();
	int c_sz = data->c->size;
	int v_sz = data->x->el[0]->size;

	double best;
	double appD;
	double dd;
	double tmp_d;
	int inew = 0;

	// find the closest center within the threads range of data
	for(int i = data->x_start; i < (data->x_start + data->x_length); i++) {
		// find the closest center
		best = POS_INF;

		for(int n = 0; n < c_sz; n++) {
			appD = data->x_norms->el[i] - data->c_norms->el[n];
			appD *= appD;

			if(best < appD)
				continue;

			dd = 0.0;

			for(int j = 0; j < v_sz; j++) {
				tmp_d = data->x->el[i]->el[j] - data->c->el[n]->el[j];

				dd += tmp_d * tmp_d;
			}

			if(dd < best) {
				best = dd;
				inew = n;
			}
		}

		data->lbls_loc->el[i] = inew;

		data->c_count_loc->el[inew]++;

		for(int j = 0; j < v_sz; j++) {
			data->c_sum_loc->el[inew]->el[j] += data->x->el[i]->el[j];
		}
	}
	
	#ifdef ENABLE_PROFILING
	logd(LOGD_X, "%d\tcalcd\t%lu\n", data->thread_id, timer_total_ms(data->tmr));
	#endif
	
	#ifdef USE_LOCKS
	pthread_mutex_lock(&mutex_lbls);
	
	#ifdef ENABLE_PROFILING
	logd(LOGD_X, "%d\tlocked\t%lu\n", data->thread_id, timer_total_ms(data->tmr));
	#endif
	
	#endif

	for(int i = data->x_start; i < (data->x_start + data->x_length); i++)
		data->lbls->el[i] = data->lbls_loc->el[i];
	
	#ifdef ENABLE_PROFILING
	logd(LOGD_X, "%d\tmerged\t%lu\n", data->thread_id, timer_total_ms(data->tmr));
	#endif
	
	#ifdef USE_LOCKS
	pthread_mutex_unlock(&mutex_lbls);
	
	#ifdef ENABLE_PROFILING
        logd(LOGD_X, "%d\tunlockd\t%lu\n", data->thread_id, timer_total_ms(data->tmr));
        #endif

	#endif

	pthread_exit(NULL);
}

void spawn_threads(int NUM_THREADS, vec2_d *x, vec_d *x_norms,
		vec2_d *centers, int iterations, vec_i *lbls) {

	l_clock_t* tmr = timer_alloc();
	
	timer_start(tmr);
	
//	clock_start();

	pthread_t threads[NUM_THREADS];

	pthread_mutex_init(&mutex_lbls, NULL);

	int x_sz = x->size;
	int v_sz = x->el[0]->size;
	int c_sz = centers->size;

	// balance load
	int bal_x[NUM_THREADS][2];
	int bal_c[NUM_THREADS][2];

	balance_x(x_sz, NUM_THREADS, &(bal_x[0][0]));
	balance_x(c_sz, NUM_THREADS, &(bal_c[0][0]));

	// init local copies
	t_data* data[NUM_THREADS];
	t_data* d;

	vec_d* c_norms = construct_vec_d(c_sz, 0);

	vec2_d* c_sum = construct_vec2_d(c_sz, v_sz, 0);
	vec_i* c_count = construct_vec_i(c_sz, 0);

	for(int i = 0; i < NUM_THREADS; i++) {
		d = (t_data*)malloc(sizeof(t_data));

		d->thread_id = i;

		d->tmr = tmr;
		
		d->x = x;
		d->x_start = bal_x[i][0];
		d->x_length = bal_x[i][1];

		d->x_norms = x_norms;

		d->lbls_loc = construct_vec_i(x_sz, 0);
		d->lbls = lbls;

		d->c = centers;

		d->c_norms = c_norms;

		d->c_n_start = 0;
		d->c_n_length = 0;

		d->c_sum_loc = construct_vec2_d(c_sz, v_sz, 0);
		d->c_sum = c_sum;

		d->c_count_loc = construct_vec_i(c_sz, 0);
		d->c_count = c_count;

		data[i] = d;
	}

//	clock_stop();
	
	logd(LOGD_H, " Thread-init done in %ld ms.\n", timer_total_ms(tmr));

/*	log(LOG_TIME, " Thread-initialization done in ");
	log(LOG_TIME, clock_get_duration());
	log(LOG_TIME, " ms.\n");*/

	for(int _it = 0; _it < iterations; _it++) {
		//clock_start();
		timer_start(tmr);

		// reset sum and counters
		for(int i = 0; i < c_sz; i++) {
			for(int j = 0; j < v_sz; j++) {
				c_sum->el[i]->el[j] = 0.0;
			}

			c_count->el[i] = 0;
		}

		// calculate center-norms
		for(int i = 0; i < c_sz; i++) {
			c_norms->el[i] = 0;

			for(int j = 0; j < v_sz; j++) {
				c_norms->el[i] += centers->el[i]->el[j] * centers->el[i]->el[j];
			}

			c_norms->el[i] = sqrt(c_norms->el[i]);
		}

		// spawn threads
		for(int t = 0; t < NUM_THREADS; t++) {
			pthread_create(&(threads[t]), NULL, kmeans_worker, data[t]);
		}

		for(int t = 0; t < NUM_THREADS; t++) {
			pthread_join(threads[t], NULL);
		}

		// merge sums and count
		for(int i = 0; i < c_sz; i++) {
			for(int t = 0; t < NUM_THREADS; t++) {
				// sum intermediates
				c_count->el[i] += data[t]->c_count_loc->el[i];

				// reset local
				data[t]->c_count_loc->el[i] = 0;
			}

			for(int j = 0; j < v_sz; j++) {
				for(int t = 0; t < NUM_THREADS; t++) {
					// sum intermediates
					c_sum->el[i]->el[j] += data[t]->c_sum_loc->el[i]->el[j];

					// reset local
					data[t]->c_sum_loc->el[i]->el[j] = 0;
				}

				centers->el[i]->el[j] = c_sum->el[i]->el[j] / c_count->el[i];
			}
		}
		
		logd(LOGD_H, " %ld\n", timer_total_ms(tmr));
		
/*		clock_stop();

		log(LOG_TIME, " ");
		log(LOG_TIME, _it);
		log(LOG_TIME, "\t ");
		log(LOG_TIME, clock_get_duration());
		log(LOG_TIME, " ms\n");*/
	}
}

const int MIN_ARGC = 8;

const int P_PNAME = 0;
const int P_F_X = 1;
const int P_F_C = 2;
const int P_N_TH = 3;
const int P_N_IT = 4;
const int P_N_LX = 5;
const int P_N_LC = 6;
const int P_N_DIM = 7;
const int P_F_LBLS = 8;

const int MAX_ARGC = MIN_ARGC + 1;

int main(int argc, char* argv[]) {
//	log_set_min(LOG_ALL);
	logd_init(LOGD_H, 200);
	
	l_clock_t* tmr = timer_alloc();
	
	if(argc < MIN_ARGC || argc > MAX_ARGC) {
/*		log_e(LOG_ALL, "\nInvalid arguments! Usage:\n");
		log_e(LOG_ALL, argv[P_PNAME]);
		log_e(LOG_ALL, " <x-input-file> <centers-input-file> "
				"<threads> <iterations> <lines_x> <lines_c> "
				"<dimensions> [<lbl-output-file>]\n");*/
		logd_e(	"\nInvalid arguments! Usage:\n"
			" kmeans <x-input-tile> <centers-input-file> <threads> <iterations>"
			" <lines-x> <lines-c> <dimensions> [<lbl-output-file>]\n");
		
		return 1;
	}

	/*int o_logm = std::atoi(argv[P_O_LOG]);

	switch(o_logm) {
		case 0: log_set_min(LOG_ALL); break;
		case 1: log_set_min(LOG_TIME); break;
		case 2: log_set_min(LOG_ERR); break;
		case 3: log_set_min(LOG_NONE); break;
		default: log_set_min(LOG_ALL);
	}*/

	logd(LOGD_L, "\n"
			"----------------------------------------------------\n"
			"    K-MEANS, Multithreaded (Helge Bruegner 2015)    \n"
			"----------------------------------------------------\n\n");

	const char* f_x = argv[P_F_X];
	const char* f_c = argv[P_F_C];
	int n_threads = std::atoi(argv[P_N_TH]);
	int n_iterations = std::atoi(argv[P_N_IT]);
	size_t n_lx = std::atoi(argv[P_N_LX]);
	size_t n_lc = std::atoi(argv[P_N_LC]);
	size_t n_dim = std::atoi(argv[P_N_DIM]);
	
	//SyncMethod o_syncm = (SyncMethod)std::atoi(argv[P_O_SYNCM]);

/*	log(LOG_ALL, "Start reading the file \"");
	log(LOG_ALL, f_x);
	log(LOG_ALL, "\"... ");*/
	
	logd(LOGD_H, "Start reading the file '%s'...", f_x);

//	clock_start();
	timer_start(tmr);

	vec2_d* x = construct_vec2_d(n_lx, n_dim, 0);
	load_data(x, f_x, ',');

//	clock_stop();

	logd(LOGD_H, " done in %ld ms.\n", timer_total_ms(tmr));
	
/*	log(LOG_TIME, " X-File loaded in ");
	log(LOG_TIME, clock_get_duration());
	log(LOG_TIME, " ms. ");

	log(LOG_ALL, "Done!");
	log(LOG_TIME, "\n");

	log(LOG_ALL, "Start reading the file \"");
	log(LOG_ALL, f_c);
	log(LOG_ALL, "\"... ");*/
	
	logd(LOGD_H, "Start reading the file '%s'...", f_c);

//	clock_start();
	timer_start(tmr);

	vec2_d* centers = construct_vec2_d(n_lc, n_dim, 0);
	load_data(centers, f_c, ',');

	dump_data(centers, "centers", ',');

/*	clock_stop();

	log(LOG_TIME, " Centers-File loaded in ");
	log(LOG_TIME, clock_get_duration());
	log(LOG_TIME, " ms. ");

	log(LOG_ALL, "Done!");
	log(LOG_TIME, "\n");

	clock_start();*/
	
	logd(LOGD_H, " done in %ld ms.\n", timer_total_ms(tmr));

	logd(LOGD_H, "Calculating the norms and generating initial labels... ");
	
	timer_start(tmr);
	
	vec_d* norms = construct_vec_d(x->size, 0);
	calc_norm(norms, x);
	vec_i* lbls = construct_vec_i(x->size, -1);
	
	logd(LOGD_H, " done in %ld ms.\n", timer_total_ms(tmr));
	
/*	clock_stop();

	log(LOG_TIME, " Calculated norms and generated labels in ");
	log(LOG_TIME, clock_get_duration());
	log(LOG_TIME, " ms. ");

	log(LOG_ALL, "Done!\n");
	log(LOG_TIME, "\n");
	log(LOG_ALL, " Executing k-means with the following configuration:\n"
			"        X-File = \"");
	log(LOG_ALL, f_x);
	log(LOG_ALL, "\" (");
	log(LOG_ALL, (int)x->size);
	log(LOG_ALL, " lines)\n"
			"  Centers-File = \"");
	log(LOG_ALL, f_c);
	log(LOG_ALL, "\" (");
	log(LOG_ALL, (int)centers->size);
	log(LOG_ALL, " lines)\n");
	log(LOG_ALL, "       Threads = ");
	log(LOG_ALL, n_threads);
	log(LOG_ALL, "\n");
	log(LOG_ALL, "    Iterations = ");
	log(LOG_ALL, (int)n_iterations);
	log(LOG_ALL, "\n");
	log(LOG_ALL, "    Dimensions = ");
	log(LOG_ALL, (int)x->el[0]->size);
	log(LOG_ALL, "\n\n");*/

	logd(LOGD_H, "Starting algorithm...\n");

//	clock_start();
	timer_start(tmr);

	spawn_threads(n_threads, x, norms, centers, n_iterations, lbls);
	
	logd(LOGD_H, "Executed algorithm in %ld ms.\n", timer_total_ms(tmr));
	
/*	clock_stop();

	log(LOG_TIME, " Executed the algorithm in ");
	log(LOG_TIME, clock_get_duration());
	log(LOG_TIME, " ms. ");

	log(LOG_ALL, "Done!\n");
	log(LOG_TIME, "\n");*/

	if(argc == MIN_ARGC + 1) {
		const char* f_lbls = argv[P_F_LBLS];

/*		log(LOG_ALL, "Dump labeling to the file [");
		log(LOG_ALL, f_lbls);
		log(LOG_ALL, "]...");*/
		
		logd(LOGD_H, "Dump labeling to the file '%s'...", f_lbls);
		
		dump_data(lbls, f_lbls, ',');
		
		logd(LOGD_H, " done in %ld ms.\n", timer_total_ms(tmr));
		
//		log(LOG_ALL, "Done!\n");
	}
	else {
//		log(LOG_ALL, "Skipped dumping to file. Printing centers:\n");
		
		logd(LOGD_H, "Skipped dumping to a file. Printing out centers:\n");

		for(int i = 0; i < centers->size; i++) {
/*			logd(LOG_M, "[");
			logd(LOG_M, (int)i);
			log(LOG_ALL, "](");*/
			logd(LOGD_M, "[%d](", i);

			for(int j = 0; j < centers->el[i]->size; j++) {
/*				log(LOG_ALL, centers->el[i]->el[j]);
				log(LOG_ALL, (j == centers->el[i]->size - 1 ? "" : ","));*/
				logd(LOGD_M, (j == centers->el[i]->size - 1 ? "%d" : "%d,"), centers->el[i]->el[j]);
			}

//			log(LOG_ALL, ")\n");
			logd(LOGD_M, ")\n");
		}
	}

//	log(LOG_ALL, "\n");
	logd(LOGD_H, "\n");
	
	timer_free(tmr);
	
	return 0;
}
