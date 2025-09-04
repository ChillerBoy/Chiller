#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H

#include <Arduino.h>
#include "src/TempSensorIDs.h"

// Map logical registers to TempSensorID indices so the rest of the system stays stable.
enum HoldingRegister : uint16_t {
  REG_TEMP_BASE = 1000,   // starting register for temps (example)
  // each temp uses two registers if 16-bit (x10) scaling, or one 32-bit float if using Modbus floats
};

// Example helpers: scale F to 0.1F for 16-bit holding register
inline int16_t F_to_reg10(float f){ return (int16_t)roundf(f*10.0f); }

#endif
