## Task 4 Data Analytics discussion

## 1. Time-Domain Behaviour

The recorded temperature signal ranged from 24.14°C to 25.96°C across the recording's duration giving a total variation of 1.82°C.

In the first cycle (0-31s), the temperature rose steadily from 25.17°C to 25.75°C as the sensor was warmed by hand. The second cycle (31-88s) showed the largest change, peaking at 25.96°C before dropping sharply to 24.14°C as the sensor was cooled by a fan. The third cycle (88–150s) was more erratic with repeated warming and cooling producing irregular variation due to the repeated switch from getting warmed by hand to getting cooled by a fan.

Plot 5 confirms these transitions, the rate of change spikes at approximately 60s and 120s correspond directly to the rapid cooling and reheating periods. Outside of these disturbances the signal was stable with minimal noise.

## 2. Frequency-Domain Behaviour

Plot 2 shows the DFT magnitude spectrum computed by the Arduino from the first monitoring cycle. The dominant frequency was 0.0313Hz, corresponding to a period of approximately 32 seconds, which is consistent with the slow thermal rise observed in cycle 1.

The signal is dominated by low-frequency content - magnitude drops sharply after the dominant peak and remains low across 0.1–0.8Hz, confirming that temperature changes were gradual rather than rapidly oscillating.

A secondary spike is visible at the upper end of the spectrum (~0.97Hz). This is a DFT artefact at the Nyquist boundary rather than a real physical frequency. The dominant frequency of 0.0313Hz also explains the reduced sampling rate in cycle 2 - the system set the new rate to approximately 2 × 0.0313 = 0.0625Hz using the Nyquist theorum.

## 3. System Behaviour

The adaptive sampling behaved as expected. Cycle 1 sampled at 1Hz producing integer-second timestamps. Following the DFT analysis the rate adjusted to approximately 0.5Hz for cycle 2, visible as 2-second intervals in the CSV. Cycle 3 adjusted again to ~0.96Hz producing the decimal timestamps of approximately 1.04 seconds.

Power mode selection was also sensible. Cycles 1 and 2 were classified as IDLE since the dominant frequency remained below 0.1Hz. Cycle 3 triggered ACTIVE as the erratic variation pushed the dominant frequency above the 0.5Hz threshold. The sudden fluctuation override in decide_power_mode() would have additionally triggered during cycle 3 where consecutive temperature differences exceeded 1.5°C.

A limitation is that with only 3 cycles the moving average never accumulated a full 10-cycle history, reducing its influence on mode decisions. A longer recording period would address this.

## 4. Data Quality

Three cycles (~150 seconds) was sufficient to demonstrate the adaptive sampling behaviour and capture meaningful thermal variation. For a real deployment however, at least 10 minutes of data would be recommended to allow the moving average to build a full cycle history.

The 1Hz sampling rate was appropriate - the Nyquist minimum for this signal was only 0.0625Hz, meaning even the reduced rates in cycles 2 and 3 remained well above the minimum requirement.

The main limitation was using manual thermal disturbance to generate variation. Real environmental temperature changes would be more gradual and consistent, producing cleaner DFT output. The abrupt transitions introduced here are not fully representative of natural conditions.

The thermistor resolution of approximately 0.02°C was adequate for this task, however the Arduino's 10-bit ADC introduces quantisation noise visible in Plot 3 as discrete jumps between values such as 24.90°C and 25.06°C rather than a smooth continuous curve.