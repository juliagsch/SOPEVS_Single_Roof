"""Tests are written for the case when we have an EV but no PV."""
import os
import pytest
import subprocess
from test_utils import extract_result

test_data_path = os.path.abspath("./tests/test_data")
eta_ev = 0.935 # Charging and discharging efficiency of the EV
intensity_path = f"{test_data_path}/intensities.txt"

def test_unidirectional():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op = "safe_unidirectional"
    num_days = 1
    command = f"./bin_nopvbutev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4 {intensity_path}"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result = extract_result(result.stdout)
    
    initial_ev_charge = 48 / eta_ev
    assert result["ev_power_used"] == 2
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    assert result["total_load"] == pytest.approx(24 + result["ev_power_charged"], 1e-6)
    assert result["total_load"] == result["grid_import"]
    assert result["grid_emissions"] == pytest.approx(initial_ev_charge*124 + 3600 + 1.973333*200, 1e-6)
    assert result["ev_power_charged"] == pytest.approx(result["ev_power_used"]+result["ev_battery_diff"]+result["power_lost"], 1e-6)
    assert result["total_load"] == pytest.approx(result["household_load"]+result["ev_power_used"]+result["ev_battery_diff"]+result["power_lost"], 1e-6)
    assert result["total_cost"] == pytest.approx(result["ev_power_charged"]*0.07 + 19*0.35 + 5*0.07, 1e-6)
    assert result["power_lost"] == pytest.approx(result["ev_power_charged"]*(1-eta_ev), 1e-6)

def test_bidirectional():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op = "hybrid_bidirectional"
    num_days = 1
    command = f"./bin_nopvbutev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4 {intensity_path}"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result = extract_result(result.stdout)
    
    initial_ev_charge = 48 / eta_ev
    assert result["ev_power_used"] == 2
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    assert result["total_load"] == result["grid_import"]
    # Addition can lead to small rounding error, therefore we need to approximate.
    assert result["total_load"] == pytest.approx(result["household_load"]+result["ev_power_used"]+result["power_lost"]+result["ev_battery_diff"], 1e-6)
    assert result["total_cost"] == pytest.approx(result["ev_power_charged"]*0.07 + 4*0.35, 1e-6) # We can always cover the household load from the EV battery except if it is away or charging (2 hours driving, 2h charging)
    assert result["power_lost"] == pytest.approx((result["ev_power_charged"])*(1-eta_ev)+(20*((1.0 / eta_ev)-1)), 1e-6) # We discharge 18 kWh from the EV battery
    assert result["grid_emissions"] == pytest.approx(initial_ev_charge*124 + (result["ev_power_charged"]-initial_ev_charge)*100 + 3*100 + 200, 1e-6)

def test_bidirectional_two_trips():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev2.csv"
    op = "hybrid_bidirectional"
    num_days = 1
    command = f"./bin_nopvbutev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4 {intensity_path}"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result = extract_result(result.stdout)
    
    assert result["ev_power_used"] == 6
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    assert result["total_load"] == result["grid_import"]
    # Addition can lead to small rounding error, therefore we need to approximate.
    assert result["total_load"] == pytest.approx(result["household_load"]+result["ev_power_used"]+result["power_lost"]+result["ev_battery_diff"], 1e-6)
    assert result["total_cost"] == pytest.approx(result["ev_power_charged"]*0.07 + 8*0.35, 1e-6) # We can always cover the household load from the EV battery except if it is away or charging (3 hours driving, 5h charging)

def test_uni_vs_bi():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op_uni = "safe_unidirectional"
    op_bi = "hybrid_bidirectional"
    num_days = 1
    command_uni = f"./bin_nopvbutev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_uni} {ev_file_path} 0 4 {intensity_path}"
    command_bi = f"./bin_nopvbutev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_bi} {ev_file_path} 0 4 {intensity_path}"

    result_uni = subprocess.run(command_uni.split(), stdout=subprocess.PIPE, text=True)
    result_bi = subprocess.run(command_bi.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result_uni = extract_result(result_uni.stdout)
    result_bi = extract_result(result_bi.stdout)

    assert result_uni["ev_power_used"] == result_bi["ev_power_used"]
    assert result_uni["household_load"] == result_bi["household_load"]
    assert result_uni["power_lost"] < result_bi["power_lost"]
    assert result_uni["ev_power_charged"] < result_bi["ev_power_charged"]
