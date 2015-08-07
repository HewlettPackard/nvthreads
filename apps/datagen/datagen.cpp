#include <cstdlib>
#include <iomanip>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <math.h>

const char CSV_DELIM = ',';

const int LOADBAR_SIZE = 50;

typedef std::vector<int> vec_i;
typedef std::vector<vec_i> vec2_i;

typedef std::vector<double> vec_d;
typedef std::vector<vec_d> vec2_d;

void loadbar(unsigned int x, unsigned int n, unsigned int w = 50) {
    if ( (x != n) && (x % (n/100+1) != 0) ) return;

    float ratio  =  x/(float)n;
    int   c      =  ratio * w;

    std::cout << std::setw(3) << (int)(ratio*100) << "% [";
    for (int x=0; x<c; x++) std::cout << "=";
    for (int x=c; x<w; x++) std::cout << " ";
    std::cout << "]\r" << std::flush;
}

vec2_d* construct_vec2_d(int x, int y) {
	vec2_d *res = new vec2_d(x);

	for(int i = 0; i < x; i++) {
		loadbar(i, x, LOADBAR_SIZE);

		for(int j = 0; j < y; j++) {
			(*res)[i] = vec_d(y);
		}
	}

	return res;
}

void fill_random(vec2_d *data, double ctr, double rng) {
	for(int i = 0; i < (*data).size(); i++) {
		loadbar(i, (*data).size(), LOADBAR_SIZE);

		for(int j = 0; j < (*data)[0].size(); j++) {
			(*data)[i][j] = (double)((std::rand() - RAND_MAX / 2) * rng * 2)/(double)RAND_MAX + ctr;
		}
	}
}

void dump_data(vec2_d *data, const char* path, const char delim) {
	std::ofstream data_file;

	data_file.open(path);

	if(data_file.is_open()) {
		for(int i = 0; i < (*data).size(); i++) {
			loadbar(i, (*data).size(), LOADBAR_SIZE);

			for(int j = 0; j < (*data)[0].size(); j++) {
				data_file << (*data)[i][j] << (j == (*data)[i].size() - 1 ? '\n' : delim);
			}
		}
	}

	data_file.close();
}

void dump_rand_data_i(int n, int d, int ctr, int rng, const char* path, const char delim, int seed) {
	srand(seed);
	
	std::ofstream data_file;
	
	data_file.open(path);
	
	if(data_file.is_open()) {
		for(int i = 0; i < n; i++) {
			loadbar(i, n, LOADBAR_SIZE);
			
			for(int j = 0; j < d; j++) {
				data_file
					<< round((((std::rand() - RAND_MAX / 2.0) * rng * 2.0)/(double)RAND_MAX + ctr))
					<< (j == d - 1 ? '\n' : delim);
			}
		}
	}
}

void dump_rand_data_d(int n, int d, double ctr, double rng, const char* path, const char delim, int seed) {
	srand(seed);

	std::ofstream data_file;

	data_file.open(path);

	double mult = (rng * 2.0) / (RAND_MAX + ctr);
	double offs = (RAND_MAX * rng) / (RAND_MAX + ctr);

	if(data_file.is_open()) {
		for(int i = 0; i < n; i++) {
			loadbar(i, n, 50);

			for(int j = 0; j < d; j++) {
				data_file << ((double)std::rand() * mult - offs)
					//<< ((double)((std::rand() - RAND_MAX / 2.0) * rng * 2.0)/(double)RAND_MAX + ctr)
					<< (j == d - 1 ? '\n' : delim);
			}
		}
	}

	data_file.close();
}

int main(int argc, char* argv[]) {
	std::cout << "\n";

	if(argc != 8) {
		std::cout << "Invalid parameters, usage: <int|double> <rows> <columns> <offset> <range> <seed> <output-file>\n";
		return 1;
	}
	
	int param = 0;
	bool is_int = true; // false -> is_double
	
	std::string str_int("int");
	std::string str_dbl("double");

	std::string type(argv[++param]);

	if(str_int.compare(type) == 0)
		is_int = true;
	else if(str_dbl.compare(type) == 0)
		is_int = false;
	else {
		std::cout << "Invalid type-parameter: write 'int' or 'double'";
		return 2;
	}

	int ctr_i;
	int rng_i;
	double ctr_d;
	double rng_d;
	
	int n = std::stoi(argv[++param]); // number of rows
	int d = std::stoi(argv[++param]); // number of columns
	
	if(is_int) {
		ctr_i = std::stoi(argv[++param]);
		rng_i = std::stoi(argv[++param]);
	}
	else {
		ctr_d = std::stod(argv[++param]); // where the random data should be centered
		rng_d = std::stod(argv[++param]); // the range of the data -> ctr +- rng
	}

	int seed = std::stoi(argv[++param]);
	char *path = argv[++param]; // the output-path

	std::cout << "Generating random " << type  << "-data and dumping to file...\n";

	if(is_int)
		dump_rand_data_i(n, d, ctr_i, rng_i, path, CSV_DELIM, seed);
	else
		dump_rand_data_d(n, d, ctr_d, rng_d, path, CSV_DELIM, seed);

	std::cout << "Done!\n";
}
