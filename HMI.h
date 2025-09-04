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

// Scaled write (matches your current implementation: val is multiplied by 10 into int16)
void HMI_sendToHMI(uint16_t address, float value);

// NEW: raw register writes (no x10 scaling)
void HMI_writeRawU16(uint16_t address, uint16_t value);
void HMI_writeRawI16(uint16_t address, int16_t value);

// NEW: convenience for booleans (exact 0/1)
inline void HMI_writeBool(uint16_t address, bool on) {
    HMI_writeRawU16(address, on ? 1 : 0);
}

// Logging + Events
void HMI_sendEvent(uint16_t code, bool isAlarm);
void HMI_update();   // handle warning auto-clear

// Bulk sensor update (kept as-is)
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
