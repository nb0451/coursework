import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df = pd.read_csv('temperature_data.csv')

for i in range(1, len(df)):
    if df.loc[i, 'Time(s)'] < df.loc[i-1, 'Time(s)']:
        df.loc[i:, 'Time(s)'] += df.loc[i-1, 'Time(s)']

time = df['Time(s)'].values
temp = df['Temperature(C)'].values
freq = df['Frequency(Hz)'].values
mag = df['Magnitude'].values

print(f"Loaded {len(df)} rows")
print(f"Temperature range: {temp.min():.2f} - {temp.max():.2f} °C")
print(f"Time range: {time.min():.2f} - {time.max():.2f} s")

# Plot 1 - Temperature vs Time
plt.figure(figsize=(10, 4))
plt.plot(time, temp, color='steelblue', linewidth=1)
plt.title('Plot 1: Temperature vs Time')
plt.xlabel('Time (s)')
plt.ylabel('Temperature (°C)')
plt.grid(True)
plt.tight_layout()
plt.savefig('plot1_temperature_vs_time.png', dpi=150, bbox_inches='tight')
plt.show()