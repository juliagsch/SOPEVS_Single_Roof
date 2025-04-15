#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include "simulate_system_load_curve.h"
#include "params_load_curve.h"
#include "ev.h"

using namespace std;

// run_simulations
// load_filename: filename, each line in file contains electricity consumption value
// solar_filename: filename, each line in file contains solar generation value
// metric: 0 for LOLP, 1 for unmet load
// epsilon: number in range [0,1] representing LOLP or unmet load fraction.
// chunk_size: length of time (in days)
void run_simulations(vector<double> &load, vector<double> &solar, int metric, int chunk_size, std::vector<EVRecord> evRecords, std::vector<std::vector<EVStatus>> allDailyStatuses, double max_soc, double min_soc)
{

	// set random seed to a specific value if you want consistency in results
	srand(10);

	// get number of timeslots in each chunk
	// zb 100 days a 24h if we have hourly data in the input files
	int t_chunk_size = chunk_size * (24 / T_u);
	cout << "t_chunk_size = " << t_chunk_size << endl;

	int Ev_start = rand() % evRecords.size();
	// to start on a Monday
	Ev_start = 0;
	// set battery to 0 if no stationary storage

	double battery_result = 4; // in kwh
	double battery_cells = battery_result / kWh_in_one_cell;
	double pv_result = 4;

	sim(load, solar, 0, t_chunk_size, battery_cells, pv_result, 0, evRecords, allDailyStatuses, max_soc, min_soc, Ev_start, false);
}

int main(int argc, char **argv)
{
	int input_process_status = process_input(argc, argv, true);

	// Handle input processing error if needed
	if (input_process_status != 0)
	{
		std::cerr << "Error processing input" << std::endl;
		return 1; // Or handle the error as appropriate
	}

	// Read EV data
	std::vector<EVRecord> evRecords = readEVData(path_to_ev_data);

	// Check if EV data was read successfully
	if (evRecords.empty())
	{
		std::cerr << "Error reading EV data or no records found" << std::endl;
		return 1; // Or handle the error as appropriate
	}

	// Initialize EVStatus
	EVStatus evStatus;
	// Generate all daily statuses
	std::vector<std::vector<EVStatus>> allDailyStatuses = generateAllDailyStatuses(evRecords);

	run_simulations(load, solar, metric, days_in_chunk, evRecords, allDailyStatuses, max_soc, min_soc);

	return 0;
}
