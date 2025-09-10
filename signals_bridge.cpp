
// signals_bridge.cpp â€” connect firmware to named signals
#include "signals_bridge.h"
#include <math.h>

// NOTE: Wire these to your existing modules/variables.
// Example placeholders return NAN or do nothing.

bool signals_init(){
  // TODO: init I2C, confirm MCP4728 present, set default DAC channels, etc.
  return true;
}

// Helpers
static inline float na(){ return NAN; }
static inline bool fb(){ return false; }

// Pressures
float read_DischargePressure(){ return na(); }
float read_LiquidLinePressure(){ return na(); }
float read_Econ2PhiPressure(){ return na(); }
float read_SuctionPressure(){ return na(); }

// Water pressures
float read_EvapSuctionP(){ return na(); }
float read_EvapDischargeP(){ return na(); }
float read_AHUSuctionP(){ return na(); }
float read_AHUDischargeP(){ return na(); }
float read_CondPumpDischargeP(){ return na(); }

// Water temps
float read_CondEWT(){ return na(); }
float read_CondLWT(){ return na(); }
float read_ChwEWT(){ return na(); }
float read_ChwLWT(){ return na(); }
float read_AHUSupplyT(){ return na(); }
float read_AHUReturnT(){ return na(); }

// Refrigerant temps
float read_DischargeTemp(){ return na(); }
float read_LiquidLineTemp(){ return na(); }
float read_Econ2PhiTemp(){ return na(); }
float read_EconSuctionTemp(){ return na(); }
float read_LiquidPostEconTemp(){ return na(); }
float read_LiquidPostFilterTemp(){ return na(); }
float read_Evap2PhiTemp(){ return na(); }
float read_SuctionTemp(){ return na(); }

// Air temps
float read_AHU1RetT(){ return na(); }
float read_AHU1SupT(){ return na(); }
float read_AHU2RetT(){ return na(); }
float read_AHU2SupT(){ return na(); }
float read_AHU3RetT(){ return na(); }
float read_AHU3SupT(){ return na(); }
float read_TowerEnterAirT(){ return na(); }
float read_TowerLeaveAirT(){ return na(); }

// Humidity
float read_AHU1RetRH(){ return na(); }
float read_AHU1SupRH(){ return na(); }
float read_AHU2RetRH(){ return na(); }
float read_AHU2SupRH(){ return na(); }
float read_AHU3RetRH(){ return na(); }
float read_AHU3SupRH(){ return na(); }
float read_TowerEnterRH(){ return na(); }
float read_TowerLeaveRH(){ return na(); }

// Derived
float read_SuctionSuperheat(){ return na(); }
float read_DischargeSuperheat(){ return na(); }
float read_Subcooling(){ return na(); }
float read_CondDeltaT(){ return na(); }
float read_EvapDeltaT(){ return na(); }
float read_HeadPressure(){ return na(); }

// Status / flags
bool read_VFDFault(){ return fb(); }
bool read_CommsOK(){ return true; }
bool read_TowerLevelOK(){ return fb(); }
bool read_EvapFlowProof(){ return fb(); }
bool read_CondFlowProof(){ return fb(); }
bool read_PhaseOK(){ return true; }

// Commands
void write_Cmd_CompressorHz(float hz){ (void)hz; /* TODO: VFD write */ }
void write_Cmd_AHUPumpV(float v){ (void)v; /* TODO: DAC chX */ }
void write_Cmd_EvapPumpV(float v){ (void)v; /* TODO: DAC chY */ }
void write_Cmd_CondPumpRun(bool on){ (void)on; /* TODO: DO */ }
void write_Cmd_SandFilterRun(bool on){ (void)on; /* TODO: DO */ }
void write_Cmd_TowerFanV(float v){ (void)v; /* TODO: DAC chZ */ }
void write_Cmd_EEVPercent(float pct){ (void)pct; /* TODO: stepper/driver */ }

// Setpoints
static float sp_lchws_f = 46.0f;
static float sp_sh_f = 10.0f;
static float sp_head_psi = 220.0f;

void write_SP_LCHWS(float f){ sp_lchws_f = f; }
void write_SP_Superheat(float f){ sp_sh_f = f; }
void write_SP_HeadTarget(float psi){ sp_head_psi = psi; }
float read_SP_LCHWS(){ return sp_lchws_f; }
float read_SP_Superheat(){ return sp_sh_f; }
float read_SP_HeadTarget(){ return sp_head_psi; }

// Modes
static SysMode g_mode = SysMode::OFF;
void write_Mode_System(SysMode m){ g_mode = m; }
SysMode read_Mode_System(){ return g_mode; }

// Runtimes
float read_Runtime_Compressor_h(){ return na(); }
float read_Runtime_AHUPump_h(){ return na(); }
float read_Runtime_EvapPump_h(){ return na(); }
float read_Runtime_CondPump_h(){ return na(); }
float read_Runtime_TowerFan_h(){ return na(); }
