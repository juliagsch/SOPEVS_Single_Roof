"""Tests are written for the case without PV or EV."""
import os
import pytest
import subprocess
from test_utils import extract_result

test_data_path = os.path.abspath("./tests/test_data")
eta_ev = 0.935 # Charging and discharging efficiency of the EV
intensity_path = f"{test_data_path}/intensities.txt"

def test_no_ev():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    num_days = 1
    command = f"./bin_nopvnoev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 no_ev {ev_file_path} 0 4 {intensity_path}"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result = extract_result(result.stdout)
    
    assert result["ev_power_used"] == 0
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    assert result["total_load"] == 24
    assert result["grid_import"] == 24
    assert result["grid_emissions"] == 3600
    assert result["ev_power_charged"] == 0
    assert result["total_cost"] == pytest.approx(19*0.35 + 5*0.07, 1e-6)
    assert result["power_lost"] == 0
