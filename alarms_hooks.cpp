
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include "alarms.h"
#include "signals_bridge.h"

// Helper: case-sensitive compare
static inline bool eq(const char* a, const char* b){ return strcmp(a,b)==0; }

float read_signal(const char* name){
  if (!name) return NAN;

  // Pressures / temps / calcs
  if (eq(name,"DischargePressure"))   return read_DischargePressure();
  if (eq(name,"SuctionPressure"))     return read_SuctionPressure();
  if (eq(name,"LiquidLinePressure"))  return read_LiquidLinePressure();
  if (eq(name,"Econ2PhiPressure"))    return read_Econ2PhiPressure();

  if (eq(name,"SuctionSuperheat"))    return read_SuctionSuperheat();
  if (eq(name,"DischargeSuperheat"))  return read_DischargeSuperheat();
  if (eq(name,"Subcooling"))          return read_Subcooling();

  if (eq(name,"HeadPressure"))        return read_HeadPressure();
  if (eq(name,"CondDeltaT"))          return read_CondDeltaT();
  if (eq(name,"EvapDeltaT"))          return read_EvapDeltaT();

  if (eq(name,"CondLWT"))             return read_CondLWT();
  if (eq(name,"ChwLWT"))              return read_ChwLWT();
  if (eq(name,"SatSuctionTemp"))      return read_SuctionTemp();
  if (eq(name,"SatDischargeTemp"))    return read_DischargeTemp();

  // Flowmeters (if not available, fallback to proof as 0/1)
  if (eq(name,"EvapFlowmeter"))       return NAN; // no explicit meter in bridge
  if (eq(name,"CondFlowmeter"))       return NAN; // no explicit meter in bridge

  // Booleans as 0/1 floats
  if (eq(name,"EvapFlowProof"))       return read_EvapFlowProof() ? 1.0f : 0.0f;
  if (eq(name,"CondFlowProof"))       return read_CondFlowProof() ? 1.0f : 0.0f;
  if (eq(name,"TowerLevelOK"))        return read_TowerLevelOK() ? 1.0f : 0.0f;
  if (eq(name,"PhaseOK"))             return read_PhaseOK() ? 1.0f : 0.0f;
  if (eq(name,"CommsOK"))             return read_CommsOK() ? 1.0f : 0.0f;
  if (eq(name,"VFDFault"))            return read_VFDFault() ? 1.0f : 0.0f;

  // Not in bridge yet: return NAN so comparisons fail safely
  if (eq(name,"MotorCurrent"))        return NAN;
  if (eq(name,"MotorCurrentImbalance")) return NAN;
  if (eq(name,"VoltageImbalance"))    return NAN;
  if (eq(name,"SensorValidity"))      return NAN;
  if (eq(name,"PumpCmdFbMismatch"))   return NAN;
  if (eq(name,"EEVHealthy"))          return NAN;
  if (eq(name,"FreezeTrip"))          return NAN;
  if (eq(name,"LowOATLock"))          return NAN;
  if (eq(name,"StartRateExceeded"))   return NAN;

  // Unknown
  return NAN;
}

void log_event(const char* code, const char* msg){
  // Minimal implementation: print to Serial for now
  Serial.print("[EVENT] ");
  if (code) Serial.print(code);
  else Serial.print("(null)");
  Serial.print(": ");
  if (msg) Serial.println(msg);
  else Serial.println("(null)");
}

void on_alarm_trip(const AlarmDef* def){
  // Hook for critical actions (e.g., stop compressor). For now just log.
  Serial.print("[ALARM TRIP] ");
  Serial.println(def ? def->code : "(null)");
  // TODO: integrate with HMI events / outputs if desired
}

void on_warning(const AlarmDef* def){
  Serial.print("[WARNING] ");
  Serial.println(def ? def->code : "(null)");
  // TODO: push to HMI warnings list
}
