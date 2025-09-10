
#pragma once
// io_pins.h — I/O map derived from user docs

// -------- Analog Inputs (Arduino Mega A0..A15) --------
// NOTE: '8' was listed twice (Evap Pump Discharge Pressure and Evap GPM).
// Provisional choice: keep A8 = Evap Pump Discharge Pressure; map Evap GPM to A9 (TODO verify).
#define AI_AHU_PUMP_SUCTION_P         A0
#define AI_AHU_PUMP_DISCH_P           A4
#define AI_EVAP_PUMP_DISCH_P          A8
#define AI_EVAP_PUMP_SUCTION_P        A12
#define AI_CTOWER_DISCH_P             A1
#define AI_AHU_GPM                    A5
#define AI_EVAP_GPM                   A9   // <-- provisional
#define AI_COND_GPM                   A13
#define AI_COMP_DISCH_PRESS           A2
#define AI_LIQUID_LINE_PRESS          A6
#define AI_TWO_PHASE_PRESS            A10
#define AI_SUCTION_PRESS              A14

// -------- Digital Inputs --------
#define DI_ALARM_SILENCE              33
#define DI_ENABLE_OFF                 27
#define DI_TOWER_FLOW_SWITCH          36
#define DI_AHU_FLOW_SWITCH            38
#define DI_EVAP_FLOW_SWITCH           40
#define DI_RIGHT_FAN_CONTACTOR_PROOF  42
#define DI_AHU_PUMP_CONTACTOR_PROOF   44
#define DI_EVAP_PUMP_CONTACTOR_PROOF  46
#define DI_LOWER_FAN_CONTACTOR_PROOF  48
#define DI_LEFT_FAN_CONTACTOR_PROOF   50
#define DI_CTOWER_PUMP_CONTACTOR_PROOF 52

// -------- Digital Outputs --------
#define DO_AHU_PUMP                   22
#define DO_EVAP_PUMP                  23
#define DO_SAND_FILTER_PUMP           24
#define DO_CTOWER_PUMP                25
#define DO_LOWER_FAN_CONTACTOR        28
#define DO_LEFT_FAN_CONTACTOR         29
#define DO_RIGHT_FAN_CONTACTOR        30
#define DO_CTOWER_FAN_ENABLE          31
#define DO_RED_LIGHT                  47
#define DO_ORANGE_LIGHT               49
#define DO_GREEN_LIGHT                51
#define DO_ALARM_BELL                 53

// -------- OneWire / DS18B20 --------
#define ONEWIRE_PIN                   2  // OneWire bus
