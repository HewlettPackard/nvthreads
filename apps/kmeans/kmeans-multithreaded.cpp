//============================================================================
// Name        : kmeans.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <pthread.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>
#include <execinfo.h>

using namespace std;

typedef vector<int> vec_i;
typedef vector<vec_i> vec2_i;
typedef vector<double> vec_d;
typedef vector<vec_d> vec2_d;

//const bool USE_LOCAL_LBLS = false;
const double POS_INF = numeric_limits<double>::infinity();

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

	bool use_local_lbls;
	bool use_locks;

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

vec2_d load_data(const char* path, const char delim) {
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
	else {
		cout << "Unable to open file :(\n\n";
	}

	return res;
}

void dump_data(vec_i data, const char* path, const char delim) {
	ofstream data_file;

	data_file.open(path);

	if(data_file.is_open()) {
		for(int i = 0; i < data.size(); i++) {
			data_file << data[i] << (i == data.size() - 1 ? ' ' : delim);
		}
	}

	data_file.close();
}

void dump_data(vec2_d data, const char* path, const char delim) {
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
		cout << "Invalid arguments: number of elements in x, number of norms and number of labels are not equal\n";
		return;
	}
	if(x_sz < 1) {
		cout << "Invalid arguments: too few elements in x, specify at least 1\n";
		return;
	}
	if(c_sz > x_sz) {
		cout << "Invalid arguments: number of centers is larger than number of elements in x\n";
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

		if((*data).use_local_lbls) {
			(*(*data).lbls_loc)[i] = inew;
		}
		else if((*data).use_locks){
			pthread_mutex_lock(&mutex_lbls);

			(*(*data).lbls)[i] = inew;

			pthread_mutex_lock(&mutex_lbls);
		}
		else {
			(*(*data).lbls)[i] = inew;
		}

		(*(*data).c_count_loc)[inew]++;

		for(int j = 0; j < v_sz; j++) {
			(*(*data).c_sum_loc)[inew][j] += (*(*data).x)[i][j];
		}
	}

	pthread_exit(NULL);
}

void spawn_threads(int NUM_THREADS, vec2_d *x, vec_d *x_norms,
		vec2_d *centers, int iterations, vec_i *lbls, bool use_local_lbls, bool use_locks) {
	if(NUM_THREADS == -1) {
		kmeans(x, x_norms, centers, iterations, lbls);
	}
	else {
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

			d.use_local_lbls = use_local_lbls;
			d.use_locks = use_locks;

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

		for(; iterations > 0; iterations--) {
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

				// merge labels of this thread
				if(use_local_lbls) {
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
		}
	}
}

int main(int argc, char* argv[]) {
	cout << "\n";
	cout << "----------------------------------------------------\n";
	cout << "    K-MEANS, Multithreaded (Helge Bruegner 2015)    \n";
	cout << "----------------------------------------------------\n\n";

	if(argc < 7 || argc > 8) {
		cout << "Invalid arguments! Usage:\n";
		cout << "kmeans <x-input-file> <centers-input-file> "
				<< "<threads> <iterations> <use_loc_lbls> <use_lbl_lock> "
				<< "[<mapping-output-file>]\n";

		cout << "\n";

		return 1;
	}

	cout << "Start reading the file [" << argv[1] << "]... " << flush;
	vec2_d x = load_data(argv[1], ',');

	cout << "Done!\n";

	cout << "Start reading the file [" << argv[2] << "]... " << flush;
	vec2_d centers = load_data(argv[2], ',');

	cout << "Done!\n";

	cout << "Calculating the norms and generating initial labels... " << flush;

	vec_d norms = calc_norm(&x);
	vec_i lbls = generate_lbls(x.size(), -1);

	cout << "Done!\n\n";

	cout << " Executing k-means with the following configuration:\n";
	cout << "        X-File = \"" << argv[1] << "\" (" << x.size() << " lines)\n";
	cout << "  Centers-File = \"" << argv[2] << "\" (" << centers.size() << " lines)\n";
	cout << "       Threads = "  << argv[3] << "\n";
	cout << "    Iterations = "  << argv[4] << "\n";
	cout << "    Dimensions = "  << x[0].size() << "\n";
	cout << " Use Loc. Lbls = "  << argv[5] << "\n";
	cout << " Use Lbl.-Lock = "  << argv[6] << "\n\n";

	cout << "Running... " << flush;
;	spawn_threads(stoi(argv[3]), &x, &norms, &centers, stoi(argv[4]), &lbls,
			stob(argv[5], "true", "false"), stob(argv[6], "true", "false"));

	cout << "Done!\n\n";

	if(argc == 8) {
		cout << "Dump labeling to the file [" << argv[8] << "]..." << flush;

		dump_data(lbls, argv[8], ',');

		cout << "Done!\n";
	}
	else {
		cout << "Skipped dumping to file. Printing centers:\n";

		for(int i = 0; i < centers.size(); i++) {
			cout << "[" << i << "](";

			for(int j = 0; j < centers[i].size(); j++) {
				cout << centers[i][j] << (j == centers[i].size() - 1 ? "" : ",");
			}

			cout << ")\n";
		}
	}

	cout << "\n";

	return 0;
}
