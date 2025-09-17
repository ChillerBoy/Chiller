/*
  JSONBridge.h (v2)
  Compatibility helpers for Arduino JSON telemetry.

  - Provides legacy-style C APIs used in your sketch:
      JSONBridge_begin(Stream*)
      JSONBridge_setInterval(uint32_t ms)
      JSONBridge_tick()

      JSONBridge_setUnit(bool enabled, const char* step)
      JSONBridge_setCHW(float supplyF, float setpointF)
      JSONBridge_setTemps(float a, float b, float c, float d)                   // 4 temps
      JSONBridge_setTempsAll(const float* temps, size_t n)                      // array form
      JSONBridge_setPress(float evapSuc, float evapDis, float ahuSuc, float ahuDis)
      JSONBridge_setFlow(float evapGpm, float ahuGpm)
      JSONBridge_setProof(bool evapFlowProven, bool ahuFlowProven)
      JSONBridge_setWarnAlarm(bool warnActive, bool alarmActive)
      JSONBridge_setCommands(bool evapPumpOn, bool ahuPumpOn, int evCmdPct, int ahCmdPct)

  - Also offers a modern namespaced API (JSONBridge::) if you prefer later.
*/

#ifndef JSONBRIDGE_H
#define JSONBRIDGE_H

#include <Arduino.h>

namespace JSONBridge {

  struct Frame {
    // Summary
    bool   unitEnabled = false;
    String step;              // textual step/state

    // CHW
    float  chwSupplyF = NAN;
    float  chwSPF     = NAN;

    // Temps
    float  evapEWT_F  = NAN;
    float  evapLWT_F  = NAN;
    float  ahuSupplyF = NAN;
    float  ahuReturnF = NAN;

    // Pressures
    float  evapSucPsi = NAN;
    float  evapDisPsi = NAN;
    float  ahuSucPsi  = NAN;
    float  ahuDisPsi  = NAN;

    // Flows
    float  evapGpm    = NAN;
    float  ahuGpm     = NAN;

    // Proofs
    bool   evapFlowProven = false;
    bool   ahuFlowProven  = false;

    // States
    bool   warningActive  = false;
    bool   alarmActive    = false;

    // Commands
    bool   evapPumpOn     = false;
    bool   ahuPumpOn      = false;
    int    evCmdPct       = 0;   // 0..100
    int    ahCmdPct       = 0;   // 0..100
  };

  // Configure the bridge
  void begin(Stream* out);
  void setInterval(uint32_t ms);
  void setFloatDigits(uint8_t digits);

  // Call regularly; prints when interval elapses
  void tick();

  // Setters to populate the current frame
  void setUnit(bool enabled, const char* step);
  void setCHW(float supplyF, float setpointF);
  void setTemps4(float a, float b, float c, float d);
  void setTempsAll(const float* temps, size_t n); // fills T1..Tn under Temp
  void setPress(float evapSuc, float evapDis, float ahuSuc, float ahuDis);
  void setFlow(float evapGpm, float ahuGpm);
  void setProof(bool evapFlowProven, bool ahuFlowProven);
  void setWarnAlarm(bool warnActive, bool alarmActive);
  void setCommands(bool evapPumpOn, bool ahuPumpOn, int evCmdPct, int ahCmdPct);

  // Force immediate publish (bypass interval)
  void publishNow();

} // namespace JSONBridge

// ---------------- Legacy C-style wrappers (match your sketch) ----------------
inline void JSONBridge_begin(Stream* s)                   { JSONBridge::begin(s); }
inline void JSONBridge_setInterval(uint32_t ms)           { JSONBridge::setInterval(ms); }
inline void JSONBridge_tick()                             { JSONBridge::tick(); }

inline void JSONBridge_setUnit(bool e, const char* step)  { JSONBridge::setUnit(e, step); }
inline void JSONBridge_setCHW(float supF, float spF)      { JSONBridge::setCHW(supF, spF); }

// 4 temps convenience
inline void JSONBridge_setTemps(float a, float b, float c, float d) { JSONBridge::setTemps4(a,b,c,d); }
// array form (legacy name already provided earlier)
inline void JSONBridge_setTempsAll(const float* temps, size_t n)    { JSONBridge::setTempsAll(temps, n); }

inline void JSONBridge_setPress(float es, float ed, float as, float ad) { JSONBridge::setPress(es,ed,as,ad); }
inline void JSONBridge_setFlow(float eg, float ag)                      { JSONBridge::setFlow(eg,ag); }
inline void JSONBridge_setProof(bool ep, bool ap)                       { JSONBridge::setProof(ep,ap); }
inline void JSONBridge_setWarnAlarm(bool w, bool a)                     { JSONBridge::setWarnAlarm(w,a); }
inline void JSONBridge_setCommands(bool evOn, bool ahOn, int evPct, int ahPct) { JSONBridge::setCommands(evOn, ahOn, evPct, ahPct); }

#endif // JSONBRIDGE_H
