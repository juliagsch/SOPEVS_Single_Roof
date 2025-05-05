import pandas as pd

# Read the CSV file into a DataFrame
df = pd.read_csv("./policy_data/ev_UK/merged_T2_UK.csv")

# Sum the values in the 'Distance (km)' column
total_distance = df['Distance (km)'].sum()

total_emissions = total_distance * 166.85 / 1000

print(f"Total Distance (km): {total_distance}")
print(f"Total CO2 Emissions (kg): {total_emissions}")
#Total Distance (km): 2431.65
#Total CO2 Emissions (Megatonnes): 406
