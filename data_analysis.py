"""
Task 4 - Data Analytics
Reads temperature and frequency data recorded from Arduino temperature_optimisation.ino
and generates plots for time-domain and frequency-domain analysis.

Name: Zaid Haris
ID: F533590
"""


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
plt.close()

# Plot 2 - Magnitude vs Frequency (first DFT cycle only)
plt.figure(figsize=(10, 4))
plt.plot(freq[:32], mag[:32], color='darkorange', linewidth=1)
dominant_k = np.argmax(mag[:32])
plt.axvline(x=freq[dominant_k], color='red', linestyle='--', label=f'Dominant: {freq[dominant_k]:.4f} Hz')
plt.title('Plot 2: Magnitude vs Frequency')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Magnitude')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('plot2_magnitude_vs_frequency.png', dpi=150, bbox_inches='tight')
plt.close()

# Plot 3 - Smoothed Temperature vs Time
smoothed_temp = pd.Series(temp).rolling(window=10).mean()

plt.figure(figsize=(10, 4))
plt.plot(time, temp, color='steelblue', linewidth=1, alpha=0.5, label='Original')
plt.plot(time, smoothed_temp, color='red', linewidth=2, label='Smoothed (MA=10)')
plt.title('Plot 3: Smoothed Temperature vs Time')
plt.xlabel('Time (s)')
plt.ylabel('Temperature (°C)')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('plot3_smoothed_temperature.png', dpi=150, bbox_inches='tight')
plt.close()

# Plot 4 - Histogram of Temperature Readings
plt.figure(figsize=(10, 4))
plt.hist(temp, bins=20, color='steelblue', edgecolor='black')
plt.title('Plot 4: Histogram of Temperature Readings')
plt.xlabel('Temperature (°C)')
plt.ylabel('Frequency')
plt.grid(True)
plt.tight_layout()
plt.savefig('plot4_temperature_histogram.png', dpi=150, bbox_inches='tight')
plt.close()