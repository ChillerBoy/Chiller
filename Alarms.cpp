
#include "alarms.h"
#include <string.h>

static ActiveAlarm g_active[64];
static uint16_t g_count = 0;

#define AL(code,name,type,prio,latched,autoClear,deb,clr,minon,source,op,thr) \
  {code,name,type,prio,latched,autoClear,deb,clr,minon,source,op,thr}

const AlarmDef ALARM_TABLE[] = {
  // ---------------- WARNINGS ----------------
  AL("LOW_SUCTION_PRESSURE","Low Suction Pressure",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"SuctionPressure","lt",110),
  AL("HIGH_DISCHARGE_PRESSURE_WARN","High Discharge Pressure (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"DischargePressure","gt",350),
  AL("LOW_SUPERHEAT","Low Suction Superheat (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"SuctionSuperheat","lt",4),
  AL("HIGH_SUPERHEAT","High Suction Superheat (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"SuctionSuperheat","gt",18),
  AL("LOW_SUBCOOLING","Low Subcooling (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"Subcooling","lt",5),
  AL("HIGH_SUBCOOLING","High Subcooling (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"Subcooling","gt",25),
  AL("LOW_EVAP_FLOW","Low Evaporator Flow (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"EvapFlowmeter","lt",0.5),
  AL("LOW_COND_FLOW","Low Condenser Flow (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"CondFlowmeter","lt",0.5),
  AL("CURRENT_IMBALANCE","Motor Current Imbalance (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"MotorCurrentImbalance","gt",10),
  AL("VOLTAGE_IMBALANCE","Line Voltage Imbalance (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"VoltageImbalance","gt",5),
  AL("LOW_SAT_SUCTION_TEMP","Low Sat Suction Temp (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"SatSuctionTemp","lt",20),
  AL("HIGH_SAT_SUCTION_TEMP","High Sat Suction Temp (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"SatSuctionTemp","gt",55),
  AL("LOW_SAT_DISCH_TEMP","Low Sat Discharge Temp (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"SatDischargeTemp","lt",80),
  AL("HIGH_SAT_DISCH_TEMP","High Sat Discharge Temp (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"SatDischargeTemp","gt",140),
  AL("LOW_COND_DELTA_T","Low Condenser Î”T (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"CondDeltaT","lt",3),
  AL("HIGH_COND_DELTA_T","High Condenser Î”T (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"CondDeltaT","gt",25),
  AL("LOW_EVAP_DELTA_T","Low Evaporator Î”T (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"EvapDeltaT","lt",1),
  AL("HIGH_EVAP_DELTA_T","High Evaporator Î”T (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"EvapDeltaT","gt",15),
  AL("LOW_HIGH_COND_LWT_EWT","Condenser EWT/LWT out of bounds (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"CondLWT","band",0),
  AL("LOW_HIGH_EVAP_LWT_EWT","Evap EWT/LWT out of bounds (warn)",AlarmType::WARNING,Priority::MEDIUM,false,true,3,2,0,"ChwLWT","band",0),

  // ---------------- ALARMS ----------------
  AL("HIGH_DISCHARGE_TEMP","High Discharge Temperature",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"DischargeTemp","gt",230),
  AL("HIGH_DISCHARGE_PRESSURE","High Discharge Pressure (trip)",AlarmType::ALARM,Priority::CRITICAL,true,false,2,0,30,"DischargePressure","gt",435),
  AL("LOW_SUCTION_PRESSURE","Low Suction Pressure (trip)",AlarmType::ALARM,Priority::CRITICAL,true,false,2,0,30,"SuctionPressure","lt",90),
  AL("LOW_SUCTION_SUPERHEAT","Low Suction Superheat (trip)",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"SuctionSuperheat","lt",2),
  AL("HIGH_SUCTION_SUPERHEAT","High Suction Superheat (trip)",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"SuctionSuperheat","gt",25),
  AL("LOW_DISCHARGE_SUPERHEAT","Low Discharge Superheat (trip)",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"DischargeSuperheat","lt",10),
  AL("HIGH_DISCHARGE_SUPERHEAT","High Discharge Superheat (trip)",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"DischargeSuperheat","gt",70),
  AL("LOW_SUBCOOLING","Low Subcooling (trip)",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"Subcooling","lt",2),
  AL("HIGH_SUBCOOLING","High Subcooling (trip)",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"Subcooling","gt",35),
  AL("VFD_ALARM","VFD Alarm",AlarmType::ALARM,Priority::CRITICAL,true,false,1,0,5,"VFDFault","eq",1),
  AL("LOSS_OF_EVAP_FLOW","Loss of Evaporator Flow",AlarmType::ALARM,Priority::CRITICAL,true,false,3,0,10,"EvapFlowProof","eq",0),
  AL("LOSS_OF_COND_FLOW","Loss of Condenser Flow",AlarmType::ALARM,Priority::CRITICAL,true,false,3,0,10,"CondFlowProof","eq",0),
  AL("WATER_LEVEL_LOW","Tower Basin Water Level Low",AlarmType::ALARM,Priority::CRITICAL,true,false,2,0,30,"TowerLevelOK","eq",0),
  AL("LOW_MOTOR_CURRENT","Low Motor Current",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"MotorCurrent","lt",0.5),
  AL("HIGH_MOTOR_CURRENT","High Motor Current",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"MotorCurrent","gt",80),
  AL("ANY_SENSOR_FAILED","Sensor Failed",AlarmType::ALARM,Priority::HIGH,true,false,2,0,10,"SensorValidity","eq",0),
  AL("LOSS_OF_PHASE","Loss of Phase",AlarmType::ALARM,Priority::CRITICAL,true,false,1,0,10,"PhaseOK","eq",0),
  AL("PUMP_RUNNING_WHEN_UNIT_OFF","Pump running while unit Off",AlarmType::ALARM,Priority::MEDIUM,true,false,5,0,10,"PumpCmdFbMismatch","eq",1),
  AL("LOSS_OF_COMM","Loss of Communication",AlarmType::ALARM,Priority::CRITICAL,true,false,2,0,10,"CommsOK","eq",0),
  AL("EEV_FAILED","Expansion Valve Failed",AlarmType::ALARM,Priority::HIGH,true,false,5,0,10,"EEVHealthy","eq",0),
  AL("FREEZE_PROTECTION","Freeze Trip",AlarmType::ALARM,Priority::CRITICAL,true,false,1,0,30,"FreezeTrip","eq",1),
  AL("LOW_OAT_LOCKOUT","Low OAT Lockout",AlarmType::ALARM,Priority::MEDIUM,true,false,1,0,30,"LowOATLock","eq",1),
  AL("MAX_STARTS_EXCEEDED","Max Compressor Starts Exceeded",AlarmType::ALARM,Priority::MEDIUM,true,false,0,0,0,"StartRateExceeded","eq",1),
};

const uint16_t ALARM_TABLE_SIZE = sizeof(ALARM_TABLE)/sizeof(ALARM_TABLE[0]);

static bool compare(const AlarmDef& def, float value){
  if (strcmp(def.op,"gt")==0) return value > def.threshold;
  if (strcmp(def.op,"lt")==0) return value < def.threshold;
  if (strcmp(def.op,"eq")==0) return value == def.threshold;
  if (strcmp(def.op,"band")==0) {
    // Band checks should be implemented against runtime setpoints;
    // treat as false here and rely on read_signal() to precompute if desired.
    return false;
  }
  return false;
}

void alarms_init(){
  memset(g_active, 0, sizeof(g_active));
  g_count = 0;
}

void eval_one(uint32_t now_ms, const AlarmDef* def){
  float v = read_signal(def->source);
  bool trig = compare(*def, v);
  // allocate slot if needed
  ActiveAlarm* slot = nullptr;
  for(uint16_t i=0;i<g_count;i++){
    if (g_active[i].def==def){ slot = &g_active[i]; break; }
  }
  if (!slot && g_count < (sizeof(g_active)/sizeof(g_active[0]))){
    slot = &g_active[g_count++];
    memset(slot,0,sizeof(*slot));
    slot->def = def;
  }
  if (!slot) return;

  if (trig){
    if (!slot->active){
      if (slot->first_ts_ms==0) slot->first_ts_ms = now_ms;
      if (now_ms - slot->first_ts_ms >= def->debounce_s*1000UL){
        slot->active = true;
        slot->last_ts_ms = now_ms;
        log_event(def->code, "ALARM_ON");
        if (def->type==AlarmType::ALARM) on_alarm_trip(def); else on_warning(def);
      }
    } else {
      slot->last_ts_ms = now_ms;
    }
  } else {
    slot->first_ts_ms = 0;
    if (slot->active && def->autoClear){
      // auto clear warnings after clear delay
      static uint32_t clear_start = 0;
      if (clear_start==0) clear_start = now_ms;
      if (now_ms - clear_start >= def->clear_s*1000UL){
        slot->active = false;
        slot->acknowledged = true;
        log_event(def->code, "ALARM_OFF");
        clear_start = 0;
      }
    }
  }
}

void alarms_tick(uint32_t now_ms){
  for (uint16_t i=0;i<ALARM_TABLE_SIZE;i++){
    eval_one(now_ms, &ALARM_TABLE[i]);
  }
}

void alarms_ack_all(){
  for (uint16_t i=0;i<g_count;i++){
    g_active[i].acknowledged = true;
  }
}

bool alarms_any_active(){
  for (uint16_t i=0;i<g_count;i++) if (g_active[i].active) return true;
  return false;
}

bool alarms_any_trip(){
  for (uint16_t i=0;i<g_count;i++){
    if (g_active[i].active && g_active[i].def->type==AlarmType::ALARM) return true;
  }
  return false;
}

uint16_t alarms_active_count(){ return g_count; }

const ActiveAlarm* alarms_get(uint16_t i){ return (i<g_count) ? &g_active[i] : nullptr; }
