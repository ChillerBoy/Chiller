#include "AnalogOutputs.h"

// ================== CONSTRUCTOR ==================
AnalogOutputs::AnalogOutputs(AnalogOutputConfig* configs, uint8_t count) {
  this->configs = configs;
  this->count = count;
  dacs = new Adafruit_MCP4725[count];
  percents = new float[count];
  for (uint8_t i = 0; i < count; i++) percents[i] = 0.0;
}

// ================== BEGIN ==================
void AnalogOutputs::begin() {
  Wire.begin();
  for (uint8_t i = 0; i < count; i++) {
    dacs[i].begin(configs[i].i2cAddr);
    setPercent(i, 0.0); // start at 0% (0V)
  }
}

// ================== SET PERCENT ==================
void AnalogOutputs::setPercent(uint8_t index, float percent) {
  if (index >= count) return;

  // Clamp to 0..100
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  // GLOBAL POLICY: any non-zero command gets a >= 30% bias (~3V)
  if (percent > 0.0f && percent < 30.0f) percent = 30.0f;

  percents[index] = percent;

  uint16_t dacVal = percentToDAC(percent);
  dacs[index].setVoltage(dacVal, false);
}

void AnalogOutputs::setByName(const char* name, float percent) {
  for (uint8_t i = 0; i < count; i++) {
    if (strcmp(configs[i].name, name) == 0) {
      setPercent(i, percent);
      return;
    }
  }
}

// ================== GET PERCENT ==================
float AnalogOutputs::getPercent(uint8_t index) const {
  if (index >= count) return NAN;
  return percents[index];
}

// ================== PRINT ==================
void AnalogOutputs::printStates() const {
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(configs[i].name);
    Serial.print(": ");
    Serial.print(percents[i], 1);
    Serial.println("%");
  }
}

// ================== HELPER: MAP PERCENT → DAC ==================
uint16_t AnalogOutputs::percentToDAC(float percent) const {
  // DAC is 12-bit (0–4095), ref = 5V out
  // Output range: 3V (idle) to 10V (full)
  // External scaling expands 0–5V → 0–10V, so we output 1.5–5.0V for 3–10V final
  float vOut = 1.5 + (percent / 100.0f) * 3.5; // 1.5–5.0 V
  if (percent <= 0.0f) vOut = 0.0f;            // true 0% yields 0V
  uint16_t dacVal = (uint16_t)((vOut / 5.0f) * 4095.0f);
  return dacVal;
}
