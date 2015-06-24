#include <cstdlib>
#include <iomanip>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

const char CSV_DELIM = ',';

typedef std::vector<double> vec_d;
typedef std::vector<vec_d> vec2_d;

static inline void loadbar(unsigned int x, unsigned int n, unsigned int w = 50) {
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
		loadbar(i, x, 50);

		for(int j = 0; j < y; j++) {
			(*res)[i] = vec_d(y);
		}
	}

	return res;
}

void fill_random(vec2_d *data, double ctr, double rng) {
	for(int i = 0; i < (*data).size(); i++) {
		loadbar(i, (*data).size(), 50);

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
			loadbar(i, (*data).size(), 50);

			for(int j = 0; j < (*data)[0].size(); j++) {
				data_file << (*data)[i][j] << (j == (*data)[i].size() - 1 ? '\n' : delim);
			}
		}
	}

	data_file.close();
}

void dump_rand_data(int n, int d, double ctr, double rng, const char* path, const char delim) {
	std::ofstream data_file;

	data_file.open(path);

	if(data_file.is_open()) {
		for(int i = 0; i < n; i++) {
			loadbar(i, n, 50);

			for(int j = 0; j < d; j++) {
				data_file
					<< ((double)((std::rand() - RAND_MAX / 2) * rng * 2)/(double)RAND_MAX + ctr)
					<< (j == d - 1 ? '\n' : delim);
			}
		}
	}

	data_file.close();
}

int main(int argc, char* argv[]) {
	std::cout << "\n";

	if(argc != 6) {
		std::cout << "Invalid parameters, usage: <rows> <columns> <offset> <range> <output-file>\n";
		return 1;
	}

	int n = std::stoi(argv[1]); // number of rows
	int d = std::stoi(argv[2]); // number of columns
	double ctr = std::stod(argv[3]); // where the random data should be centered
	double rng = std::stod(argv[4]); // the range of the data -> ctr +- rng

	char *path = argv[5]; // the output-path

	/*std::cout << "Allocating memory...\n";

	vec2_d *v = construct_vec2_d(n, d);

	std::cout << "Done!\n\n";
	std::cout << "Generating random double-data...\n";

	fill_random(v, ctr, rng);

	std::cout << "Done!\n\n";
	std::cout << "Dumping to csv-file... \n";

	dump_data(v, path, CSV_DELIM);

	std::cout << "Done!\n\n";*/

	std::cout << "Generating random double-data and dumping to file...\n";

	dump_rand_data(n, d, ctr, rng, path, CSV_DELIM);

	std::cout << "Done!\n";
}
