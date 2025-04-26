#include <cmath>
#include <iostream>
#include <vector>
#include <utility>
#include "simulate_system.h"
#include "ev.h"
#include "common.h"

using namespace std;

int loss_events = 0;
double load_deficit = 0;

void update_parameters(double n)
{

	num_cells = n;

	a1_intercept = 0.0 * num_cells;

	a2_intercept = kWh_in_one_cell * num_cells;

	alpha_d = a2_intercept * 1.0;
	alpha_c = a2_intercept * 1.0;
	return;
}

// Get maximum charging for stationary battery
double calc_max_charging(double power, double b_prev)
{

	double step = power / 30.0;

	for (double c = power; c >= 0; c -= step)
	{
		double upper_lim = a2_slope * (c / nominal_voltage_c) + a2_intercept;
		double b = b_prev + c * eta_c * T_u;
		if (b <= upper_lim)
		{
			return c;
		}
	}
	return 0;
}

// Get maximum discharging from stationary battery
double calc_max_discharging(double power, double b_prev)
{

	double step = power / 30.0;

	for (double d = power; d >= 0; d -= step)
	{
		double lower_lim = a1_slope * (d / nominal_voltage_d) + a1_intercept;
		double b = b_prev - d * eta_d * T_u;
		if (b >= lower_lim)
		{
			return d;
		}
	}
	return 0;
}

// Get maximum charging for EV battery
double calc_max_charging_ev(double power, double ev_b)
{
	double step = power / 30.0;
	double upper_lim = max_soc * ev_battery_capacity;
	double ev_b_m = 0.0;

	for (double c = power; c > 0; c -= step)
	{
		ev_b_m = ev_b + c * eta_c_ev * T_u;
		if (ev_b_m < upper_lim)
		{
			return c;
		}
	}
	return 0;
}

// Get maximum discharging from EV battery
double calc_max_discharging_ev(double power, double ev_b, double min_soc, double ev_battery_size)
{
	double step = power / 30.0;
	double lower_lim = min_soc * ev_battery_capacity;

	for (double d = power; d >= 0; d -= step)
	{
		// Update battery SOC
		double ev_b_m = ev_b - d * eta_d_ev * T_u;

		// Check if the updated SOC is above the lower limit
		if (ev_b_m >= lower_lim)
		{
			return d;
		}
	}
	return 0;
}

// Returns true if the EV should currently be charging, false otherwise
bool get_is_charging(const std::vector<EVStatus> &dailyStatuses, double ev_b, int currentHour)
{
	// Find the next departure hour
	int next_dept = -1;
	if (dailyStatuses[currentHour].isAtHome && ev_b < (max_soc * ev_battery_capacity - 0.1))
	{
		next_dept = convertTimeToHour(dailyStatuses[currentHour].nextDepartureTime);
	}
	if (next_dept == -1)
	{
		return false;
	}

	// Compute how many hours are needed to reach the max SOC.
	double diff = ev_battery_capacity * max_soc - ev_b;
	int hours_needed = ceil(diff / (charging_rate * eta_c_ev * T_u));

	// Find the number of hours until departure.
	int hours_until_dept = (24 + next_dept - currentHour) % 24;

	// Note: It is possible that we don't quite reach the full battery level if we discharge in the hour before charging starts.
	// This is insignificant as the EV battery update logic was changed to subtract the electricity used from the previous battery level instead
	// of assuming a full battery at departure and reading the battery level at arrival.
	if (hours_needed >= hours_until_dept)
	{
		return true;
	}
	return false;
}

// Called when operational policy "no_ev" is chosen
std::pair<double, double> no_ev(double b, double c, double d, int hour, double grid_intensity)
{
	// There is leftover solar power after covering household and expected EV load
	if (c > 0)
	{
		double max_c = fmin(calc_max_charging(c, b), alpha_c);
		b = b + max_c * eta_c * T_u;
		stat_charged += max_c;
	}

	// Electricity is missing to cover household  load
	else if (d > 0)
	{
		double max_d = fmin(calc_max_discharging(d, b), alpha_d);
		b = b - max_d * eta_d * T_u;
		stat_discharged += max_d;
		double res = d - max_d;
		if (res > 0)
		{
			loss_events += 1;
			load_deficit += res;
			grid_import += res;
			grid_emissions += res * grid_intensity;

			if (hour == 0 || hour == 1 || hour == 2 || hour == 3 || hour == 4)
			{
				// special grid tariff for home load
				total_cost += res * 0.07;
			}
			else
			{
				total_cost += res * 0.35;
			}
		}
	}

	return std::make_pair(0, b);
}

