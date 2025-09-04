#pragma once
namespace ACH580 {
  constexpr uint16_t REG_SPEED_REF      = 1100; // example
  constexpr uint16_t REG_START_CMD      = 2000; // example
  constexpr uint16_t REG_FAULT_CODE     = 3000; // example
  inline uint16_t pct_to_abb(float pct){ pct = constrain(pct,0.0f,100.0f); return (uint16_t)round(pct*20.0f); }
}
