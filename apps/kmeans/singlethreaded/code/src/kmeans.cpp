//============================================================================
// Name        : kmeans.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>

using namespace std;

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

vector<vector<double>> load_data(const char* path, const char delim) {
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

void dump_data(vector<int> data, const char* path, const char delim) {
	ofstream data_file;

	data_file.open(path);

	if(data_file.is_open()) {
		for(int i = 0; i < data.size(); i++) {
			data_file << data[i] << (i == data.size() - 1 ? ' ' : delim);
		}
	}

	data_file.close();
}

void dump_data(vector<vector<double>> data, const char* path, const char delim) {
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

vector<double> calc_norm(vector<vector<double>> *x) {
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

vector<int> generate_lbls(int sz, int init) {
	vector<int> res(sz);

	for(int i = 0; i < sz; i++)
		res[i] = init;

	return res;
}

void kmeans(vector<vector<double>> *x, vector<double> *norms,
		vector<vector<double>> *centers, int iterations, vector<int> *lbls) {
	int x_sz = (*x).size();
	int n_sz = (*norms).size();
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

	const double POS_INF = numeric_limits<double>::infinity();

	bool skip_c_norms = false;

	double appD;
	double best;
	double dd;
	double tmp;
	int inew = 0;
	int ctr;
	int n;

	std::vector<double> itmd;

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
				appD = (*norms)[i] - c_norms[n];
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

int main(int argc, char* argv[]) {
	cout << "-----------------------------------------------\n";
	cout << " K-MEANS, Singlethreaded (Helge Bruegner 2015) \n";
	cout << "-----------------------------------------------\n\n";


	if(argc < 4) {
		cout << "Invalid arguments: \n\n";
		cout << " Usage: kmeans <x-input-file> <centers-input-file> <iterations> [<mapping-output-file>]\n";

		return 1;
	}

	cout << "Start reading the file [" << argv[1] << "]...\n";
	vector<vector<double> > x = load_data(argv[1], ',');

	cout << "Start reading the file [" << argv[2] << "]...\n";
	vector<vector<double> > centers = load_data(argv[2], ',');

	cout << "Done!\n\n";

	cout << "Calculating the norms and generating initial labels...\n";

	vector<double> norms = calc_norm(&x);
	vector<int> lbls = generate_lbls(x.size(), -1);

	cout << "Done!\n\n";

	cout << "Executing singlethreaded k-means with the following configuration:\n";
	cout << "       X-File = \"" << argv[1] << "\" (" << x.size() << " lines)\n";
	cout << " Centers-File = \"" << argv[2] << "\" (" << centers.size() << " lines)\n";
	cout << "   Iterations = " 	<< argv[3] << "\n";
	cout << "   Dimensions = " 	<< x[0].size() << " ...\n";

	kmeans(&x, &norms, &centers, stoi(argv[3]), &lbls);

	cout << "Done!\n\n";

	if(argc > 4) {
		cout << "Dump labeling to the file [" << argv[4] << "]...\n";

		dump_data(lbls, argv[4], ',');

		cout << "Done!";
	}
	else {
		cout << "Skipped dumping. Printing centers:\n";

		for(int i = 0; i < centers.size(); i++) {
			cout << "[" << i << "](";

			for(int j = 0; j < centers[i].size(); j++) {
				cout << centers[i][j] << (j == centers[i].size() - 1 ? "" : ",");
			}

			cout << ")\n";
		}
	}

	return 0;
}
