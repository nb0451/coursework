#include <math.h>

#define LOG_SAMPLES 180      // Stores 3 minutes of data (1 Hz sampling)
#define DFT_SAMPLES 32       // Smaller window for DFT to fit SRAM limits

// Power mode constants (int for embedded simplicity)
#define MODE_ACTIVE 0
#define MODE_IDLE 1
#define MODE_POWER_DOWN 2

const int B = 4275000;       // B value of the thermistor
const int R0 = 100000;       // R0 = 100k
const int pinTempSensor = A0; // Grove - Temperature Sensor connect to A0

float tempData[LOG_SAMPLES]; // Global buffer for collected temperature data

// Static buffers for DFT to avoid stack overflow + dynamic allocation
static float realPart[DFT_SAMPLES];
static float imagPart[DFT_SAMPLES];
static float freqOut[DFT_SAMPLES];


int collect_temperature_data(unsigned long duration, float samplingRateHz);
float* apply_dft(float* data, int N, float fs, float* magOut);
void send_data_to_pc(float* timeData, float* tempData, float* freqData, float* magData, int N);
int decide_power_mode(float dominantFreq, float predictedVariation, float currentTemp, float lastTemp, int* idleCycles);


void setup()
{
  Serial.begin(9600); // Initialises serial communication for output
}

// Collects temperature data at 1 Hz for a given duration
int collect_temperature_data(unsigned long duration, float samplingRateHz)
{
  unsigned long startTime = millis();       // Records start time
  unsigned long lastSampleTime = startTime; // Tracks last sample time
  int index = 0;
  unsigned long intervalMs = (unsigned long)(1000.0 / samplingRateHz);

  // Continues until duration reached/buffer full
  while ((millis() - startTime < duration) && (index < LOG_SAMPLES))
  {
    unsigned long currentTime = millis();

    // Enforces dynamic sampling interval
    if (currentTime - lastSampleTime >= intervalMs)
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
      lastSampleTime += intervalMs;  // Maintains precise dynamic timing
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

int decide_power_mode(float dominantFreq, float predictedVariation, float currentTemp, float lastTemp, int* idleCycles) {
  const float FLUCTUATION_DELTA = 1.5;
  const float TREND_HIGH = 2.5;
  const float TREND_MODERATE = 1.5;
  const int PERSISTENCE_LIMIT = 5;

  // 1. Sudden fluctuation override (highest priority)
  if (lastTemp != 0.0 && fabs(currentTemp - lastTemp) > FLUCTUATION_DELTA) {
    *idleCycles = 0;
    return MODE_ACTIVE;
  }
  // 2. Frequency-based baseline selection
  else if (dominantFreq > 0.5) {
    *idleCycles = 0;
    return MODE_ACTIVE;
  }
  else if (dominantFreq > 0.1) {
    *idleCycles = 0;
    return MODE_IDLE;
  }
  // 3. Trend-based decision using moving average prediction
  else if (predictedVariation > TREND_HIGH) {
    *idleCycles = 0;
    return MODE_ACTIVE;
  }
  else if (predictedVariation > TREND_MODERATE) {
    *idleCycles = 0;
    return MODE_IDLE;
  }
  // 4. Low variation: apply 5-cycle persistence for POWER_DOWN (final fallback)
  else {
    (*idleCycles)++;
    if (*idleCycles >= PERSISTENCE_LIMIT) {
      return MODE_POWER_DOWN;
    } else {
      return MODE_IDLE;
    }
  }
}

void loop()
{
  static float currentFs = 1.0;
  static int idleCycles = 0;
  static float lastTemp = 0.0;
  static float variationHistory[10];
  static int historyIndex = 0;
  static int historyCount = 0;
  
  int totalSamples = collect_temperature_data(60000, currentFs);

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
  
  float* frequencies = apply_dft(dftInput, N, currentFs, magnitude);
  
  float timeData[DFT_SAMPLES];
  for (int i = 0; i < N; i++) {
    timeData[i] = (float)i / currentFs;
  }
  
  send_data_to_pc(timeData, &tempData[dftStart], frequencies, magnitude, N);

  int dominantK = 1;
  float maxMag = 0.0;
  for (int k = 1; k < N; k++) {
    if (magnitude[k] > maxMag) {
      maxMag = magnitude[k];
      dominantK = k;
    }
  }
  float dominantFreq = frequencies[dominantK];
  
  float variation = 0.0;
  float lastValid = -999.0;
  for (int i = 0; i < N; i++) {
    if (tempData[dftStart + i] > -999.0) {
      if (lastValid > -999.0) {
        variation += fabs(tempData[dftStart + i] - lastValid);
      }
      lastValid = tempData[dftStart + i];
    }
  }

  variationHistory[historyIndex] = variation;
  historyIndex = (historyIndex + 1) % 10;
  if (historyCount < 10) historyCount++;

  float predictedVariation = 0.0;
  for (int i = 0; i < historyCount; i++) predictedVariation += variationHistory[i];
  predictedVariation /= historyCount;
  
  float currentTemp = tempData[totalSamples - 1];
  if (currentTemp <= -999.0) currentTemp = 25.0; // Fallback if last sample invalid

  int currentMode = decide_power_mode(dominantFreq, predictedVariation, currentTemp, lastTemp, &idleCycles);
  lastTemp = currentTemp;
  
  float targetFs = 2.0 * dominantFreq;
  if (targetFs < 0.5) targetFs = 0.5;
  if (targetFs > 4.0) targetFs = 4.0;
  currentFs = targetFs;
  
  Serial.print("Mode: ");
  if (currentMode == MODE_ACTIVE) Serial.println("ACTIVE");
  else if (currentMode == MODE_IDLE) Serial.println("IDLE");
  else Serial.println("POWER_DOWN");

  delay(5000); // Waits before next full cycle
}

//All functions tested and correct - 1 May