// Called when operational policy "safe_unidirectional" is chosen
std::pair<double, double> safe_unidirectional(double b, double ev_b, double c, double d, bool isCharging, double maxCharging, double isHome, int hour, double grid_intensity)
{
	if (isCharging == true)
	{
		ev_b = ev_b + maxCharging * eta_c_ev * T_u;
		power_lost += (maxCharging * T_u) - (maxCharging * eta_c_ev * T_u);
		ev_charged += maxCharging;
		ChargingEvent event;
		event.hour = hour;
		event.chargingAmount = maxCharging;
		chargingEvents.push_back(event);
	}

	// There is leftover solar power after covering household and expected EV load
	if (c > 0)
	{
		double max_c = fmin(calc_max_charging(c, b), alpha_c);
		b = b + max_c * eta_c * T_u;
		stat_charged += max_c;
	}

	// Electricity is missing to cover household and expected EV load
	else if (d > 0)
	{
		double max_d = fmin(calc_max_discharging(d, b), alpha_d);
		b = b - max_d * eta_d * T_u;
		stat_discharged += max_d;
		double res = d - max_d;
		if (res > 0)
		{
			loss_events += 1;
			load_deficit += res;
			grid_import += res;
			grid_emissions += res * grid_intensity;

			if (hour == 0 || hour == 1 || hour == 2 || hour == 3 || hour == 4)
			{
				// special grid tariff for home load
				total_cost += res * 0.07;
			}
			else
			{
				total_cost += fmin(maxCharging, res) * 0.07;
				res -= fmin(maxCharging, res);
				total_cost += res * 0.35;
			}
		}
	}

	return std::make_pair(ev_b, b);
}

// Called when operational policy "hybrid_bidirectional" is chosen
std::pair<double, double> hybrid_bidirectional(double b, double ev_b, double c, double d, bool isCharging, double maxCharging, bool isHome, int hour, double grid_intensity)
{
	if (isCharging == true)
	{
		ev_b = ev_b + maxCharging * eta_c_ev * T_u;
		power_lost += (maxCharging * T_u) - (maxCharging * eta_c_ev * T_u);
		ev_charged += maxCharging; // TODO: Depending on use case this should be maxCharging * eta_c_ev * T_u
		ChargingEvent event;
		event.hour = hour;
		event.chargingAmount = maxCharging; // TODO: Depending on use case this should be maxCharging * eta_c_ev * T_u
		chargingEvents.push_back(event);
	}

	// There is leftover solar power after covering household and expected EV load
	if (c > 0)
	{
		// First we use the remaining the power the EV if if is home and if it hasn't been powered already.
		if (isHome == true && isCharging == false)
		{
			double max_c_ev = calc_max_charging_ev(fmin(c, charging_rate), ev_b);
			ev_b = ev_b + max_c_ev * eta_c_ev * T_u;
			power_lost += (max_c_ev * T_u) - (max_c_ev * eta_c_ev * T_u);
			ev_charged += max_c_ev; // TODO: Depending on use case this should be max_c_ev * eta_c_ev * T_u
			if (max_c_ev > 0)
			{
				ChargingEvent event;
				event.hour = hour;
				event.chargingAmount = max_c_ev; // TODO: Depending on use case this should be max_c_ev * eta_c_ev * T_u
				chargingEvents.push_back(event);
			}
			c -= max_c_ev;
			total_load += max_c_ev;
			max_charging_total += max_c_ev;
		}
		// If there is still electricity remaining, we charge the stationary battery.
		if (c > 0)
		{
			double max_c = fmin(calc_max_charging(c, b), alpha_c);
			b = b + max_c * eta_c * T_u;
			stat_charged += max_c; // TODO: Depending on use case this should be max_c * eta_c * T_u
		}
	}

	// Electricity is missing to cover household and expected EV load
	else if (d > 0)
	{
		// First cover load from stationary battery
		double max_d = fmin(calc_max_discharging(d, b), alpha_d);
		b = b - max_d * eta_d * T_u;
		stat_discharged += max_d;
		double res = d - max_d;

		// If energy still missing, discharge EV
		if (res > 0 && isCharging == false && isHome == true)
		{
			double max_d_ev = fmin(calc_max_discharging_ev(res, ev_b, min_soc, ev_battery_capacity), discharging_rate / (eta_d_ev * T_u));
			ev_b = ev_b - max_d_ev * eta_d_ev * T_u;
			power_lost += (max_d_ev * eta_d_ev * T_u) - (max_d_ev * T_u);
			ev_discharged += max_d_ev; // TODO: Depending on use case this should be maxCharging * eta_c_ev * T_u
			res = res - max_d_ev;

			// The electricity discharged from the EV needs to be excluded from the total load as it has already been included when the EV was charged.
			total_load -= max_d_ev;
		}

		// We need to import from grid if there is still electricity missing
		if (res > 0)
		{
			loss_events += 1;
			load_deficit += res;
			grid_import += res;
			grid_emissions += res * grid_intensity;
			if (hour == 0 || hour == 1 || hour == 2 || hour == 3 || hour == 4)
			{
				// special grid tariff for home load
				total_cost += res * 0.07;
			}
			else
			{
				total_cost += fmin(maxCharging, res) * 0.07;
				res -= fmin(maxCharging, res);
				total_cost += res * 0.35;
			}
		}
	}
	return std::make_pair(ev_b, b);
}

