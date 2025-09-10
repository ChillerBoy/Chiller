
#pragma once
#include <Arduino.h>

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

struct FlowSensorConfig {
  uint8_t pin;
  const char* name;
  float k_factor; // pulses per gallon (or scaling)
};

class FlowSensors {
public:
  FlowSensors(FlowSensorConfig* cfg, uint8_t count);
  void begin();
  void update();                 // call periodically to compute flow from pulses
  void resetCounts();
  uint8_t getCount() const { return _count; }
  float getGPM(uint8_t idx) const;  // returns gallons per minute estimate

private:
  FlowSensorConfig* _cfg;
  uint8_t _count;
  volatile unsigned long* _pulseCounts;
  float* _gpm;
  unsigned long* _lastMs;
};

// ISRs (support up to 3 sensors for now)
void IRAM_ATTR flowISR0();
void IRAM_ATTR flowISR1();
void IRAM_ATTR flowISR2();

extern volatile unsigned long pulseCount0;
extern volatile unsigned long pulseCount1;
extern volatile unsigned long pulseCount2;
