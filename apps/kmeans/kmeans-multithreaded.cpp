#include <pthread.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>
#include <execinfo.h>
#include <chrono>
#include <stack>

using namespace std;

typedef vector<int> vec_i;
typedef vector<vec_i> vec2_i;
typedef vector<double> vec_d;
typedef vector<vec_d> vec2_d;

const double POS_INF = numeric_limits<double>::infinity();

enum SyncMethod {
	AFTER_T_EXIT,		// local during computation, sync in main-thread
	ON_T_EXIT, 			// local during computation, sync WITHOUT locks before _exit
	ON_T_EXIT_LOCKED,	// local during computation, sync WITH locks before _exit
	IMM,				// global during computation WITHOUT locks
	IMM_LOCKED			// global during computation WITH locks
};

/* ==== CONSTRUCTOR-FUNCTIONS ==== */

vec2_i* construct_vec2_i(int x, int y) {
	vec2_i *res = new vec2_i(x);

	for(int i = 0; i < x; i++) {
		for(int j = 0; j < y; j++) {
			(*res)[i] = vec_i(y);
		}
	}

	return res;
}

vec2_d* construct_vec2_d(int x, int y) {
	vec2_d *res = new vec2_d(x);

	for(int i = 0; i < x; i++) {
		for(int j = 0; j < y; j++) {
			(*res)[i] = vec_d(y);
		}
	}

	return res;
}

/* ==== STRUCTS AND STUFF ==== */

struct t_data {
	int thread_id;

	SyncMethod sync_method;

