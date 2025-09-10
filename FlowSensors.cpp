
#include "FlowSensors.h"

volatile unsigned long pulseCount0 = 0;
volatile unsigned long pulseCount1 = 0;
volatile unsigned long pulseCount2 = 0;

void IRAM_ATTR flowISR0() { pulseCount0++; }
void IRAM_ATTR flowISR1() { pulseCount1++; }
void IRAM_ATTR flowISR2() { pulseCount2++; }

FlowSensors::FlowSensors(FlowSensorConfig* cfg, uint8_t count)
: _cfg(cfg), _count(count) {
  _pulseCounts = new volatile unsigned long[_count];
  _gpm = new float[_count];
  _lastMs = new unsigned long[_count];
  for (uint8_t i=0;i<_count;i++){
    _pulseCounts[i] = 0;
    _gpm[i] = 0.0f;
    _lastMs[i] = millis();
  }
}

void FlowSensors::begin() {
  for (uint8_t i=0;i<_count;i++) {
    pinMode(_cfg[i].pin, INPUT_PULLUP);
    // attach interrupts based on slot index (0..2)
    if (i==0) attachInterrupt(digitalPinToInterrupt(_cfg[i].pin), flowISR0, RISING);
    else if (i==1) attachInterrupt(digitalPinToInterrupt(_cfg[i].pin), flowISR1, RISING);
    else if (i==2) attachInterrupt(digitalPinToInterrupt(_cfg[i].pin), flowISR2, RISING);
  }
}

void FlowSensors::resetCounts() {
  for (uint8_t i=0;i<_count;i++) _pulseCounts[i] = 0;
}

void FlowSensors::update() {
  unsigned long now = millis();
  for (uint8_t i=0;i<_count;i++) {
    unsigned long dt = now - _lastMs[i];
    if (dt >= 1000) { // 1 second window
      // Copy ISR counters atomically
      noInterrupts();
      unsigned long pulses = (i==0? pulseCount0 : (i==1? pulseCount1 : pulseCount2));
      if (i==0) pulseCount0=0; else if (i==1) pulseCount1=0; else pulseCount2=0;
      interrupts();

      float perSecond = (float)pulses / (float)dt * 1000.0f;
      // Convert to GPM using k_factor (pulses per gallon). perSecond / k = gal/sec -> *60 to GPM
      float gal_per_sec = (_cfg[i].k_factor > 0.0f) ? (perSecond / _cfg[i].k_factor) : 0.0f;
      _gpm[i] = gal_per_sec * 60.0f;
      _lastMs[i] = now;
    }
  }
}

float FlowSensors::getGPM(uint8_t idx) const {
  if (idx >= _count) return 0.0f;
  return _gpm[idx];
}
