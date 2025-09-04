#include "RefPressureSensors.h"
#include "SensorLabels.h"

RefPressureSensors::RefPressureSensors(const RefPressureSensorConfig* configs, uint8_t count)
: cfg(configs), n(count) {
  psi = new float[n];
  for (uint8_t i=0;i<n;i++) psi[i] = NAN;
}

void RefPressureSensors::begin() { analogReference(DEFAULT); }

void RefPressureSensors::update() {
  for (uint8_t i=0;i<n;i++) psi[i] = readPsi(cfg[i].pin, cfg[i].rangePsi);
}

void RefPressureSensors::printPressures(Stream& io) const {
  for (uint8_t i=0;i<n;i++) {
    io.print(cfg[i].label); io.print(": ");
    io.print(psi[i]); io.println(" psig");
  }
}

float RefPressureSensors::getPressure(uint8_t index) const {
  return (index < n) ? psi[index] : NAN;
}

float RefPressureSensors::readPsi(uint8_t pin, float range) {
  const uint16_t samples = 8;
  uint32_t acc = 0;
  for (uint16_t i=0;i<samples;i++) acc += analogRead(pin);
  float adc = float(acc) / samples;
  float v = (adc / 1023.0f) * 5.0f;
  float x = (v - 0.5f) / 4.0f;
  if (x < 0) x = 0; if (x > 1) x = 1;
  return x * range;
}
