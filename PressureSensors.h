#ifndef PRESSURESENSORS_H
#define PRESSURESENSORS_H

#include <Arduino.h>

// ================== CONFIG STRUCT ==================
struct PressureSensorConfig {
  uint8_t pin;        // Analog input pin
  float rangePSI;     // Full-scale PSI (e.g., 80, 100)
  const char* name;   // Label
};

// ================== CLASS ==================
class PressureSensors {
public:
  PressureSensors(PressureSensorConfig* configs, uint8_t count);

  void begin();
  void update();
  void printPressures();

  float getPressure(uint8_t index);
  float getByName(const char* name);

  float getDeltaP(const char* suctionName, const char* dischargeName);

private:
  PressureSensorConfig* configs;
  uint8_t count;
  float* values;
};

#endif
