#include <math.h>

#define LOG_SAMPLES 180
#define DFT_SAMPLES 32

const int B = 4275000;
const int R0 = 100000;
const int pinTempSensor = A0;

float tempData[LOG_SAMPLES];

static float realPart[DFT_SAMPLES];
static float imagPart[DFT_SAMPLES];
static float freqOut[DFT_SAMPLES];

int collect_temperature_data(unsigned long duration);
float* apply_dft(float* data, int N, float fs, float* magOut);

void setup()
{
  Serial.begin(9600);
}

int collect_temperature_data(unsigned long duration)
{
  unsigned long startTime = millis();
  unsigned long lastSampleTime = startTime;
  int index = 0;

  while ((millis() - startTime < duration) && (index < LOG_SAMPLES))
  {
    unsigned long currentTime = millis();

    if (currentTime - lastSampleTime >= 1000)
    {
      int a = analogRead(pinTempSensor);

      if (a == 0)
      {
        tempData[index] = -999.0;
      }
      else
      {
        float R = 1023.0 / a - 1.0;
        R = R0 * R;
        float temperature = 1.0 / (log(R / R0) / B + 1 / 298.15) - 273.15;
        tempData[index] = temperature;
      }

      index++;
      lastSampleTime += 1000;
    }
    else
    {
      delay(1);
    }
  }

  return index;
}

float* apply_dft(float* data, int N, float fs, float* magOut) {
  for (int k = 0; k < N; k++) {
    realPart[k] = 0.0;
    imagPart[k] = 0.0;

    for (int n = 0; n < N; n++) {
      float angle = 2.0 * PI * k * n / N;
      realPart[k] += data[n] * cos(angle);
      imagPart[k] -= data[n] * sin(angle);
    }

    magOut[k] = sqrt(realPart[k] * realPart[k] + imagPart[k] * imagPart[k]);
    freqOut[k] = (float)k * fs / N;
  }
  return freqOut;
}

void loop()
{
  int totalSamples = collect_temperature_data(180000);

  int dftStart = totalSamples - DFT_SAMPLES;
  if (dftStart < 0) dftStart = 0;
  
  float magnitude[DFT_SAMPLES];
  float samplingFreq = 1.0;
  
  float* frequencies = apply_dft(&tempData[dftStart], DFT_SAMPLES, samplingFreq, magnitude);
  
  Serial.println("--- DFT Results ---");
  Serial.println("Index,Freq(Hz),Magnitude");
  for (int k = 1; k < DFT_SAMPLES; k++) {
    Serial.print(k);
    Serial.print(",");
    Serial.print(frequencies[k], 4);
    Serial.print(",");
    Serial.println(magnitude[k], 4);
  }
  Serial.println("--- End DFT ---");

  for (int i = 0; i < totalSamples; i++)
  {
    if (tempData[i] <= -999.0) continue;
    
    Serial.print("temperature = ");
    Serial.println(tempData[i]);
  }

  delay(5000);
}