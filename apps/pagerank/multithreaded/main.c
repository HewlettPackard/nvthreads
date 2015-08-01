#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <logger.h>
#include <vector.h>
#include <crsmatrix.h>
#include <algorithm.h>

const double DAMPING_FACTOR = 0.1;

const int P_MIN = 6;

const int P_FIN = 1;
const int P_NIT = 2;
const int P_NTHD = 3;
const int P_INITNL = 4;
const int P_FOUT = 5;

int main(int argc, char** argv) {
	mcrs_err e;
	
	logd_set_level(LOGD_X);

	if(argc != P_MIN)
        {
                logd_e("Invalid arguments!\nUsage: pagerank <inputfile> <n_iterations> <n_threads> <n_lines> [<outputfile>]\n");
                return 1;
        }

	char *fin = argv[P_FIN];
        const int iterations = atoi(argv[P_NIT]);
        size_t n_threads = atoi(argv[P_NTHD]);
	size_t n_vals = atoi(argv[P_INITNL]);

        int save_res = 0;
        char *fout;
        if(argc > P_FOUT) {
                save_res = 1;
                fout = argv[P_FOUT];
        }

        assert(n_threads > 0);

        srand(time(0));

	// allocate one timer
	logd(LOGD_M, "Initializing timer\n");
	l_clock_t* tmr = timer_alloc();
	l_time_t time;

	// get initial time
	logd(LOGD_H, "Start loading adj. matrix from file... \n");
	timer_start(tmr);

	//printf("allocating %zu bytes\n", sizeof(matrix_crs_f)); 
	
	matrix_crs_f *adjm = malloc(sizeof(matrix_crs_f));
	mcrs_f_init(adjm, 0.0, n_vals);

	//matrix_i *adjm = (matrix_i*)malloc(sizeof(matrix_i));
	//matrix_i_init_set(adjm, 1, 0);
	//matrix_i_realloc(adjm, 90000, 0);
	//if(matrix_i_load(fin, adjm, ',', '\n')) {
	if((e = mcrs_f_load(adjm, fin, ',', '\n')) != MCRS_ERR_NONE) {
		logd_e("Unable to load file (CODE %d). Aborting.\n", e);

		return 2;
	}

	logd(LOGD_H, "(Done loading)\n");

	// time matrix load
	time = timer_total_ms(tmr);

	logd(LOGD_H, "finished in %ld ms.\n", time);

	// reset timer
	timer_start(tmr);
    	
	//printf("allocating %zu bytes\n", sizeof(vector_i));
	
	vector_i *linkv = (vector_i*)malloc(sizeof(vector_i));
	vector_i_init_set(linkv, adjm->n_row, 0);
	int empty;
	
	gen_link_vector_crs(linkv, &empty, adjm);

	// time linkvector gen
	time = timer_total_ms(tmr);

	logd(LOGD_H, "Link vector generated in %ld ms.\n", time);

	// reset timer
	timer_start(tmr);

	//matrix_f *gm = (matrix_f*)malloc(sizeof(matrix_f));
	//matrix_f_init_set(gm, adjm->size, 0.0);
	gen_google_matrix_crs(adjm, linkv, DAMPING_FACTOR);
	//matrix_f_transpose(gm);

	// time gmatrix gen
	time = timer_total_ms(tmr);

	logd(LOGD_H, "Google matrix generated in %ld ms.\n", time);
	logd(LOGD_H, " n_row=%lu n_col=%lu sz_row=%lu sz_col=%lu empty=%lu\n", 
		adjm->n_row, adjm->n_col, adjm->sz_row, adjm->sz_col, adjm->empty);

	// reset timer
	timer_start(tmr);

	// check integrity of gmatrix
	check_gmatrix_integrity(LOGD_H, adjm);
	
	time = timer_total_ms(tmr);
	
	logd(LOGD_H, "Integrity-check executed in %ld ms.\n", time);

	// reset timer
	timer_start(tmr);
	
    	//printf("allocating %zu bytes\n", sizeof(vector_f)); 
	
	vector_f *init = malloc(sizeof(vector_f));
	vector_f_init_set(init, adjm->n_row, 1.0 / adjm->n_row);
	
	//printf(" -> init=%f*%lu=%f\n", init->elements[0], adjm->n_row, init->elements[0] * adjm->n_row);

	// time init resultvec gen
	time = timer_total_ms(tmr);

	logd(LOGD_H, "Initial result vector generated in %ld ms.\n", time);

    	//printf("allocating %zu bytes\n", sizeof(vector_f)); 
	
	vector_f *res = (vector_f*)malloc(sizeof(vector_f));
	vector_f_init_set(res, adjm->n_row, 0.0);

	int ct = (iterations > 10) ? 10 : iterations;

	timer_start(tmr);

	vector_f *tmp;

	logd(LOGD_H, "Start multiplying...\n");

	if((e = mcrs_gmatrix_mult_vector_f_mt(LOGD_H, res, adjm, init, n_threads, iterations)) != MCRS_ERR_NONE) {
		logd_e("ERROR while multiplying (CODE %d). Aborting.\n", e);
		return 4;
	}

	time = timer_total_ms(tmr);
	logd(LOGD_H, " Multiplying done in %ld ms\n", time);

	if(save_res) {
		timer_start(tmr);
		
		vector_f_save(fout, res);
		
		time = timer_total_ms(tmr);
		logd(LOGD_H, "Saved result file '%s' in %ld ms.\n", fout, time);
	}
	else {
		logd(LOGD_M, "Skipped dumping result.\n");
	}
	
	return 0;
}

