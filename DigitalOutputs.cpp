#include "DigitalOutputs.h"

// ====== You will instantiate `digitalOutputs` in your main .ino with your config ======
DigitalOutputs::DigitalOutputs(const DigitalOutputConfig* cfg, uint8_t count)
  : cfg(cfg), count(count)
{
  // Initialize cached states to OFF
  for (uint8_t i = 0; i < DO_COUNT; i++) state[i] = false;
}

void DigitalOutputs::begin() {
  for (uint8_t i = 0; i < count; i++) {
    pinMode(cfg[i].pin, OUTPUT);
    // Default safe: OFF
    if (cfg[i].activeHigh) {
      digitalWrite(cfg[i].pin, LOW);
    } else {
      digitalWrite(cfg[i].pin, HIGH);
    }
    state[i] = false;
  }
}

void DigitalOutputs::allOff() {
  for (uint8_t i = 0; i < count; i++) {
    if (cfg[i].activeHigh) {
      digitalWrite(cfg[i].pin, LOW);
    } else {
      digitalWrite(cfg[i].pin, HIGH);
    }
    state[i] = false;
  }
}

void DigitalOutputs::setOutput(uint8_t index, bool on) {
  if (index >= count) return;
  state[index] = on;
  if (cfg[index].activeHigh) {
    digitalWrite(cfg[index].pin, on ? HIGH : LOW);
  } else {
    digitalWrite(cfg[index].pin, on ? LOW : HIGH);
  }
}

void DigitalOutputs::setByName(const char* name, bool on) {
  for (uint8_t i = 0; i < count; i++) {
    if (strcmp(cfg[i].name, name) == 0) {
      setOutput(i, on);
      return;
    }
  }
}

bool DigitalOutputs::get(uint8_t index) const {
  if (index >= count) return false;
  return state[index];
}

bool DigitalOutputs::getByName(const char* name) const {
  for (uint8_t i = 0; i < count; i++) {
    if (strcmp(cfg[i].name, name) == 0) return state[i];
  }
  return false;
}

void DigitalOutputs::printStates() const {
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(cfg[i].name);
    Serial.print(" (pin ");
    Serial.print(cfg[i].pin);
    Serial.print("): ");
    Serial.println(state[i] ? "ON" : "OFF");
  }
}
