# AI Prompt Engineering Example

I studied prompt engineering techniques and applied it throughout the coursework to ensure that any AI output was correct and suitable for use. Key techniques used include: providing clear context, breaking tasks into logical steps, specifying constraints, and requesting exact output formats.
(https://developers.openai.com/api/docs/guides/prompt-engineering)

## Example Prompt — apply_dft() — 30 April 2026

This prompt was used to generate the apply_dft() function. The logical 
steps and DFT equations were planned and understood before prompting. 
AI was used to translate these into working C code.

Role: You are an experienced professional specialising in Arduino C.

Context: Continuing with the Arduino project. I have collected temperature data 
stored in a global array float tempData[180]. I need to apply a Discrete Fourier 
Transform to a recent window of this data to convert it from the time domain to 
the frequency domain, following these exact equations from the coursework brief:

Eq 3.1: X[k] = Σ_{n=0}^{N-1} x[n] · e^{-j 2πkn/N}
Eq 3.2: f_k = (k · f_s) / N
Eq 3.3: real[k] = Σ_{n=0}^{N-1} x[n] · cos(2πkn/N)
Eq 3.4: imag[k] = - Σ_{n=0}^{N-1} x[n] · sin(2πkn/N)
Eq 3.5: Magnitude[k] = sqrt( real[k]^2 + imag[k]^2 )

Task: Write a function float* apply_dft(float* data, int N, float fs, float* magOut) 
that follows the logical steps in order:
Prepare storage using the pre-declared static arrays realPart[32], imagPart[32] 
and freqOut[32] — do not declare new arrays inside the function
Iterate through each frequency index k from 0 to N-1
Reset realPart[k] and imagPart[k] accumulators to 0.0 at the start of each k iteration
Iterate through each time sample index n from 0 to N-1
Calculate the phase angle: angle = (2.0 * PI * k * n) / N
Accumulate the real part using Eq 3.3: realPart[k] += data[n] * cos(angle)
Accumulate the imaginary part using Eq 3.4: imagPart[k] -= data[n] * sin(angle)
After the inner loop completes, compute magnitude using Eq 3.5 and store in magOut[k]
Convert index k to actual frequency in Hz using Eq 3.2 and store in freqOut[k]
Return a pointer to the freqOut array

Constraints:
Arduino Uno has only 2KB SRAM — no dynamic allocation, no malloc, no new
Use windowed processing on a subset of tempData (window size = N = 32 samples)
Use only the pre-declared static buffers: realPart[32], imagPart[32], freqOut[32]
The function must compile and run correctly on Arduino hardware

Output: Provide only the apply_dft function code block, do not rewrite the whole file.

Here is the code: 
@@ -1,70 +1,21 @@
// Loovee @ 2015-8-26
#include <math.h>

#define MAX_SAMPLES 180  // 3 minutes at 1 Hz

const int B = 4275000;     // B value of the thermistor
const int R0 = 100000;     // R0 = 100k
const int B = 4275000; // B value of the thermistor
const int R0 = 100000; // R0 = 100k
const int pinTempSensor = A0; // Grove - Temperature Sensor connect to A0

float tempData[MAX_SAMPLES];

void setup()
{
  Serial.begin(9600);
}

int collect_temperature_data(unsigned long duration)
{
  unsigned long startTime = millis();
  unsigned long lastSampleTime = startTime;
  int index = 0;

  while ((millis() - startTime < duration) && (index < MAX_SAMPLES))
  {
    unsigned long currentTime = millis();

    if (currentTime - lastSampleTime >= 1000)
    {
      int a = analogRead(pinTempSensor);

      if (a == 0)
      {
        tempData[index] = NAN;  // handle sensor error safely
      }
      else
      {
        float R = 1023.0 / a - 1.0;
        R = R0 * R;
        float temperature = 1.0 / (log(R / R0) / B + 1 / 298.15) - 273.15;
        tempData[index] = temperature;
      }

      index++;
      lastSampleTime += 1000;  // prevents timing drift
    }
    else
    {
      delay(1);  // reduce CPU usage
    }
  }

  return index;
}

void loop()
{
  int samples = collect_temperature_data(180000); // 3 minutes

  for (int i = 0; i < samples; i++)
  {
    Serial.print("temperature = ");
    Serial.println(tempData[i]);
  }

  delay(5000);
}


//26-27 April: Added collect_temperature_data function
//27-28 April: Learnt DFT theory
  int a = analogRead(pinTempSensor);
  float R = 1023.0/a-1.0;
  R = R0*R;
  float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
  Serial.print("temperature = ");
  Serial.println(temperature);
  delay(100);
}

## What I reviewed and changed
- Verified all equations matched the coursework brief (Eq 3.1-3.5)
- Tested the function on hardware and confirmed correct DFT output
- Added comments explaining the logic in my own words
- Integrated into the full sketch with the existing buffer structure