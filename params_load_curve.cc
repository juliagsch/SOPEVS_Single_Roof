#include <cstring>
#include <cstdlib>
#include <vector>
#include <climits>
#include <iostream>
#include <string>
#include <regex>
#include <set>

#include "params_load_curve.h"

double PV_inv; // cost per unit (kW) of PV

double pv_min;
double pv_max;
double pv_step; // search in steps of x kW

double max_soc;
double min_soc;
double ev_battery_capacity = 40.0;
double charging_rate = 7.4;
double discharging_rate = 7.4;

vector<double> solar;
std::string wfh_type;

vector<double> socValues;
vector<ChargingEvent> chargingEvents;

double grid_import = 0.0;
double total_load = 0.0;
double total_cost = 0.0;
double total_hours = 0.0;
double load_sum = 0;           // Total load used
double ev_power_used = 0;      // Total power used by ev driving (discharging to power house not included)
double power_lost = 0;         // Electricity lost due to charging and discharging efficiencies of EV
double max_charging_total = 0; // Total electricity used to charge the EV
double ev_battery_diff = 0;    // EV battery difference between beginning and start of the simulation

std::string EV_charging = "naive";               // Default policy
std::string Operation_policy = "unidirectional"; // Default policy
std::string path_to_ev_data;
std::string out_path;

std::string extract_wfh_type(const std::string &ev_filename)
{
    std::regex pattern("ev_data/ev_merged_T(\\d+)\\.csv");
    std::smatch match;

    if (std::regex_search(ev_filename, match, pattern) && match.size() > 1)
    {
        return match.str(1); // The captured group
    }
    else
    {
        return ""; // No match found
    }
}

int extractLoadNumber(const std::string &filename)
{
    // Find the position of "load_"
    size_t loadPos = filename.find("load_");
    if (loadPos == std::string::npos)
    {
        std::cerr << "Error: 'load_' not found in filename." << std::endl;
        return -1; // Error indicator
    }

    // Extract the substring starting from "load_" to the end of the filename
    std::string numberStr = filename.substr(loadPos + 5); // 5 is the length of "load_"

    // Remove ".txt" from the end of the number string
    size_t txtPos = numberStr.find(".txt");
    if (txtPos != std::string::npos)
    {
        numberStr = numberStr.substr(0, txtPos);
    }

    // Convert the number string to an integer
    int loadNumber = std::stoi(numberStr);

    return loadNumber;
}

