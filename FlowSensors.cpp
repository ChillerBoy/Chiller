#include "FlowSensors.h"

// Static storage for pulse counts (for ISRs)
static volatile unsigned long pulseCount0 = 0;
static volatile unsigned long pulseCount1 = 0;
static volatile unsigned long pulseCount2 = 0;

// Map ISR calls to counters
void IRAM_ATTR flowISR0() { pulseCount0++; }
void IRAM_ATTR flowISR1() { pulseCount1++; }
void IRAM_ATTR flowISR2() { pulseCount2++; }

FlowSensors::FlowSensors(FlowSensorConfig* configs, uint8_t count) {
  _configs = configs;
  _count = count;

  _pulseCounts = new volatile unsigned long[_count];
  _flowRates   = new float[_count];
  _lastMillis  = new unsigned long[_count];

  for (uint8_t i = 0; i < _count; i++) {
    _pulseCounts[i]()_
