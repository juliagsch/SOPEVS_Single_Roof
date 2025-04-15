//
// Created by Brad Huang on 8/17/20.
//

#include "params_common.h"

using namespace std;

double B_inv; // cost per cell
double epsilon;
double confidence;
int metric;

size_t days_in_chunk;
size_t chunk_size;
size_t chunk_step;
size_t chunk_total;

vector<double> load;

// define the upper and lower values to test for battery cells and pv,
// as well as the step size of the search
double cells_min;
double cells_max;
double cells_step; // search in step of x cells

size_t number_of_chunks;

vector<double> read_data_from_file(istream &datafile, int limit) {

    vector<double> data;

    if (datafile.fail()) {
        data.push_back(-1);
        cerr << errno << ": read data file failed." << endl;
        return data;
    }

    // read data file into vector
    string line;
    double value;

    for (int i = 0; i < limit && getline(datafile, line); ++i) {
        istringstream iss(line);
        iss >> value;
        data.push_back(value);
    }

    return data;
}
