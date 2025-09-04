#include "PressureSensors.h"
#include "SensorLabels.h"

// ================== CONSTRUCTOR ==================
PressureSensors::PressureSensors(PressureSensorConfig* configs, uint8_t count) {
  this->configs = configs;
  this->count = count;
  values = new float[count];
  for (uint8_t i = 0; i < count; i++) values[i] = NAN;
}

// ================== BEGIN ==================
void PressureSensors::begin() {
  for (uint8_t i = 0; i < count; i++) {
    pinMode(configs[i].pin, INPUT);
  }
}

// ================== UPDATE ==================
void PressureSensors::update() {
  for (uint8_t i = 0; i < count; i++) {
    int raw = analogRead(configs[i].pin);
    float voltage = (raw / 1023.0) * 5.0;

    // Convert 0.5–4.5V to 0–rangePSI
    if (voltage < 0.5 || voltage > 4.5) {
      values[i] = NAN; // out of range / sensor fault
    } else {
      values[i] = ((voltage - 0.5) / 4.0) * configs[i].rangePSI;
    }
  }
}

// ================== PRINT ==================
void PressureSensors::printPressures() {
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(configs[i].name);
    Serial.print(": ");
    if (isnan(values[i])) {
      Serial.println("ERR");
    } else {
      Serial.print(values[i], 1);
      Serial.println(" psi");
    }
  }
}

// ================== GETTERS ==================
float PressureSensors::getPressure(uint8_t index) {
  if (index >= count) return NAN;
  return values[index];
}

float PressureSensors::getByName(const char* name) {
  for (uint8_t i = 0; i < count; i++) {
    if (strcmp(configs[i].name, name) == 0) {
      return values[i];
    }
  }
  return NAN;
}

// ================== DELTA P ==================
float PressureSensors::getDeltaP(const char* suctionName, const char* dischargeName) {
  float s = getByName(suctionName);
  float d = getByName(dischargeName);
  if (isnan(s) || isnan(d)) return NAN;
  return d - s;
}
