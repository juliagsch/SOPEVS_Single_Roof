// common.h
#ifndef COMMON_H
#define COMMON_H

#include <vector>

using namespace std;

// INPUTS

extern double B_inv;  // cost per cell
extern double PV_inv; // cost per unit (kW) of PV
extern double epsilon;
extern double confidence;
extern int metric;
extern int days_in_chunk;

extern vector<double> load;
extern vector<double> solar;
extern vector<double> intensity;
extern double battery_result;
extern double pv_result;

// define the upper and lower values to test for battery cells and pv,
// as well as the step size of the search
extern double cells_min;
extern double cells_max;
extern double cells_step; // search in step of x cells

extern double pv_min;
extern double pv_max;
extern double pv_step; // search in steps of x kW

extern double min_soc;
extern double max_soc;
extern double ev_battery_capacity;
extern double charging_rate;
extern double discharging_rate;

extern std::string path_to_ev_data;
extern int loadNumber;
extern std::string wfh_type;

// common.h
extern std::string Operation_policy;

double static T_u = 1.0; // this is the time unit, representing the number of hours in each time slot of the load and solar traces. If changed, the simulation code will likely break.
double static kWh_in_one_cell = 0.011284;
double static num_cells_steps = 400; // search in total of n steps for cells
double static num_pv_steps = 350;	 // search in total of n steps for pv

extern int ev_charged;
extern int ev_discharged;
extern int stat_charged;
extern int stat_discharged;
extern double grid_import;
extern double total_load;
extern double grid_emissions;
extern double total_cost;
extern double total_hours;
extern double load_sum;			  // Total load used
extern double ev_power_used;	  // Total power used by ev driving (discharging to power house not included)
extern double power_lost;		  // Electricity lost due to charging and discharging efficiencies
extern double max_charging_total; // Total electricity used to charge the EV
extern double ev_battery_diff;	  // EV battery difference between beginning and start of the simulation

extern std::vector<double> socValues;

struct ChargingEvent
{
	int hour;
	double chargingAmount;
};

extern std::vector<ChargingEvent> chargingEvents;

struct SimulationResult
{

	double B;
	double C;
	double cost;

	SimulationResult(double B_val, double C_val, double cost_val) : B(B_val), C(C_val), cost(cost_val) {}
};

vector<double> read_data_from_file(string);

int process_input(char **, bool);

#endif
