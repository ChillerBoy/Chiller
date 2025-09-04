#ifndef DIGITALINPUTS_H
#define DIGITALINPUTS_H

#include <Arduino.h>

// ================== CONFIG STRUCT ==================
struct DigitalInputConfig {
  uint8_t pin;         // Input pin
  const char* name;    // Label
  bool critical;       // true = safety trip, false = fault only
};

// ================== CLASS ==================
class DigitalInputs {
public:
  DigitalInputs(DigitalInputConfig* configs, uint8_t count);

  void begin();
  void update();
  void printStates();

  bool checkCriticalTrip();   // true if any critical input tripped
  bool checkAnyFault();       // true if any input (critical or fault) tripped
  bool getStateByName(const char* name);

private:
  DigitalInputConfig* configs;
  uint8_t count;
  bool* states;   // true = input active, false = inactive
};

#endif
