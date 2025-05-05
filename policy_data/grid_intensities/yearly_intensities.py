"""
Combines monthly grid intensities to get yearly trace. Grid intensities were downloaded from for the year 2024:
https://carbonintensity.org.uk/
"""
import pandas as pd

months = ['01','02','03','04','05','06','07','08','09','10','11','12']
year = 24

total = 0
count = 0
# Read half hourly grid intensities
for month in months:
    path = f"./policy_data/grid_intensities/monthly_traces/{month}{year}.csv"
    df = pd.read_csv(path)
    intensities = df['Actual Carbon Intensity (gCO2/kWh)']
    # Get hourly intensities by averaging half-hourly data
    averaged = intensities[::2].reset_index(drop=True) + intensities[1::2].reset_index(drop=True)
    averaged = averaged / 2

    with open("./policy_data/grid_intensities/grid_intensities24.csv", 'a') as f:
        for value in averaged:
            f.write(f"{value}\n")
            total += value
            count += 1

print("average intensity :", total/count)