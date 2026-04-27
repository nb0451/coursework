// Loovee @ 2015-8-26
#include <math.h>

#define MAX_SAMPLES 180  // 3 minutes at 1 Hz

const int B = 4275000;     // B value of the thermistor
const int R0 = 100000;     // R0 = 100k
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