	/*bool use_local_lbls;
	bool use_locks;
	bool use_postsync;*/

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

/* ==== LOGGING ==== */

const int LOG_NONE = 100;
const int LOG_ERR = 10;
const int LOG_TIME = 5;
const int LOG_ALL = 0;

int min_level = 0;

void log_set_min(int level) {
	min_level = level;
}

void log(int level, string msg) {
	if(level >= min_level)
		cout << msg << flush;
}

void log(int level, int msg) {
	if(level >= min_level)
		cout << msg << flush;
}

void log(int level, long msg) {
	if(level >= min_level)
		cout << msg << flush;
}

void log(int level, double msg) {
	if(level >= min_level)
		cout << msg << flush;
}

void log_e(int level, string msg) {
	if(level >= min_level)
		cerr << msg << flush;
}

void log_e(int level, int msg) {
	if(level >= min_level)
		cerr << msg << flush;
}

void log_e(int level, long msg) {
	if(level >= min_level)
		cerr << msg << flush;
}

void log_e(int level, double msg) {
	if(level >= min_level)
		cerr << msg << flush;
}

/* ==== CLOCK ==== */

stack<chrono::high_resolution_clock::time_point> times_start;
stack<chrono::high_resolution_clock::time_point> times_end;

void clock_start() {
	times_start.push(chrono::high_resolution_clock::now());
}

void clock_stop() {
	times_end.push(chrono::high_resolution_clock::now());
}

long clock_get_duration() {
	long duration = chrono::duration_cast<chrono::milliseconds>(times_end.top() - times_start.top()).count();

	times_start.pop();
	times_end.pop();

	return duration;
}

/* ==== FUNCTIONS FOR DATA-LOADING AND -SAVING ==== */

vector<double> parse_line(const string &line, char delim) {
	vector<double> res;
	stringstream ss(line);
	string itm;

	while(getline(ss, itm, delim)) {
		//cout << itm << " ";
		res.push_back(stod(itm));
	}

	return res;
}

vec2_d load_data(string path, const char delim) {
	ifstream data_file;
	string line;

	vector<vector<double>> res;

	data_file.open(path);

	if(data_file.is_open()) {
		while(getline(data_file, line)) {
			res.push_back(parse_line(line, delim));
		}

		data_file.close();
	}

	return res;
}

void dump_data(vec_i data, string path, const char delim) {
	ofstream data_file;

	data_file.open(path);

	if(data_file.is_open()) {
		for(int i = 0; i < data.size(); i++) {
			data_file << data[i] << (i == data.size() - 1 ? ' ' : delim);
		}
	}

	data_file.close();
}

void dump_data(vec2_d data, string path, const char delim) {
	ofstream data_file;

	data_file.open(path);

	if(data_file.is_open()) {
		for(int i = 0; i < data.size(); i++) {
			for(int j = 0; j < data[i].size(); j++) {
				data_file << data[i][j] << (j == data[i].size() - 1 ? ' ' : delim);
			}

			data_file << "\n";
		}
	}

	data_file.close();
}

void vec_to_arr(vector<double> *v, double *a) {
	int v_sz = (*v).size();

	for(int i = 0; i < v_sz; i++) {
		(*(a + i)) = (*v)[i];
	}
}

void vec_to_arr(vector<vector<double>> *v, double *a) {
	int v_sz = (*v).size();
	int v0_sz = (*v)[0].size();

	for(int i = 0; i < v_sz; i++) {
		for(int j = 0; j < v0_sz; j++) {
			(*(a + i * v0_sz + j)) = (*v)[i][j];
		}
	}
}

/* ==== SOME HELPER-FUNCTIONS ==== */

vec_d calc_norm(vec2_d *x) {
	int x_sz = (*x).size();
	int x0_sz = (*x)[0].size();

	vector<double> norms(x_sz);

	for(int i = 0; i < x_sz; i++) {
		for(int j = 0; j < x0_sz; j++)
			norms[i] += (*x)[i][j] * (*x)[i][j];

		norms[i] = sqrt(norms[i]);
	}

	return norms;
}

void calc_norms(double *x, int x_sz, int v_sz, double* a) {
	for(int i = 0; i < x_sz; i++) {
		for(int j = 0; j < v_sz; j++)
			*(a + i) += *(x + i * v_sz + j) * *(x + i * v_sz + j);
	}
}

vec_i generate_lbls(int sz, int init) {
	vec_i res(sz);

	for(int i = 0; i < sz; i++)
		res[i] = init;

	return res;
}

void arr_i_fill(int *a, int sz, int f) {
	for(int i = 0; i < sz; i++)
		*(a + i) = f;
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

	vec2_i *res = new vec2_i(t);

	for(int i = 0; i < t; i++) {
		int length = ((x - assigned) / (t - i));

		int start = assigned;

		(*res)[i][0] = start;
		(*res)[i][1] = length;

		assigned += length;
	}

	return res;
}

bool stob(string s, string s_t, string s_f) {
	if(s == s_t)
		return true;
	else if(s == s_f)
		return false;
	else
		return false;
}

/* ==== SINGLE-THREADED K-MEANS ==== */

void kmeans(vec2_d *x, vec_d *x_norms,
		vec2_d *centers, int iterations, vec_i *lbls) {
	int x_sz = (*x).size();
	int n_sz = (*x_norms).size();
	int c_sz = (*centers).size();
	int lbl_sz = (*lbls).size();

	if(x_sz != lbl_sz || x_sz != n_sz || lbl_sz != n_sz) {
		log_e(LOG_ERR, "Invalid arguments: number of elements in x, number of norms and number of labels are not equal\n");
		return;
	}
	if(x_sz < 1) {
		log_e(LOG_ERR, "Invalid arguments: too few elements in x, specify at least 1\n");
		return;
	}
	if(c_sz > x_sz) {
		log_e(LOG_ERR, "Invalid arguments: number of centers is larger than number of elements in x\n");
		return;
	}

	int v_sz = (*x)[0].size();

	double c_norms[c_sz];	// Stores norms of all centers
	int c_n[c_sz];			// Stores numbers of elements for each center

	bool skip_c_norms = false;

	double appD;
	double best;
	double dd;
	double tmp;
	int inew = 0;
	int ctr;
	int n;

	for(; iterations > 0; iterations--) {
		// Iterate over centers, calculate c_norms
		if(!skip_c_norms) {
			for(int i = 0; i < c_sz; i++) {
				c_norms[i] = 0;

				for(int j = 0; j < v_sz; j++) {
					c_norms[i] += (*centers)[i][j] * (*centers)[i][j];
				}

				c_norms[i] = std::sqrt(c_norms[i]);
			}

			skip_c_norms = true;
		}

		// Iterate over points in x

		for(int i = 0; i < x_sz; i++) {
			best = POS_INF;

			for(int n = 0; n < c_sz; n++) {
				appD = (*x_norms)[i] - c_norms[n];
				appD *= appD;

				// Stop, if approximation is already too big
				if(best < appD)
					continue;

				dd = 0.0;

				for(int j = 0; j < v_sz; j++) {
					tmp = (*x)[i][j] - (*centers)[n][j];

					dd += tmp * tmp;
				}

				if(dd < best) {
					best = dd;
					inew = n;
				}
			}

			if((*lbls)[i] != inew)
				(*lbls)[i] = inew;
		}

		// Iterate over centers, update them

		// Reset centers and numbers of elements per center
		for(int i = 0; i < c_sz; i++) {
			for(int j = 0; j < v_sz; j++) {
				(*centers)[i][j] = 0;
			}

			c_n[i] = 0;
		}

		// Count no. of elements per center, sum components of vectors
		for(int i = 0; i < x_sz; i++) {
			ctr = (*lbls)[i];
			c_n[ctr]++;

			for(int j = 0; j < v_sz; j++) {
				(*centers)[ctr][j] += (*x)[i][j];
			}
		}

		// Reposition centers and calculate the new center-norms
		for(int i = 0; i < c_sz; i++) {
			n = c_n[i];

			c_norms[i] = 0;

			for(int j = 0; j < v_sz; j++) {
				(*centers)[i][j] /= n;

				c_norms[i] += (*centers)[i][j] * (*centers)[i][j];
			}

			c_norms[i] = sqrt(c_norms[i]);
		}
	}
}

/* ==== MULTI-THREADED K-MEANS ==== */

pthread_mutex_t mutex_log;
pthread_mutex_t mutex_counter;
pthread_mutex_t mutex_lbls;
pthread_mutex_t mutex_c_norms;
pthread_mutex_t mutex_c_sum;
pthread_mutex_t mutex_c_count;

pthread_barrier_t barrier_calc_c_norms;
pthread_barrier_t barrier_find_center;

void *kmeans_worker(void *p_data) {
	t_data *data = (t_data*)p_data;

	//int x_sz = (*(*data).x).size();
	int c_sz = (*(*data).c).size();
	int v_sz = (*(*data).x)[0].size();

	double best;
	double appD;
	double dd;
	double tmp_d;
	int inew = 0;

	// find the closest center within the threads range of data
	for(int i = (*data).x_start; i < ((*data).x_start + (*data).x_length); i++) {
		// find the closest center
		best = POS_INF;

		for(int n = 0; n < c_sz; n++) {
			appD = (*(*data).x_norms)[i] - (*(*data).c_norms)[n];
			appD *= appD;

			if(best < appD)
				continue;

			dd = 0.0;

			for(int j = 0; j < v_sz; j++) {
				tmp_d = (*(*data).x)[i][j] - (*(*data).c)[n][j];

				dd += tmp_d * tmp_d;
			}

			if(dd < best) {
				best = dd;
				inew = n;
			}
		}

		if((*data).sync_method == SyncMethod::AFTER_T_EXIT
				|| (*data).sync_method == SyncMethod::ON_T_EXIT
				|| (*data).sync_method == SyncMethod::ON_T_EXIT_LOCKED) {
			(*(*data).lbls_loc)[i] = inew;
		}
		else if((*data).sync_method == SyncMethod::IMM_LOCKED){
			pthread_mutex_lock(&mutex_lbls);

			(*(*data).lbls)[i] = inew;

			pthread_mutex_unlock(&mutex_lbls);
		}
		else { // SyncMethod::IMM
			(*(*data).lbls)[i] = inew;
		}

		(*(*data).c_count_loc)[inew]++;

		for(int j = 0; j < v_sz; j++) {
			(*(*data).c_sum_loc)[inew][j] += (*(*data).x)[i][j];
		}
	}

	if((*data).sync_method == SyncMethod::ON_T_EXIT) {
		for(int i = (*data).x_start; i < ((*data).x_start + (*data).x_length); i++)
			(*(*data).lbls)[i] = (*(*data).lbls_loc)[i];
	}
	else if((*data).sync_method == SyncMethod::ON_T_EXIT_LOCKED) {
		pthread_mutex_lock(&mutex_lbls);

		for(int i = (*data).x_start; i < ((*data).x_start + (*data).x_length); i++)
					(*(*data).lbls)[i] = (*(*data).lbls_loc)[i];

		pthread_mutex_unlock(&mutex_lbls);
	}

	pthread_exit(NULL);
}

void spawn_threads(int NUM_THREADS, vec2_d *x, vec_d *x_norms,
		vec2_d *centers, int iterations, vec_i *lbls, SyncMethod sync_method) {//bool use_local_lbls, bool use_locks) {
	if(NUM_THREADS == -1) {
		kmeans(x, x_norms, centers, iterations, lbls);
	}
	else {
		clock_start();

		pthread_t threads[NUM_THREADS];

		// init barriers and mutexes
		pthread_barrier_init(&barrier_calc_c_norms, NULL, NUM_THREADS);
		pthread_barrier_init(&barrier_find_center, NULL, NUM_THREADS);

		pthread_mutex_init(&mutex_log, NULL);
		pthread_mutex_init(&mutex_counter, NULL);
		pthread_mutex_init(&mutex_lbls, NULL);
		pthread_mutex_init(&mutex_c_norms, NULL);
		pthread_mutex_init(&mutex_c_sum, NULL);
		pthread_mutex_init(&mutex_c_count, NULL);

		int x_sz = (*x).size();
		int v_sz = (*x)[0].size();
		int c_sz = (*centers).size();

		// balance load
		int bal_x[NUM_THREADS][2];
		int bal_c[NUM_THREADS][2];

		balance_x(x_sz, NUM_THREADS, &(bal_x[0][0]));
		balance_x(c_sz, NUM_THREADS, &(bal_c[0][0]));

		// init local copies
		t_data data[NUM_THREADS];
		t_data d;

		vec_d c_norms(c_sz);

		vec2_d c_sum = *construct_vec2_d(c_sz, v_sz);
		vec_i c_count(c_sz);

		for(int i = 0; i < NUM_THREADS; i++) {
			d = *(new t_data);

			d.thread_id = i;

			d.sync_method = sync_method;

			//d.use_local_lbls = use_local_lbls;
			//d.use_locks = use_locks;

			d.x = x;
			d.x_start = bal_x[i][0];
			d.x_length = bal_x[i][1];

			d.x_norms = x_norms;

			d.lbls_loc = new vec_i(x_sz);
			d.lbls = lbls;

			d.c = centers;

			d.c_norms = &c_norms;

			d.c_n_start = 0;
			d.c_n_length = 0;

			d.c_sum_loc = construct_vec2_d(c_sz, v_sz);
			d.c_sum = &c_sum;

			d.c_count_loc = new vec_i(c_sz);
			d.c_count = &c_count;

			data[i] = d;
		}

		clock_stop();

		log(LOG_TIME, " Thread-initialization done in ");
		log(LOG_TIME, clock_get_duration());
		log(LOG_TIME, " ms.\n");

		for(int _it = 0; _it < iterations; _it++) {
			clock_start();

			// reset sum and counters
			for(int i = 0; i < c_sz; i++) {
				for(int j = 0; j < v_sz; j++) {
					c_sum[i][j] = 0.0;
				}

				c_count[i] = 0;
			}

			// calculate center-norms
			for(int i = 0; i < c_sz; i++) {
				c_norms[i] = 0;

				for(int j = 0; j < v_sz; j++) {
					c_norms[i] += (*centers)[i][j] * (*centers)[i][j];
				}

				c_norms[i] = sqrt(c_norms[i]);
			}

			// spawn threads
			for(int t = 0; t < NUM_THREADS; t++) {
				pthread_create(&(threads[t]), NULL, kmeans_worker, &(data[t]));
			}

			for(int t = 0; t < NUM_THREADS; t++) {
				pthread_join(threads[t], NULL);

				// merge labels of this thread if AFTER_T_EXIT-syncing is enabled
				if(sync_method == SyncMethod::AFTER_T_EXIT) {
					for(int i = data[t].x_start; i < data[t].x_start + data[t].x_length; i++) {
						(*lbls)[i] = (*(data[t]).lbls_loc)[i];
					}
				}
			}

			// merge sums and count
			for(int i = 0; i < c_sz; i++) {
				for(int t = 0; t < NUM_THREADS; t++) {
					// sum intermediates
					c_count[i] += (*(data[t]).c_count_loc)[i];

					// reset local
					(*(data[t]).c_count_loc)[i] = 0;
				}

				for(int j = 0; j < v_sz; j++) {
					for(int t = 0; t < NUM_THREADS; t++) {
						// sum intermediates
						c_sum[i][j] += (*(data[t]).c_sum_loc)[i][j];

						// reset local
						(*(data[t]).c_sum_loc)[i][j] = 0;
					}

					(*centers)[i][j] = c_sum[i][j] / c_count[i];
				}
			}

			clock_stop();

			log(LOG_TIME, " ");
			log(LOG_TIME, _it);
			log(LOG_TIME, "\t ");
			log(LOG_TIME, clock_get_duration());
			log(LOG_TIME, " ms\n");
		}
	}
}

const int MIN_ARGC = 7;

const int P_PNAME = 0;
const int P_F_X = 1;
const int P_F_C = 2;
const int P_N_TH = 3;
const int P_N_IT = 4;
const int P_O_SYNCM = 5;
const int P_O_LOG = 6;
const int P_F_LBLS = 7;

const int MAX_ARGC = MIN_ARGC + 1;

int main(int argc, char* argv[]) {
	log_set_min(LOG_ALL);

	if(argc < MIN_ARGC || argc > MAX_ARGC) {
		log_e(LOG_ALL, "\nInvalid arguments! Usage:\n");
		log_e(LOG_ALL, argv[P_PNAME]);
		log_e(LOG_ALL, " <x-input-file> <centers-input-file> "
				"<threads> <iterations> <sync-mode> <log-mode> "
				"[<mapping-output-file>]\n\n"
				" Allowed sync-modes:\n"
				"  0 - Sync in main-thread\n"
				"  1 - Sync on thread-exit without locks\n"
				"  2 - ... with locks\n"
				"  3 - Immediately after the label is calculated without locks\n"
				"  4 - ... with locks\n"
				" \n"
				" Allowed log-modes:\n"
				"  0 - All\n"
				"  1 - Time and Err-only\n"
				"  2 - Err-only\n"
				"  3 - None\n"
				"\n");

		return 1;
	}

	int o_logm = stoi(argv[P_O_LOG]);

	switch(o_logm) {
		case 0: log_set_min(LOG_ALL); break;
		case 1: log_set_min(LOG_TIME); break;
		case 2: log_set_min(LOG_ERR); break;
		case 3: log_set_min(LOG_NONE); break;
		default: log_set_min(LOG_ALL);
	}

	log(LOG_ALL, "\n"
			"----------------------------------------------------\n"
			"    K-MEANS, Multithreaded (Helge Bruegner 2015)    \n"
			"----------------------------------------------------\n\n");

	string f_x(argv[P_F_X]);
	string f_c(argv[P_F_C]);
	int n_threads = stoi(argv[P_N_TH]);
	int n_iterations = stoi(argv[P_N_IT]);
	SyncMethod o_syncm = (SyncMethod)stoi(argv[P_O_SYNCM]);

	log(LOG_ALL, "Start reading the file \"");
	log(LOG_ALL, f_x);
	log(LOG_ALL, "\"... ");

	clock_start();

	vec2_d x = load_data(f_x, ',');

	clock_stop();

	log(LOG_TIME, " X-File loaded in ");
	log(LOG_TIME, clock_get_duration());
	log(LOG_TIME, " ms. ");

	log(LOG_ALL, "Done!");
	log(LOG_TIME, "\n");

	log(LOG_ALL, "Start reading the file \"");
	log(LOG_ALL, f_c);
	log(LOG_ALL, "\"... ");

	clock_start();

	vec2_d centers = load_data(f_c, ',');

	clock_stop();

	log(LOG_TIME, " Centers-File loaded in ");
	log(LOG_TIME, clock_get_duration());
	log(LOG_TIME, " ms. ");

	log(LOG_ALL, "Done!");
	log(LOG_TIME, "\n");

	clock_start();

	log(LOG_ALL, "Calculating the norms and generating initial labels... ");

	vec_d norms = calc_norm(&x);
	vec_i lbls = generate_lbls(x.size(), -1);

	clock_stop();

	log(LOG_TIME, " Calculated norms and generated labels in ");
	log(LOG_TIME, clock_get_duration());
	log(LOG_TIME, " ms. ");

	log(LOG_ALL, "Done!\n");
	log(LOG_TIME, "\n");
	log(LOG_ALL, " Executing k-means with the following configuration:\n"
			"        X-File = \"");
	log(LOG_ALL, f_x);
	log(LOG_ALL, "\" (");
	log(LOG_ALL, (int)x.size());
	log(LOG_ALL, " lines)\n"
			"  Centers-File = \"");
	log(LOG_ALL, f_c);
	log(LOG_ALL, "\" (");
	log(LOG_ALL, (double)centers.size());
	log(LOG_ALL, " lines)\n");
	log(LOG_ALL, "       Threads = ");
	log(LOG_ALL, n_threads);
	log(LOG_ALL, "\n");
	log(LOG_ALL, "    Iterations = ");
	log(LOG_ALL, (int)n_iterations);
	log(LOG_ALL, "\n");
	log(LOG_ALL, "    Dimensions = ");
	log(LOG_ALL, (int)x[0].size());
	log(LOG_ALL, "\n");
	log(LOG_ALL, "   Sync-Method = ");
	log(LOG_ALL, o_syncm);
	log(LOG_ALL, "\n\n");

	log(LOG_ALL, "Running... ");

	clock_start();

	spawn_threads(n_threads, &x, &norms, &centers, n_iterations, &lbls, o_syncm);

	clock_stop();

	log(LOG_TIME, " Executed the algorithm in ");
	log(LOG_TIME, clock_get_duration());
	log(LOG_TIME, " ms. ");

	log(LOG_ALL, "Done!\n");
	log(LOG_TIME, "\n");

	if(argc == MIN_ARGC + 1) {
		string f_lbls(argv[P_F_LBLS]);

		log(LOG_ALL, "Dump labeling to the file [");
		log(LOG_ALL, f_lbls);
		log(LOG_ALL, "]...");

		dump_data(lbls, f_lbls, ',');

		log(LOG_ALL, "Done!\n");
	}
	else {
		log(LOG_ALL, "Skipped dumping to file. Printing centers:\n");

		for(int i = 0; i < centers.size(); i++) {
			log(LOG_ALL, "[");
			log(LOG_ALL, (int)i);
			log(LOG_ALL, "](");

			for(int j = 0; j < centers[i].size(); j++) {
				log(LOG_ALL, centers[i][j]);
				log(LOG_ALL, (j == centers[i].size() - 1 ? "" : ","));
			}

			log(LOG_ALL, ")\n");
		}
	}

	log(LOG_ALL, "\n");

	return 0;
}
