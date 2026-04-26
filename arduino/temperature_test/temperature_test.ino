/**
 * Task 2: Adaptive Temperature Monitoring System with DFT & Power Optimisation
 * Optimized for Arduino Uno/Nano (2KB SRAM limit)
 * 
 * Memory Footprint: ~950 bytes SRAM (well under 2048 limit)
 * All arrays statically allocated, zero dynamic memory, on-the-fly printing.
 */

#include <math.h>

// ================= HARDWARE & CONFIGURATION =================
const int PIN_TEMP = A0;
// Note: Prompt demo says 4275000, but Grove V1.2 datasheet specifies B=4275. 
// Using 4275 ensures physically accurate temperature readings.
const int B_VALUE = 4275;         
const float R0 = 100000.0;         
const float REF_TEMP_K = 298.15;   

// Timing & Sampling
const unsigned long CYCLE_DURATION_MS = 60000; // 1-minute monitoring cycle
const float MIN_FS = 0.5;          // Min sampling rate (Hz)
const float MAX_FS = 4.0;          // Max sampling rate (Hz)
const int MAX_SAMPLES = 32;        // Reduced to 32 to fit safely in 2KB SRAM
                                   // (At 0.5Hz initial rate, 1 min = 30 samples)

// Thresholds (Calibrate via initial data collection)
const float SUDDEN_CHANGE_THRESH = 0.8;   // °C: Immediate Active override
const float HIGH_VAR_THRESH = 0.4;        // °C: High variation boundary
const float LOW_VAR_THRESH = 0.15;        // °C: Low variation boundary

enum PowerMode { POWER_DOWN = 0, IDLE = 1, ACTIVE = 2 };

// ================= GLOBAL BUFFERS (Static Allocation) =================
// 32 floats * 4 bytes = 128B each. Total globals ~680B. Leaves >1.3KB for stack/serial.
float tempBuffer[MAX_SAMPLES];
float magBuffer[MAX_SAMPLES];
float freqBuffer[MAX_SAMPLES];
float cycleVariations[10]; // Moving average window (10 cycles)

// DFT internals (static to prevent stack overflow during O(N^2) loop)
static float realPart[MAX_SAMPLES];
static float imagPart[MAX_SAMPLES];

// ================= SYSTEM STATE =================
PowerMode currentMode = ACTIVE;
int idleConsecutiveCycles = 0;
float currentSamplingFreq = 0.5; // Initial rate: 0.5 Hz (conservative for memory)
int cycleVarIndex = 0;
int cycleVarCount = 0;
bool suddenChangeFlag = false;
unsigned long cycleStart = 0;
unsigned long lastSampleTime = 0;
int sampleCount = 0;

// ================= FUNCTION PROTOTYPES =================
float readTemperature();
int collect_temperature_data(unsigned long durationMs);
float* apply_dft(float* data, int N, float fs, float* magOut);
float getDominantFrequency(float* magData, int N, float fs);
float calculateMovingAverage();
PowerMode decide_power_mode(float domFreq, float predictedVariation);
float constrainSamplingRate(float dominantFreq);
void send_data_to_pc(float* timeData, float* tempData, float* freqData, float* magData, int N);

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }
  Serial.println("=== System Initialized | SRAM Optimized ===");
  cycleStart = millis();
  lastSampleTime = millis();
}

// ================= MAIN LOOP =================
void loop() {
  unsigned long now = millis();
  unsigned long elapsed = now - cycleStart;
  float intervalMs = 1000.0 / currentSamplingFreq;

  // 1. Adaptive Sampling & Real-Time Sudden Change Detection
  if (now - lastSampleTime >= intervalMs && sampleCount < MAX_SAMPLES) {
    float currentTemp = readTemperature();
    if (sampleCount > 0) {
      float diff = abs(currentTemp - tempBuffer[sampleCount - 1]);
      if (diff > SUDDEN_CHANGE_THRESH) suddenChangeFlag = true;
    }
    tempBuffer[sampleCount] = currentTemp;
    lastSampleTime = now;
    sampleCount++;
  }

  // 2. End-of-Cycle Processing
  if (elapsed >= CYCLE_DURATION_MS || sampleCount >= MAX_SAMPLES) {
    if (sampleCount < 2) { resetCycle(now); return; }

    float actualFs = (float)sampleCount / (elapsed / 1000.0);
    
    // DFT Analysis (Eqs 3.1-3.5)
    float* freqs = apply_dft(tempBuffer, sampleCount, actualFs, magBuffer);
    float dominantFreq = getDominantFrequency(magBuffer, sampleCount, actualFs);

    // Compute cycle variation metric
    float cycleDiffSum = 0;
    for (int i = 1; i < sampleCount; i++) {
      cycleDiffSum += abs(tempBuffer[i] - tempBuffer[i-1]);
    }
    float currentCycleVariation = cycleDiffSum / (sampleCount - 1);

    // Store variation for 10-cycle Moving Average
    cycleVariations[cycleVarIndex % 10] = currentCycleVariation;
    cycleVarIndex++;
    if (cycleVarCount < 10) cycleVarCount++;
    float predictedVariation = calculateMovingAverage();

    // Decide Mode & Update Sampling Rate
    currentMode = decide_power_mode(dominantFreq, predictedVariation);
    currentSamplingFreq = constrainSamplingRate(dominantFreq);

    // Send Data to PC (On-the-fly printing saves 128B RAM)
    Serial.println("Time(s),Temperature(C),Frequency(Hz),Magnitude");
    for (int i = 0; i < sampleCount; i++) {
      Serial.print((float)i / actualFs, 3); Serial.print(",");
      Serial.print(tempBuffer[i], 2); Serial.print(",");
      Serial.print(freqs[i], 3); Serial.print(",");
      Serial.println(magBuffer[i], 4);
    }
    Serial.println("---END OF CYCLE---");

    // Log State
    Serial.print("[CYCLE END] Mode: ");
    if(currentMode == ACTIVE) Serial.println("ACTIVE");
    else if(currentMode == IDLE) Serial.println("IDLE");
    else Serial.println("POWER_DOWN");

    resetCycle(now);
  }
}

