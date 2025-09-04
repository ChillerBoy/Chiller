#pragma once
// Serial aliases (simple: HMI + DEBUG both use Serial for now)
#ifndef HMI_SERIAL
  #define HMI_SERIAL Serial
#endif
#ifndef DEBUG_SERIAL
  #define DEBUG_SERIAL Serial
#endif

// Reset input
constexpr uint8_t PIN_RESET = 34;

// Flow inputs
constexpr uint8_t PIN_FLOW_AHU   = 21;
constexpr uint8_t PIN_FLOW_EVAP  = 20;
constexpr uint8_t PIN_FLOW_TOWER = 19;

// EEV stepper pins (adjust to real wiring)
constexpr uint8_t PIN_EEV_EVAP_STEP = 2;
constexpr uint8_t PIN_EEV_EVAP_DIR  = 3;
constexpr uint8_t PIN_EEV_EVAP_EN   = 4;
constexpr uint8_t PIN_EEV_EVAP_SLP  = 5;

constexpr uint8_t PIN_EEV_ECON_STEP = 6;
constexpr uint8_t PIN_EEV_ECON_DIR  = 7;
constexpr uint8_t PIN_EEV_ECON_EN   = 8;
constexpr uint8_t PIN_EEV_ECON_SLP  = 9;

// Analog Output channel indices
constexpr uint8_t AO_TOWER_FAN   = 0;
constexpr uint8_t AO_AHU_PUMP    = 1;
constexpr uint8_t AO_EVAP_PUMP   = 2;

// Timing
constexpr unsigned long START_STAGE_DELAY_MS = 30UL * 1000UL;
