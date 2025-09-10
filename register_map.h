
#pragma once
#include <Arduino.h>

enum class HMIReg : uint16_t {
  BASE_ALARMS   = 0x4000,
  BASE_WARNINGS = 0x4100
};
