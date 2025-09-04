#ifndef HMI_H
#define HMI_H

#include <Arduino.h>

// ================== EVENT STRUCTS ==================
struct HMIEvent {
    uint16_t code;        // event code (100+ alarms, 200+ warnings)
    bool isAlarm;         // true = alarm, false = warning
    char message[40];     // human-readable description
    uint32_t timestamp;   // millis() when triggered
    bool active;          // active or cleared
};

// ================== API ==================
void HMI_begin(unsigned long baud);
void HMI_sendToHMI(uint16_t address, float value);

// Logging + Events
void HMI_sendEvent(uint16_t code, bool isAlarm);
void HMI_update();   // handle warning auto-clear

// Bulk sensor update
void HMI_sendAll(
  class WaterTempSensors& wts,
  class RefTempSensors& rts,
  class PressureSensors& wps,
  class RefPressureSensors& rps,
  class FlowSensors& fs,
  class DigitalInputs& di,
  class DigitalOutputs& dout,
  class AnalogOutputs& ao
);

#endif