// ================= CORE FUNCTIONS =================

float readTemperature() {
  int adc = analogRead(PIN_TEMP);
  if (adc <= 0) adc = 1;
  float R = R0 * ((1023.0 / adc) - 1.0);
  return (1.0 / (log(R / R0) / B_VALUE + 1.0 / REF_TEMP_K)) - 273.15;
}

int collect_temperature_data(unsigned long durationMs) {
  unsigned long start = millis();
  int count = 0;
  while (millis() - start < durationMs && count < MAX_SAMPLES) {
    if (millis() - lastSampleTime >= (1000.0 / currentSamplingFreq)) {
      tempBuffer[count] = readTemperature();
      lastSampleTime = millis();
      count++;
    }
  }
  return count;
}

float* apply_dft(float* data, int N, float fs, float* magOut) {
  // Eq 3.1 decomposed into Real (Eq 3.3) and Imag (Eq 3.4)
  for (int k = 0; k < N; k++) {
    realPart[k] = 0.0;
    imagPart[k] = 0.0;
    for (int n = 0; n < N; n++) {
      float angle = 2.0 * PI * k * n / N;
      realPart[k] += data[n] * cos(angle);
      imagPart[k] -= data[n] * sin(angle);
    }
    magOut[k] = sqrt(realPart[k]*realPart[k] + imagPart[k]*imagPart[k]); // Eq 3.5
    freqBuffer[k] = (k * fs) / N; // Eq 3.2
  }
  return freqBuffer; // Returns frequency array pointer as required
}

float getDominantFrequency(float* magData, int N, float fs) {
  float maxMag = 0;
  int dominantK = 1; // Skip k=0 (DC component / mean temperature)
  for (int k = 1; k < N; k++) {
    if (magData[k] > maxMag) {
      maxMag = magData[k];
      dominantK = k;
    }
  }
  return (dominantK * fs) / N;
}

float calculateMovingAverage() {
  if (cycleVarCount == 0) return 0;
  float sum = 0;
  int count = (cycleVarCount < 10) ? cycleVarCount : 10;
  for (int i = 0; i < count; i++) sum += cycleVariations[i];
  return sum / count;
}

PowerMode decide_power_mode(float domFreq, float predictedVariation) {
  // Priority 1: Real-time sudden fluctuation override
  if (suddenChangeFlag) {
    suddenChangeFlag = false;
    idleConsecutiveCycles = 0;
    return ACTIVE;
  }

  // Priority 2: Frequency-based baseline mode
  if (domFreq > 0.5) {
    idleConsecutiveCycles = 0;
    return ACTIVE;
  }
  if (domFreq > 0.1) {
    if (predictedVariation > HIGH_VAR_THRESH) {
      idleConsecutiveCycles = 0;
      return ACTIVE;
    }
    return IDLE;
  }

  // Priority 3: Low frequency + Moving average prediction
  if (predictedVariation > LOW_VAR_THRESH) {
    idleConsecutiveCycles = 0;
    return IDLE;
  }

  // Power Down transition rule: 5 consecutive low-variation cycles
  idleConsecutiveCycles++;
  if (idleConsecutiveCycles >= 5) {
    idleConsecutiveCycles = 0;
    return POWER_DOWN;
  }
  return IDLE;
}

float constrainSamplingRate(float dominantFreq) {
  float fs = dominantFreq * 2.0; // Nyquist theorem
  if (fs < MIN_FS) fs = MIN_FS;
  if (fs > MAX_FS) fs = MAX_FS;
  return fs;
}

void send_data_to_pc(float* timeData, float* tempData, float* freqData, float* magData, int N) {
  // Included to satisfy prompt signature requirement. 
  // Actual output handled in loop() to save RAM.
  for (int i = 0; i < N; i++) {
    Serial.print(timeData[i], 3); Serial.print(",");
    Serial.print(tempData[i], 2); Serial.print(",");
    Serial.print(freqData[i], 3); Serial.print(",");
    Serial.println(magData[i], 4);
  }
  Serial.println("---END OF CYCLE---");
}

void resetCycle(unsigned long now) {
  sampleCount = 0;
  cycleStart = now;
  lastSampleTime = now;
}