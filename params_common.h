//
// Created by Brad Huang on 8/17/20.
//

#ifndef ROBUST_SIZING_PARAMS_COMMON_H
#define ROBUST_SIZING_PARAMS_COMMON_H

#include <vector>
#include <climits>
#include <limits>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

// INPUTS

extern double B_inv; // cost per cell
extern double epsilon;
extern double confidence;
extern int metric;

extern size_t days_in_chunk;
extern size_t chunk_size;
extern size_t chunk_step;
extern size_t chunk_total;

extern vector<double> load;

// define the upper and lower values to test for battery cells and pv,
// as well as the step size of the search
extern double cells_min;
extern double cells_max;
extern double cells_step; // search in step of x cells

// CONSTANTS

// defines the number of samples, set via command line input
extern size_t number_of_chunks;

/**
 * T_u: this is the time unit, representing the number of hours in
 *      each time slot of the load and solar traces
 */
constexpr size_t static T_u = 1;

/**
 * T_yr: this is year unit, representing the number of traces that constitutes a year.
 *       Inputs must have multiples of this size.
 */
const size_t static T_yr = 365 * 24 / T_u;

double static kWh_in_one_cell = 0.011284;
constexpr double static num_steps = 1000; // search in total of n steps

constexpr double static INFTY = numeric_limits<double>::infinity();
constexpr double static EPS = numeric_limits<double>::epsilon();

// FUNCTIONS

vector<double> read_data_from_file(istream &datafile, int limit = INT_MAX);

#endif //ROBUST_SIZING_PARAMS_COMMON_H
