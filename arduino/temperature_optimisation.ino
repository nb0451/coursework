#include <math.h>

#define LOG_SAMPLES 180      // Stores 3 minutes of data (1 Hz sampling)
#define DFT_SAMPLES 32       // Smaller window for DFT to fit SRAM limits

const int B = 4275000;       // B value of the thermistor
const int R0 = 100000;       // R0 = 100k
const int pinTempSensor = A0; // Grove - Temperature Sensor connect to A0

float tempData[LOG_SAMPLES]; // Global buffer for collected temperature data

// Static buffers for DFT to avoid stack overflow + dynamic allocation
static float realPart[DFT_SAMPLES];
static float imagPart[DFT_SAMPLES];
static float freqOut[DFT_SAMPLES];


int collect_temperature_data(unsigned long duration);
float* apply_dft(float* data, int N, float fs, float* magOut);
void send_data_to_pc(float* timeData, float* tempData, float* freqData, float* magData, int N);

void setup()
{
  Serial.begin(9600); // Initialises serial communication for output
}

// Collects temperature data at 1 Hz for a given duration
int collect_temperature_data(unsigned long duration)
{
  unsigned long startTime = millis();       // Records start time
  unsigned long lastSampleTime = startTime; // Tracks last sample time
  int index = 0;

  // Continues until duration reached/buffer full
  while ((millis() - startTime < duration) && (index < LOG_SAMPLES))
  {
    unsigned long currentTime = millis();

    // Enforces 1 Hz sampling interval
    if (currentTime - lastSampleTime >= 1000)
    {
      int a = analogRead(pinTempSensor); // Reads raw ADC value

      if (a == 0)
      {
        tempData[index] = -999.0;
      }
      else
      {
        // Converts ADC value to temperature using thermistor equation
        float R = 1023.0 / a - 1.0;
        R = R0 * R;
        float temperature = 1.0 / (log(R / R0) / B + 1 / 298.15) - 273.15;
        tempData[index] = temperature;
      }

      index++;                 // Moves to next storage index
      lastSampleTime += 1000;  // Maintains precise 1 Hz timing
    }
    else
    {
      delay(1); // Reduces CPU usage while waiting
    }
  }

  return index; // Returns number of valid samples collected
}

// Applies Discrete Fourier Transform to window of data
float* apply_dft(float* data, int N, float fs, float* magOut) {
  if (N > DFT_SAMPLES) return NULL;

  // Loop through each frequency bin
  for (int k = 0; k < N; k++) {
    realPart[k] = 0.0; // Resets real accumulator
    imagPart[k] = 0.0; // Resets imaginary accumulator

    // Sums contributions from all time samples
    for (int n = 0; n < N; n++) {
      float sample = data[n];
      if (sample <= -999.0) sample = 0.0;

      float angle = 2.0 * PI * k * n / N; // Computes phase angle
      float cosVal = cos(angle);
      float sinVal = sin(angle);
      realPart[k] += sample * cosVal; // Real component (cosine)
      imagPart[k] -= sample * sinVal; // Imaginary component (sine)
    }

    // Computes magnitude of complex result
    magOut[k] = sqrt(realPart[k] * realPart[k] + imagPart[k] * imagPart[k]);

    // Converts index to actual frequency
    freqOut[k] = (float)k * fs / N;
  }

  return freqOut; // Returns pointer to frequency array
}

void send_data_to_pc(float* timeData, float* tempData, float* freqData, float* magData, int N) {
  Serial.println("Time(s),Temperature(C),Frequency(Hz),Magnitude");
  for (int i = 0; i < N; i++) {
    if (tempData[i] <= -999.0) continue;
    Serial.print(timeData[i], 2); Serial.print(",");
    Serial.print(tempData[i], 2); Serial.print(",");
    Serial.print(freqData[i], 4); Serial.print(",");
    Serial.println(magData[i], 4);
  }
}

void loop()
{
  // Collects 3 minutes of temperature data
  int totalSamples = collect_temperature_data(180000);

  // Selects most recent window for DFT processing
  int dftStart = totalSamples - DFT_SAMPLES;
  if (dftStart < 0) dftStart = 0; // Ensures valid index
  
  int N = (totalSamples < DFT_SAMPLES) ? totalSamples : DFT_SAMPLES;

  float dftInput[DFT_SAMPLES];

  float mean = 0;
  for (int i = 0; i < N; i++) mean += tempData[dftStart + i];
  mean /= N;

  for (int i = 0; i < N; i++)
  {
    float val = tempData[dftStart + i];
    if (val <= -999.0) val = 0.0;
    dftInput[i] = val - mean;
  }
  
  float magnitude[DFT_SAMPLES]; // Buffer for DFT magnitudes
  float samplingFreq = 1.0;     // Sampling frequency (1 Hz)
  
  // Performs DFT on selected window
  float* frequencies = apply_dft(dftInput, N, samplingFreq, magnitude);
  
  // Generates time array for the DFT window
  float timeData[DFT_SAMPLES];
  for (int i = 0; i < N; i++) {
    timeData[i] = (float)i / samplingFreq;
  }
  
  // Sends combined CSV output
  send_data_to_pc(timeData, &tempData[dftStart], frequencies, magnitude, N);

  delay(5000); // Waits before next full cycle
}