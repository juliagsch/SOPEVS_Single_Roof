import re

def extract_result(output):
    """Helper function to extract values from the sim output."""
    patterns = {
        "grid_import": r"Grid import: (\d+\.?\d*)",
        "total_cost": r"Total Cost: (\d+\.?\d*)",
        "total_hours": r"Total Hours: (\d+\.?\d*)",
        "total_load": r"Total load: (\d+\.?\d*)",
        "grid_emissions": r"Grid Emissions: (\d+\.?\d*)",
        "ev_power_used": r"EV Power Usage: (\d+\.?\d*)",
        "household_load": r"Total Household Load: (\d+\.?\d*)",
        "power_lost": r"Power Lost: (\d+\.?\d*)",
        "ev_power_charged": r"EV Power Charged: (\d+\.?\d*)",
        "ev_battery_diff": r"EV Battery Diff: (\d+\.?\d*)",
    }

    extracted_values = {}
    for key, pattern in patterns.items():
        match = re.search(pattern, output)
        extracted_values[key] = float(match.group(1)) if match else None

    return extracted_values