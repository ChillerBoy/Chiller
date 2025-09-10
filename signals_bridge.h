
#pragma once
// signals_bridge.h â€” unify firmware variables to named signals for BAS/Alarms/HMI
// Generated: 2025-09-01

#include <stdint.h>

// Initialize any buses (I2C), verify MCP4728 presence, etc.
bool signals_init();

// Core reads (return NAN if unknown)
// --------- Pressures (psi)
float read_DischargePressure();
float read_LiquidLinePressure();
float read_Econ2PhiPressure();
float read_SuctionPressure();

// --------- Water pressures (psi)
float read_EvapSuctionP();
float read_EvapDischargeP();
float read_AHUSuctionP();
float read_AHUDischargeP();
float read_CondPumpDischargeP();

// --------- Water temps (degF)
float read_CondEWT();
float read_CondLWT();
float read_ChwEWT();
float read_ChwLWT();
float read_AHUSupplyT();
float read_AHUReturnT();

// --------- Refrigerant temps (degF)
float read_DischargeTemp();
float read_LiquidLineTemp();
float read_Econ2PhiTemp();
float read_EconSuctionTemp();
float read_LiquidPostEconTemp();
float read_LiquidPostFilterTemp();
float read_Evap2PhiTemp();
float read_SuctionTemp();

// --------- Air temps (degF)
float read_AHU1RetT();
float read_AHU1SupT();
float read_AHU2RetT();
float read_AHU2SupT();
float read_AHU3RetT();
float read_AHU3SupT();
float read_TowerEnterAirT();
float read_TowerLeaveAirT();

// --------- Humidity (%RH)
float read_AHU1RetRH();
float read_AHU1SupRH();
float read_AHU2RetRH();
float read_AHU2SupRH();
float read_AHU3RetRH();
float read_AHU3SupRH();
float read_TowerEnterRH();
float read_TowerLeaveRH();

// --------- Derived
float read_SuctionSuperheat();
float read_DischargeSuperheat();
float read_Subcooling();
float read_CondDeltaT();
float read_EvapDeltaT();
float read_HeadPressure();

// --------- Status / flags
bool read_VFDFault();
bool read_CommsOK();
bool read_TowerLevelOK();
bool read_EvapFlowProof();
bool read_CondFlowProof();
bool read_PhaseOK();

// --------- Commands (write-throughs)
void write_Cmd_CompressorHz(float hz);
void write_Cmd_AHUPumpV(float v);
void write_Cmd_EvapPumpV(float v);
void write_Cmd_CondPumpRun(bool on);
void write_Cmd_SandFilterRun(bool on);
void write_Cmd_TowerFanV(float v);
void write_Cmd_EEVPercent(float pct);

// --------- Setpoints
void write_SP_LCHWS(float f);
void write_SP_Superheat(float f);
void write_SP_HeadTarget(float psi);
float read_SP_LCHWS();
float read_SP_Superheat();
float read_SP_HeadTarget();

// --------- Modes
enum class SysMode : uint8_t { OFF=0, AUTO=1, MANUAL=2, TEST=3 };
void write_Mode_System(SysMode m);
SysMode read_Mode_System();

// --------- Runtimes (hours)
float read_Runtime_Compressor_h();
float read_Runtime_AHUPump_h();
float read_Runtime_EvapPump_h();
float read_Runtime_CondPump_h();
float read_Runtime_TowerFan_h();