double get_maxCharging(double ev_b)
{
	double available_power = charging_rate * T_u;
	return calc_max_charging_ev(available_power, ev_b);
}

// Get current EV battery level. It is the same as in previous step except if the EV arrives from a trip. In that case,
// the electricity used for driving is subtracted from the previous battery level.
double get_ev_b(std::vector<EVStatus> &dailyStatuses, int hour, double last_soc)
{
	double ev_b = last_soc;
	if (hour != 0 && !dailyStatuses[hour - 1].isAtHome && dailyStatuses[hour].isAtHome)
	{
		ev_b -= dailyStatuses[hour - 1].powerUsed;
		ev_power_used += dailyStatuses[hour - 1].powerUsed;
	}

	return ev_b;
}

// call it with a specific battery and PV size and want to compute the loss
double sim(vector<double> &load_trace, vector<double> &solar_trace, vector<double> &intensity_trace, int start_index, int end_index, double cells, double pv, double b_0, std::vector<EVRecord> evRecords, std::vector<std::vector<EVStatus>> allDailyStatuses, double max_soc, double min_soc, int Ev_start)
{
	update_parameters(cells);
	loss_events = 0;
	load_deficit = 0;
	load_sum = 0;
	grid_import = 0;
	grid_emissions = 0;
	total_load = 0;
	power_lost = 0;
	max_charging_total = 0;
	double initial_battery_level_ev = 0;
	double last_soc = 0;

	double b = 0.0; // Start simulation with an empty stationary battery
	double c = 0.0; // Remaining solar energy after covering household and EV charging load
	double d = 0.0; // Missing energy to cover household and EV charging load after using produced solar energy.
	// Operation_policy = "no_ev";
	// We assume that the initial battery charge of the EV comes from the grid.
	if (Operation_policy != "no_ev")
	{
		initial_battery_level_ev = ev_battery_capacity * max_soc; // EV battery level at the start of the simulation
		last_soc = initial_battery_level_ev;					  // EV battery level during the previous time step

		grid_import = initial_battery_level_ev / eta_c_ev;
		total_load = grid_import;
		total_cost = grid_import * 0.07;
		power_lost = grid_import - initial_battery_level_ev;
		max_charging_total = grid_import;
		grid_emissions += grid_import * 124; // Use average grid intensity of the year 2024. Source: https://www.carbonbrief.org/analysis-uks-electricity-was-cleanest-ever-in-2024/
	}

	int index_t_solar;	   // Current index in solar trace
	int index_t_load;	   // Current index in load trace
	int index_t_intensity; // Current index in intensity trace

	int trace_length_solar = solar_trace.size();
	int trace_length_load = load_trace.size();
	int trace_length_intensity = intensity_trace.size();

	// Get start day of the simulation
	int trace_days = min(trace_length_load / 24, trace_length_solar / 24);
	int load_s_Start_day = rand() % trace_days;
	start_index = load_s_Start_day * 24;

	for (int day = 0; day < days_in_chunk; day++)
	{
		int ev_day = day + Ev_start;
		ev_day = ev_day % 365;

		for (int hour = 0; hour < 24; hour++)
		{
			total_hours += 1;
			int t = day * 24 + start_index + hour;
			index_t_solar = t % trace_length_solar;
			index_t_load = t % trace_length_load;
			index_t_intensity = t % trace_length_intensity;
			load_sum += load_trace[index_t_load];

			double current_intensity = intensity_trace[index_t_intensity];

			std::pair<double, double> operationResult;
			// Household doesn't have an EV
			if (Operation_policy == "no_ev")
			{
				double hourly_load = load_trace[index_t_load];
				total_load += hourly_load;

				c = solar_trace[index_t_solar] * pv - hourly_load; // Remaining solar power after covering load
				d = hourly_load - solar_trace[index_t_solar] * pv; // Missing power to cover load

				operationResult = no_ev(b, c, d, hour, current_intensity);
			}
			// Household has EV
			else
			{
				bool isCharging = false;
				bool isHome = allDailyStatuses[ev_day][hour].isAtHome;
				double ev_b = get_ev_b(allDailyStatuses[ev_day], hour, last_soc);
				double maxCharging = 0.0;
				if (Operation_policy == "safe_unidirectional")
				{
					if (isHome == true)
					{
						maxCharging = get_maxCharging(ev_b);
					}

					if (maxCharging > 0)
					{
						isCharging = true;
					}

					double hourly_load = load_trace[index_t_load] + maxCharging;
					total_load += hourly_load;

					c = solar_trace[index_t_solar] * pv - hourly_load; // Remaining solar power after covering load
					d = hourly_load - solar_trace[index_t_solar] * pv; // Missing power to cover load

					operationResult = safe_unidirectional(b, ev_b, c, d, isCharging, maxCharging, isHome, hour, current_intensity);
				}
				else if (Operation_policy == "hybrid_bidirectional")
				{
					// Check if EV should currently be charging
					isCharging = get_is_charging(allDailyStatuses[ev_day], ev_b, hour);
					if (isCharging)
					{
						maxCharging = get_maxCharging(ev_b);
					}

					double hourly_load = load_trace[index_t_load] + maxCharging;
					total_load += hourly_load;

					c = solar_trace[index_t_solar] * pv - hourly_load; // Remaining solar power after covering load
					d = hourly_load - solar_trace[index_t_solar] * pv; // Missing power to cover load

					operationResult = hybrid_bidirectional(b, ev_b, c, d, isCharging, maxCharging, isHome, hour, current_intensity);
				}
				else
				{
					std::cout << "ERROR: Invalid operation policy selected" << std::endl;
				}
				max_charging_total += maxCharging;

				// Update battery levels
				ev_b = operationResult.first;
				std::cout << ev_b << endl;
				last_soc = ev_b;
			}
			b = operationResult.second;
		}
	}
	ev_battery_diff = last_soc;
	// std::cout << "Final EV Battery level: " << last_soc << " Final Stationary Battery Level: " << b << " EV power used: " << ev_power_used << " household load used " << load_sum << " max charging " << max_charging_total << " power loss " << power_lost << " load+car+powerloss " << ev_power_used + load_sum + power_lost << std::endl;
	if (metric == 0)
	{
		// lolp
		return loss_events / ((end_index - start_index) * 1.0);
	}
	else
	{
		// metric == 1, eue
		return load_deficit / (load_sum * 1.0);
	}
}

void simulate(vector<double> &load_trace, vector<double> &solar_trace, vector<double> &intensity_trace, int start_index, int end_index, double b_0, std::vector<EVRecord> evRecords, std::vector<std::vector<EVStatus>> allDailyStatuses, double max_soc, double min_soc, int Ev_start)
{
	double loss = 0.0;
	loss = sim(load_trace, solar_trace, intensity_trace, start_index, end_index, 0, 4, b_0, evRecords, allDailyStatuses, max_soc, min_soc, Ev_start);
}