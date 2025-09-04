#pragma once
#include "register_map.h"
namespace HMIEvent {
  enum : uint16_t {
    ALM_CRITICAL_TRIP       = 2001,
    ALM_HIGH_DISCH_PRESS    = 2002,
    ALM_LOW_SUCTION_CUTOUT  = 2003,
    WRN_HIGH_DISCH_APPROACH = 3001,
  };
}
inline void HMI_sendAlarm(uint16_t code)   { HMI_sendToHMI(HMIReg::BASE_ALARMS,   code); }
inline void HMI_sendWarning(uint16_t code) { HMI_sendToHMI(HMIReg::BASE_WARNINGS, code); }
