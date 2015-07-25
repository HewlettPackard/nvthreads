#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <logger.h>
#include <vector.h>
#include <matrix.h>
#include <crsmatrix.h>
#include <crsmatrixop.h>
#include <algorithm.h>
#include <minunit.h>
#include <utils.h>
#include <tests.h>

static char* all_tests()
{
	mu_run_test(test_compare_floats);

	mu_run_test(test_vector_normalize);
	mu_run_test(test_vector_copy);
	mu_run_test(test_vector_sort);

	mu_run_test(test_matrix_multiply);
	mu_run_test(test_matrix_transpose);
	mu_run_test(test_matrix_solve);

	mu_run_test(test_matrix_multi);
	mu_run_test(test_matrix_vector_multiply);
	
	mu_run_test(test_matrix_compress);

	mu_run_test(test_gen_web_matrix);
	mu_run_test(test_calculate_links);
	mu_run_test(test_calculate_probability);
	mu_run_test(test_full_algorithm);

	return 0;
}

size_t g_n_threads;

const double DAMPING_FACTOR = 0.1;

const int P_MIN = 5;

const int P_FIN = 1;
const int P_NIT = 2;
const int P_NTHD = 3;
const int P_FOUT = 4;

int main(int argc, char** argv)
{
	printf("%d\n", MCRS_ERR_NO_SEQ_ACCESS);
	mcrs_err e;
	
	logd_set_level(LOGD_H);

	char* result = all_tests();
	if (result != 0)
	{
		logd(LOGD_L, "%s\n", result);
	}
	else
	{
		logd(LOGD_L, "ALL TESTS PASSED\n");
	}
	logd(LOGD_L, "Tests run: %d\n", tests_run);
	
	if(argc == 2) {
		logd_set_level(LOGD_ALL);
		
		logd(LOGD_L, "TEST CRS-MATRIX\n");
		logd(LOGD_L, "Testing file load with file '%s'...", argv[1]);
		
		matrix_crs_f *m = malloc(sizeof(matrix_crs_f));
		mcrs_f_init(m, 0);
	
		if((e = mcrs_f_load(m, argv[1], ',', '\n')) != MCRS_ERR_NONE) {
			logd_e("ERROR: %d\n", e);
			
			return 1;
		}
		
		mcrs_f_display(LOGD_L, m);
				
		logd(LOGD_L, " Done!\n");
		logd(LOGD_L, " n_row=%d n_col=%d\n sz_row(row_ptr)=%d sz_col(col_ind,values)=%d\n allocd_row=%d allocd_col=%d\n\n", m->n_row, m->n_col, m->sz_row, m->sz_col, m->allocd_row, m->allocd_col);

		mcrs_f_display(LOGD_L, m);

		vector_i *linkv = malloc(sizeof(vector_i));
		vector_i_init_set(linkv, m->n_row, 0);		
		int empty;
		
		logd(LOGD_L, " Generating linkv...");
		
		gen_link_vector_crs(linkv, &empty, m);
		
		logd(LOGD_L, " Done!\n");
		
		vector_i_display(LOGD_L, linkv);
	
		logd(LOGD_L, " Generating Google-matrix...");
		
		gen_google_matrix_crs(m, linkv, 0.1);
		
		logd(LOGD_L, " Done!\n");
		
		mcrs_f_display(LOGD_L, m);

		logd(LOGD_L, " Converting to matrix...");
		
		matrix_f *gm = malloc(sizeof(matrix_f));
		matrix_f_init_set(gm, m->n_row, 0.0);
		
		matrix_f_from_mcrs_f(gm, m);
		
		logd(LOGD_L, " Done!\n");
		
		matrix_f_display(LOGD_L, gm);		
		
		logd(LOGD_L, " Testing multiplication...");
		
		vector_f *v1 = malloc(sizeof(vector_f));
		vector_f *v2 = malloc(sizeof(vector_f));
		vector_f_init_set(v1, m->n_row, 1.0 / m->n_row);
		vector_f_init_set(v2, m->n_row, 0.0);
		
		//vector_f *tmp;
		
		if((e = mcrs_gmatrix_mult_vector_f(v2, m, v1, linkv, empty)) != MCRS_ERR_NONE) {
			logd_e("ERROR: %d\n", e);
			return 1;
		}
		
		logd(LOGD_L, " Done!\n");
		
		logd(LOGD_L, "INIT-VECTOR\n");
		vector_f_display(LOGD_L, v1);
		
		logd(LOGD_L, "\n");
		
		logd(LOGD_L, "RESULT-VECTOR\n");
		vector_f_display(LOGD_L, v2);
	
		logd(LOGD_L, " Saving result...");
		
		vector_f_save("result", v2);
	
		return 0;
	}
	
	if(argc == 3) {
		logd(LOGD_L, "TEST STD-MATRIX\n");
		logd(LOGD_L, " Loading file... %s...\n", argv[1]);
		
		matrix_i *adjm = malloc(sizeof(matrix_i));
		matrix_i_init_set(adjm, 1, 0);
		
		matrix_i_load(argv[1], adjm, ',', '\n');
		
		logd(LOGD_L, " Done!\n");
		
		matrix_i_display(LOGD_L, adjm);
		
		logd(LOGD_L, " Generating linkv...");
		
		vector_i *linkv = malloc(sizeof(vector_i));
		vector_i_init_set(linkv, adjm->size, 0);
		
		gen_link_vector(linkv, adjm);
		
		logd(LOGD_L, " Done!\n");
		
		vector_i_display(LOGD_L, linkv);
		
		logd(LOGD_L, " Generating Google-matrix...\n");
		
		matrix_f *gm = malloc(sizeof(matrix_f));
		matrix_f_init_set(gm, adjm->size, 0.0);
	
		gen_google_matrix_i(gm, adjm, linkv, 0.1);
	
		matrix_f_transpose(gm);
			
		logd(LOGD_L, " Done!\n");
		
		matrix_f_display(LOGD_L, gm);
		
		return 0;
	}
	
	if(argc != P_MIN)
        {
                logd_e("Invalid arguments!\nUsage: pagerank <inputfile> <n_iterations> <n_threads(ignored)> [<outputfile>]\n");
                return 1;
        }

	char *fin = argv[P_FIN];
        const int iterations = atoi(argv[P_NIT]);
        size_t n_threads = atoi(argv[P_NTHD]);

        int save_res = 0;
        char *fout;
        if(argc > P_FOUT) {
                save_res = 1;
                fout = argv[P_FOUT];
        }

        assert(n_threads > 0);

        g_n_threads = n_threads;
        srand(time(0));

	// allocate one timer
	logd(LOGD_M, "Initializing timer\n");
	struct timespec *tmr = timer_alloc();
	l_time_t time;

	// get initial time
	logd(LOGD_H, "Start loading adj. matrix from file... \n");
	timer_start(tmr);

	matrix_crs_f *adjm = malloc(sizeof(matrix_crs_f));
	mcrs_f_init(adjm, 0.0);

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
	mcrs_f_display(LOGD_M, adjm);
	logd(LOGD_M, "\n");

	// reset timer
	timer_start(tmr);

	vector_i *linkv = (vector_i*)malloc(sizeof(vector_i));
	vector_i_init_set(linkv, adjm->n_row, 0);
	int empty;
	
	gen_link_vector_crs(linkv, &empty, adjm);

	// time linkvector gen
	time = timer_total_ms(tmr);

	logd(LOGD_H, "Link vector generated in %ld ms.\n", time);
	vector_i_display(LOGD_M, linkv);
	logd(LOGD_M, "\n");

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
	mcrs_f_display(LOGD_M, adjm);
	logd(LOGD_M, "\n");

	// reset timer
	timer_start(tmr);

	// check integrity of gmatrix
	check_gmatrix_integrity(LOGD_H, adjm);
	
	time = timer_total_ms(tmr);
	
	logd(LOGD_H, "Integrity-check executed in %ld ms.\n", time);

	// reset timer
	timer_start(tmr);
	
	vector_f *init = malloc(sizeof(vector_f));
	vector_f_init_set(init, adjm->n_row, 1.0 / adjm->n_row);
	
	printf(" -> init=%f*%lu=%f\n", init->elements[0], adjm->n_row, init->elements[0] * adjm->n_row);

	// time init resultvec gen
	time = timer_total_ms(tmr);

	logd(LOGD_H, "Initial result vector generated in %ld ms.\n", time);
	vector_f_display(LOGD_M, init);
	logd(LOGD_M, "\n");

	vector_f *res = (vector_f*)malloc(sizeof(vector_f));
	vector_f_init_set(res, adjm->n_row, 0.0);

	int ct = (iterations > 10) ? 10 : iterations;

	timer_start(tmr);

	vector_f *tmp;

	for(int i = 0; i < iterations; i++) {
		//matrix_f_multiply_vector_f(res, gm, init);
		if((e = mcrs_gmatrix_mult_vector_f(res, adjm, init, linkv, empty)) != MCRS_ERR_NONE) {
			logd_e("ERROR while multiplying (CODE %d). Aborting.\n", e);
			return 4;
		}
	
		if((i + 1) % (iterations / ct) == 0 || i == 0) {	
			logd(LOGD_M, "Result vector:\n", i + 1);
			vector_f_display(LOGD_M, res);
		}

		//vector_f_copy(init, res);
		tmp = init;
		init = res;
		res = tmp;

		time = timer_lap(tmr);
		logd(LOGD_H, "%d\t%ld\tms\n", i, time);
	}

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

