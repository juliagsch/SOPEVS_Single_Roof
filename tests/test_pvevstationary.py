"""Tests are written for the case when we have both PV and EV. We have a 4kW PV system with a 4kWh stationary battery."""
import os
import pytest
import subprocess
from test_utils import extract_result

test_data_path = os.path.abspath("./tests/test_data")
eta_ev = 0.935 # Charging and discharging efficiency of the EV
eta_d_stationary = 1 / 0.9 # Discharging efficiency of stationary battery
eta_c_stationary = 0.9942 # Charging efficiency of stationary battery
intensity_path = f"{test_data_path}/intensities.txt"

def test_unidirectional():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op = "safe_unidirectional"
    num_days = 1
    command = f"./bin_pv_ev_storage/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4 {intensity_path}"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result = extract_result(result.stdout)
    
    assert result["ev_power_used"] == 2
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    assert result["total_load"] > result["grid_import"]
    assert result["ev_power_charged"] == pytest.approx(result["ev_power_used"]+result["ev_battery_diff"]+result["power_lost"], 1e-6)
    assert result["total_load"] == pytest.approx(result["household_load"]+result["ev_power_used"]+result["ev_battery_diff"]+result["power_lost"], 1e-6)
    assert result["power_lost"] == pytest.approx(result["ev_power_charged"]-result["ev_power_charged"]*eta_ev, 1e-6)

def test_bidirectional():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op = "hybrid_bidirectional"
    num_days = 1
    command = f"./bin_pv_ev_storage/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4 {intensity_path}"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)
    print(command)
    # Extract numbers from output
    result = extract_result(result.stdout)
    
    assert result["ev_power_used"] == 2
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    assert result["total_load"] > result["grid_import"]
    # Addition can lead to small rounding error, therefore we need to approximate.
    assert result["total_load"] == pytest.approx(result["household_load"]+result["ev_power_used"]+result["power_lost"]+result["ev_battery_diff"], 1e-6)

def test_uni_vs_bi():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op_uni = "safe_unidirectional"
    op_bi = "hybrid_bidirectional"
    num_days = 1
    command_uni = f"./bin_pv_ev_storage/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_uni} {ev_file_path} 0 4 {intensity_path}"
    command_bi = f"./bin_pv_ev_storage/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_bi} {ev_file_path} 0 4 {intensity_path}"

    result_uni = subprocess.run(command_uni.split(), stdout=subprocess.PIPE, text=True)
    result_bi = subprocess.run(command_bi.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result_uni = extract_result(result_uni.stdout)
    result_bi = extract_result(result_bi.stdout)

    assert result_uni["ev_power_used"] == 2
    assert result_uni["household_load"] == 24
    assert result_uni["ev_power_used"] == result_bi["ev_power_used"]
    assert result_uni["household_load"] == result_bi["household_load"]
    assert result_uni["grid_import"] > result_bi["grid_import"]
    assert result_uni["power_lost"] < result_bi["power_lost"]
    assert result_uni["ev_power_charged"] < result_bi["ev_power_charged"]

def test_stationary_vs_nostationary_uni():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op_uni = "safe_unidirectional"
    num_days = 1
    command_stationary = f"./bin_pv_ev_storage/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_uni} {ev_file_path} 0 4 {intensity_path}"
    command_nostationary = f"./bin_pvandev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_uni} {ev_file_path} 0 4 {intensity_path}"

    command_stationary = subprocess.run(command_stationary.split(), stdout=subprocess.PIPE, text=True)
    command_nostationary = subprocess.run(command_nostationary.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    command_stationary = extract_result(command_stationary.stdout)
    command_nostationary = extract_result(command_nostationary.stdout)

    assert command_stationary["ev_power_used"] == 2
    assert command_stationary["household_load"] == 24
    assert command_stationary["ev_power_used"] == command_nostationary["ev_power_used"]
    assert command_stationary["household_load"] == command_nostationary["household_load"]
    assert command_stationary["grid_import"] < command_nostationary["grid_import"]
    assert command_stationary["power_lost"] == command_nostationary["power_lost"]
    assert command_stationary["ev_power_charged"] == command_nostationary["ev_power_charged"]
    assert command_stationary["total_cost"] <= command_nostationary["total_cost"]

def test_stationary_vs_nostationary_bi():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op = "hybrid_bidirectional"
    num_days = 1
    command_stationary = f"./bin_pv_ev_storage/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4 {intensity_path}"
    command_nostationary = f"./bin_pvandev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4 {intensity_path}"

    command_stationary = subprocess.run(command_stationary.split(), stdout=subprocess.PIPE, text=True)
    command_nostationary = subprocess.run(command_nostationary.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    command_stationary = extract_result(command_stationary.stdout)
    command_nostationary = extract_result(command_nostationary.stdout)

    assert command_stationary["ev_power_used"] == 2
    assert command_stationary["household_load"] == 24
    assert command_stationary["ev_power_used"] == command_nostationary["ev_power_used"]
    assert command_stationary["household_load"] == command_nostationary["household_load"]
    assert command_stationary["grid_import"]-command_stationary["ev_battery_diff"] < command_nostationary["grid_import"]-command_nostationary["ev_battery_diff"]
    assert command_stationary["power_lost"] < command_nostationary["power_lost"]
    assert command_stationary["ev_power_charged"] == command_nostationary["ev_power_charged"]
    assert command_stationary["total_cost"] == command_nostationary["total_cost"]
