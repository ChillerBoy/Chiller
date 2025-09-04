#ifndef FLOWSENSORS_H
#define FLOWSENSORS_H

#include <Arduino.h>

struct FlowSensorConfig {
  uint8_t pin;
  const char* name;
};

class FlowSensors {
  public:
    FlowSensors(FlowSensorConfig* configs, uint8_t count);
    void begin();
    void update();
    void printFlows();
    float getGPM(const char* name);

  private:
    FlowSensorConfig* _configs;
    uint8_t _count;
    volatile unsigned long* _pulseCounts;
    float* _flowRates;
    unsigned long* _lastMillis;

    void resetCounts();
    int findIndexByName(const char* name);
};

// ISR handlers
void IRAM_ATTR flowISR0();
void IRAM_ATTR flowISR1();
void IRAM_ATTR flowISR2();

#endif
