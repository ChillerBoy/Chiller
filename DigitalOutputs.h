#ifndef DIGITALOUTPUTS_H
#define DIGITALOUTPUTS_H

#include <Arduino.h>

// Map your DOs to stable indices so you never sprinkle pin numbers in logic
enum DOIndex : uint8_t {
  DO_AHU_PUMP = 0,          // pin 22
  DO_EVAP_PUMP,             // pin 23
  DO_SAND_FILTER_PUMP,      // pin 24
  DO_CT_PUMP,               // pin 25
  DO_ALARM,                 // pin 26 (latched alarm output)
  DO_FAN3,                  // pin 28
  DO_FAN2,                  // pin 29
  DO_FAN1,                  // pin 30
  DO_CT_FAN_ENABLE,         // pin 31
  DO_COUNT
};

struct DigitalOutputConfig {
  uint8_t pin;         // Arduino pin
  const char* name;    // Human-readable label
  bool activeHigh;     // true if relay is driven active HIGH
};

class DigitalOutputs {
public:
    bool getState(int idx) const; 

    inline int getCount() const { return count; }

  DigitalOutputs(const DigitalOutputConfig* cfg, uint8_t count);

  void begin();
  void allOff();

  void setOutput(uint8_t index, bool on);
  void setByName(const char* name, bool on);

  bool get(uint8_t index) const;
  bool getByName(const char* name) const;

  void printStates() const;

private:
  const DigitalOutputConfig* cfg;
  uint8_t count;
  bool state[DO_COUNT];   // cached state (true = logical ON)
};

extern DigitalOutputs digitalOutputs;

#endif
