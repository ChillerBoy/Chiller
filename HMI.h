
#pragma once
#include <Arduino.h>

struct HMIEvent {
  uint16_t code;
  bool isAlarm;
  char message[40];
  uint32_t timestamp;
  bool active;
};

void HMI_begin(unsigned long baud);
void HMI_writeRawU16(uint16_t addr, uint16_t value);
void HMI_sendEvent(uint16_t code, bool isAlarm);

#include "register_map.h"
inline void HMI_writeRawU16(HMIReg reg, uint16_t value) { HMI_writeRawU16(static_cast<uint16_t>(reg), value); }
