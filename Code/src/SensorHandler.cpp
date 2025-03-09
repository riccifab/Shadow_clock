#include "SensorHandler.h"

float calibrateSensor(int adcValue) {
  // Convert ADC reading to voltage (for ADS1115 with gain = 1)
  float voltage = (adcValue * 4.096) / 32767.0;
  // Replace the next line with your actual calibration conversion.
  float distance = voltage;
  return distance;
}
