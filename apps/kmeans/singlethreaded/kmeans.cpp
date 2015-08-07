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

	for(int _it = 0; _it < iterations; _it++) {
		clock_start();

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

		 clock_stop();

                 log(LOG_TIME, " ");
                 log(LOG_TIME, _it);
                 log(LOG_TIME, "\t ");
                 log(LOG_TIME, clock_get_duration());
                 log(LOG_TIME, " ms\n");

	}
}

const int MIN_ARGC = 5;

const int P_PNAME = 0;
const int P_F_X = 1;
const int P_F_C = 2;
const int P_N_IT = 3;
const int P_O_LOG = 4;
const int P_F_LBLS = 5;

const int MAX_ARGC = MIN_ARGC + 1;

int main(int argc, char* argv[]) {
	log_set_min(LOG_ALL);

	if(argc < MIN_ARGC || argc > MAX_ARGC) {
		log_e(LOG_ALL, "\nInvalid arguments! Usage:\n");
		log_e(LOG_ALL, argv[P_PNAME]);
		log_e(LOG_ALL, " <x-input-file> <centers-input-file> "
				"<iterations> <log-mode> "
				"[<mapping-output-file>]\n\n"
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
	int n_iterations = stoi(argv[P_N_IT]);

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
	log(LOG_ALL, "    Iterations = ");
	log(LOG_ALL, (int)n_iterations);
	log(LOG_ALL, "\n");
	log(LOG_ALL, "    Dimensions = ");
	log(LOG_ALL, (int)x[0].size());
	log(LOG_ALL, "\n\n");

	log(LOG_ALL, "Running... ");

	clock_start();

	kmeans(&x, &norms, &centers, n_iterations, &lbls);

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
