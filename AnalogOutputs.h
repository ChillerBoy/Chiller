#ifndef ANALOGOUTPUTS_H
#define ANALOGOUTPUTS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>

// ================== CONFIG STRUCT ==================
struct AnalogOutputConfig {
  uint8_t i2cAddr;     // I2C address (e.g. 0x60, 0x61)
  const char* name;    // Label
};

// ================== CLASS ==================
class AnalogOutputs {
public:
  AnalogOutputs(AnalogOutputConfig* configs, uint8_t count);

  void begin();
  void setPercent(uint8_t index, float percent);   // 0–100%
  void setByName(const char* name, float percent);
  float getPercent(uint8_t index) const;

  void printStates() const;

private:
  AnalogOutputConfig* configs;
  Adafruit_MCP4725* dacs;
  uint8_t count;
  float* percents;

  uint16_t percentToDAC(float percent) const;
};

#endif
