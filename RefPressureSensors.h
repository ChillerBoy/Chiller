#ifndef REF_PRESSURE_SENSORS_H
#define REF_PRESSURE_SENSORS_H

#include <Arduino.h>

struct RefPressureSensorConfig {
  uint8_t pin;
  float rangePsi; // 435 psi sensors
  const char* label;
};

class RefPressureSensors {
public:
  RefPressureSensors(const RefPressureSensorConfig* configs, uint8_t count);
  void begin();
  void update();
  void printPressures(Stream& io = Serial) const;
  float getPressure(uint8_t index) const;

private:
  const RefPressureSensorConfig* cfg;
  uint8_t n;
  float* psi;
  static float readPsi(uint8_t pin, float range);
};

#endif
