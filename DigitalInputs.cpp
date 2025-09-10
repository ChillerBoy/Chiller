#include "DigitalInputs.h"

// ================== CONSTRUCTOR ==================
DigitalInputs::DigitalInputs(DigitalInputConfig* configs, uint8_t count) {
  this->configs = configs;
  this->count = count;
  states = new bool[count];
  for (uint8_t i = 0; i < count; i++) states[i] = false;
}

// ================== BEGIN ==================
void DigitalInputs::begin() {
  for (uint8_t i = 0; i < count; i++) {
    pinMode(configs[i].pin, INPUT_PULLUP); // assume N.O. switches
  }
}

// ================== UPDATE ==================
void DigitalInputs::update() {
  for (uint8_t i = 0; i < count; i++) {
    int val = digitalRead(configs[i].pin);
    // Switch is normally open → closed = fault/trip
    states[i] = (val == LOW);
  }
}

// ================== PRINT ==================
void DigitalInputs::printStates() {
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(configs[i].name);
    Serial.print(": ");
    if (states[i]) {
      Serial.println("ACTIVE");
    } else {
      Serial.println("OK");
    }
  }
}

// ================== CHECKERS ==================
bool DigitalInputs::checkCriticalTrip() {
  for (uint8_t i = 0; i < count; i++) {
    if (configs[i].critical && states[i]) {
      Serial.print("!!! CRITICAL TRIP: ");
      Serial.println(configs[i].name);
      return true;
    }
  }
  return false;
}

bool DigitalInputs::checkAnyFault() {
  for (uint8_t i = 0; i < count; i++) {
    if (states[i]) {
      Serial.print("FAULT: ");
      Serial.println(configs[i].name);
      return true;
    }
  }
  return false;
}

bool DigitalInputs::getStateByName(const char* name) {
  for (uint8_t i = 0; i < count; i++) {
    if (strcmp(configs[i].name, name) == 0) {
      return states[i];
    }
  }
  return false;
}