int process_input(int argc, char **argv, bool process_metric_input)
{

    int i = 0;

    string inv_PV_string = argv[++i];
    PV_inv = stod(inv_PV_string);

#ifdef DEBUG
    cout << "inv_PV_string = " << PV_inv
         << ", PV_inv = " << PV_inv << endl;
#endif

    string inv_B_string = argv[++i];
    B_inv = stod(inv_B_string) * kWh_in_one_cell; // convert from per-kWh to per-cell cost

#ifdef DEBUG
    cout << "inv_B_string = " << inv_B_string
         << ", B_inv = " << B_inv << endl;
#endif

    string pv_max_string = argv[++i];
    pv_max = stod(pv_max_string);

    // set default pv_min and pv_step
    pv_min = 0;
    pv_step = (pv_max - pv_min) / num_steps;

#ifdef DEBUG
    cout << "pv_max_string = " << pv_max_string
         << ", pv_max = " << pv_max
         << ", pv_min = " << pv_min
         << ", pv_step = " << pv_step
         << endl;
#endif

    string cells_max_string = argv[++i];
    cells_max = stod(cells_max_string) / kWh_in_one_cell;

    // set default cells_min and cells_step
    cells_min = 0;
    cells_step = (cells_max - cells_min) / num_steps;

#ifdef DEBUG
    cout << "cells_max_string = " << cells_max_string
         << ", cells_max = " << cells_max
         << ", cells_min = " << cells_min
         << ", cells_step = " << cells_step
         << endl;
#endif

    if (process_metric_input)
    {
        string metric_string = argv[++i];
        metric = stoi(metric_string);

#ifdef DEBUG
        cout << "metric_string = " << metric_string
             << ", metric = " << metric << endl;
#endif
    }

    string epsilon_string = argv[++i];
    epsilon = stod(epsilon_string);

#ifdef DEBUG
    cout << "epsilon_string = " << epsilon_string
         << ", epsilon = " << epsilon << endl;
#endif

    string confidence_string = argv[++i];
    confidence = stod(confidence_string);

#ifdef DEBUG
    cout << "confidence_string = " << confidence_string
         << ", confidence = " << confidence << endl;
#endif

    string days_in_chunk_string = argv[++i];
    days_in_chunk = stoi(days_in_chunk_string);

#ifdef DEBUG
    cout << "days_in_chunk_string = " << days_in_chunk_string
         << ", days_in_chunk = " << days_in_chunk << endl;
#endif

    string loadfile = argv[++i];

#ifdef DEBUG
    cout << "loadfile = " << loadfile << endl;
#endif

    if (loadfile == string("--"))
    {
        // read from cin
        int limit = stoi(argv[++i]);

#ifdef DEBUG
        cout << "reading load data from stdin. limit = " << limit << endl;
#endif

        load = read_data_from_file(cin, limit);
    }
    else
    {

#ifdef DEBUG
        cout << "reading load file" << endl;
#endif

        // read in data into vector
        ifstream loadstream(loadfile.c_str());
        load = read_data_from_file(loadstream);
    }

#ifdef DEBUG
    cout << "checking for errors in load file..." << endl;
#endif

    if (load[0] < 0)
    {
        cerr << "error reading load file " << loadfile << endl;
        return 1;
    }
    else if (load.size() % T_yr > 0)
    {
        cerr << "load file length needs to be multiple of " << T_yr << endl;
        return 1;
    }

    string solarfile = argv[++i];

#ifdef DEBUG
    cout << "solarfile = " << solarfile << endl;
#endif

    if (solarfile == string("--"))
    {

#ifdef DEBUG
        cout << "reading solar file" << endl;
#endif

        // read from cin
        int limit = stoi(argv[++i]);

#ifdef DEBUG
        cout << "reading solar data from stdin. limit = " << limit << endl;
#endif

        solar = read_data_from_file(cin, limit);
    }
    else
    {
        // read in data into vector
        ifstream solarstream(solarfile.c_str());
        solar = read_data_from_file(solarstream);
    }

#ifdef DEBUG
    cout << "checking for errors in solar file..." << endl;
#endif

    if (solar[0] < 0)
    {
        cerr << "error reading solar file " << solarfile << endl;
        return 1;
    }
    else if (solar.size() % T_yr > 0)
    {
        cerr << "solar file length needs to be multiple of " << T_yr << endl;
        return 1;
    }

    string max_soc_string = argv[++i];
    max_soc = stod(max_soc_string);

#ifdef DEBUG
    cout << "max_soc_string = " << max_soc_string << ", max_soc = " << max_soc << endl;
#endif

    string min_soc_string = argv[++i];
    min_soc = stod(min_soc_string);

#ifdef DEBUG
    cout << "min_soc_string = " << min_soc_string << ", min_soc = " << min_soc << endl;
#endif
    string ev_battery_capacity_string = argv[++i];
    ev_battery_capacity = stod(ev_battery_capacity_string);

#ifdef DEBUG
    cout << "ev_battery_capacity_string = " << ev_battery_capacity_string << ", ev_battery_capacity = " << ev_battery_capacity << endl;
#endif

    string charging_rate_string = argv[++i];
    charging_rate = stod(charging_rate_string);

#ifdef DEBUG
    cout << "charging_rate_string = " << charging_rate_string << ", charging_rate = " << charging_rate << endl;
#endif

    // std::set<std::string> validOperationPolicyOptions = {"optimal_unidirectional", "safe_unidirectional", "hybrid_unidirectional", "optimal_bidirectional", "hybrid_bidirectional", "safe_bidirectional", "hybrid_bidirectional", "no_ev"};

    std::string operationPolicyInput = argv[++i];

    // if (validOperationPolicyOptions.find(operationPolicyInput) == validOperationPolicyOptions.end())
    // {
    //     std::cerr << "Invalid Operation policy: " << operationPolicyInput << std::endl;
    //     exit(EXIT_FAILURE);
    // }

    Operation_policy = operationPolicyInput;

    string path_to_ev_data_string = argv[++i];
    path_to_ev_data = path_to_ev_data_string;
    // cout << "path_to_ev_data_string = " << path_to_ev_data_string << endl;
    wfh_type = extract_wfh_type(path_to_ev_data);
    out_path = argv[++i];
    // cout << "wfh_type = " << wfh_type << endl;

#ifdef DEBUG
    cout << " path_to_ev_data = " << path_to_ev_data << endl;
#endif

    chunk_size = days_in_chunk * 24 / T_u;
    number_of_chunks = 100;

    chunk_step = (load.size() / T_yr) * solar.size() / number_of_chunks;

    return 0;
}
