/*
  Chiller_Main_Code.ino  (Monolithic sketch)
  Board: Arduino Mega 2560
  Baud: 115200
  Output: newline-delimited JSON (one object per line)

  DESIGN INTENT
  -------------
  - Everything "live" runs from THIS file.
  - Other sketches in /arduino/reference are reference-only and are NOT compiled.
  - DS18B20_Sensors.* and TempSensorIDs.h are required helpers.

  PATCHING (search these tokens in this file)
  ------------------------------------------
  PATCH_POINT_PINS        -> pin map for pumps/relays, flow inputs, etc.
  PATCH_POINT_PRESSURE    -> analog pin map + ranges for pressure sensors
  PATCH_POINT_FLOW        -> flow meter calibration (K-factor) + interrupt pins
  PATCH_POINT_VFD         -> how VFD run/fault/ready are wired + speed output
  PATCH_POINT_EEV         -> EEV homing feedback or stub
  PATCH_POINT_LIMITS      -> default limits for pressures/temps/flows
  PATCH_POINT_JSON_KEYS   -> JSON key naming (if you want different keys)

  SERIAL COMMANDS (simple baseline)
  --------------------------------
  - Setpoint:   SP LWT 44.0
  - Deadband:   SP DB  2.0
  - Training:   SIM ON / SIM OFF
  - Fault:      SIM FAULT <name>
      names: EVAP_FLOW_FAIL, EVAP_FLOW_LOST, COND_FLOW_FAIL, COND_FLOW_LOST,
             HI_DISCH_P, LO_SUCTION_P, VFD_FAULT_START, SENSOR_STUCK_LWT

  NOTE: This is a baseline reference implementation. Tune later.
*/

#include <Arduino.h>

#define ONEWIRE_PIN 2
#include "DS18B20_Sensors.h"

// -----------------------------
// PATCH_POINT_PINS
// -----------------------------
static const uint8_t PIN_EVAP_PUMP_RELAY = 45;   // active-low outputs are supported below
static const uint8_t PIN_COND_PUMP_RELAY = 39;   // adjust to your actual mapping
static const uint8_t PIN_CT_PUMP_RELAY   = 43;   // if separate, change
static const uint8_t PIN_FILTER_PUMP_RELAY = 41; // optional
static const uint8_t PIN_VFD_RUN_RELAY   = 53;   // optional digital run enable
static const uint8_t PIN_ALARM_BELL      = 27;   // optional

// Flow meter input pins (must be interrupt-capable or use polling/PCINT later)
// Mega external interrupts: 2, 3, 18, 19, 20, 21
static const uint8_t PIN_FLOW_AHU  = 3;
static const uint8_t PIN_FLOW_EVAP = 18;
static const uint8_t PIN_FLOW_COND = 19;

// Optional VFD status inputs (wire as needed)
static const uint8_t PIN_VFD_READY = 33;  // BI_VFD_Ready (HIGH = ready) - change as needed
static const uint8_t PIN_VFD_FAULT = 34;  // BI_VFD_Fault (HIGH = fault) - change as needed
static const uint8_t PIN_VFD_RUNFB = 35;  // BI_VFD_Run   (HIGH = running) - change as needed

// Active-low relay modules (TRUE if your relay board turns ON when pin LOW)
static const bool RELAY_ACTIVE_LOW = true;

// -----------------------------
// PATCH_POINT_PRESSURE
// Analog pressure sensors: 0.5–4.5V -> 0..Range
// -----------------------------
struct PressureChan {
  const char* key;
  uint8_t pin;
  float rangePsig;  // engineering range at 4.5V
};

static const PressureChan PRESS_CHANS[] = {
  {"P_AHU_PUMP_SUCTION",  A0, 80.0f},
  {"P_AHU_PUMP_DISCH",    A4, 80.0f},
  {"P_EVAP_PUMP_DISCH",   A8, 80.0f},
  {"P_EVAP_PUMP_SUCTION", A12, 80.0f},
  {"P_CTOWER_DISCH",      A1, 80.0f},
  {"P_COMP_DISCH",        A2, 435.0f},
  {"P_LIQUID_LINE",       A6, 435.0f},
  {"P_TWO_PHASE",         A10, 435.0f},
  {"P_SUCTION",           A14, 435.0f},
};
static const uint8_t NUM_PRESS = sizeof(PRESS_CHANS)/sizeof(PRESS_CHANS[0]);

// -----------------------------
// PATCH_POINT_FLOW
// Flow meters: FL-608 (pulse output)
// Provide pulses-per-gallon (or equivalent K-factor). Start with placeholder.
// -----------------------------
static const float FLOW_PULSES_PER_GAL = 450.0f; // TODO: set true K-factor for your sensors
static const uint32_t FLOW_SAMPLE_MS   = 1000;   // compute GPM every 1s

// -----------------------------
// PATCH_POINT_LIMITS
// Default thresholds and timers (tune later)
// -----------------------------
static float SP_LWT_F     = 44.0f;
static float SP_LWT_DB_F  = 2.0f;

static float EVAP_MIN_GPM = 5.0f;   // TODO: determine
static float COND_MIN_GPM = 5.0f;   // TODO: determine

static const uint32_t FLOW_PROVE_MS = 5000;
static const uint32_t FLOW_LOSS_MS  = 3000;

static const uint32_t STABILIZE_MS  = 15000;
static const uint32_t MIN_ON_MS     = 120000;
static const uint32_t MIN_OFF_MS    = 120000;
static const uint32_t POSTRUN_MS    = 60000;

static float LIM_DISCH_P_HIGH_TRIP = 350.0f;  // placeholder
static float LIM_SUCTION_P_LOW_TRIP = 10.0f;  // placeholder

// -----------------------------
// PATCH_POINT_VFD
// Speed command output is represented as AO_CompSpeedCmd (%) in JSON.
// Actual DAC/PWM output wiring is project-specific and is left as a stub.
// -----------------------------
static const float LIM_COMP_MIN_SPEED_PCT = 20.0f;
static const float LIM_COMP_MAX_SPEED_PCT = 100.0f;
static const float LIM_COMP_RAMP_UP_PCT_PER_SEC = 5.0f;
static const float LIM_COMP_RAMP_DOWN_PCT_PER_SEC = 8.0f;

// -----------------------------
// PATCH_POINT_EEV
// For now, treat EEV homing as always OK unless you wire feedback.
// -----------------------------
static bool BI_EEV_EvapHomed = true;
static bool BI_EEV_EconHomed = true;

// -----------------------------
// Internal state
// -----------------------------
enum Mode : uint8_t {
  MODE_OFF=0,
  MODE_IDLE_READY,
  MODE_PRECHECK,
  MODE_FLOW_PROVE,
  MODE_EEV_HOME,
  MODE_START_VFD,
  MODE_RUN_STABILIZE,
  MODE_RUN,
  MODE_UNLOAD,
  MODE_SOFT_STOP,
  MODE_FAULT,
  MODE_LOCKOUT
};

static Mode mode = MODE_OFF;
static uint32_t modeEnterMs = 0;
static uint32_t lastStartMs = 0;
static uint32_t lastStopMs = 0;

// Alarm flags (BAS-style)
static bool ALM_EvapFlowFailToProve=false;
static bool ALM_EvapFlowLost=false;
static bool ALM_CondFlowFailToProve=false;
static bool ALM_CondFlowLost=false;
static bool ALM_HiDischP=false;
static bool ALM_LoSuctionP=false;
static bool ALM_VFDFault=false;
static bool ALM_SensorFault=false;
static bool ALM_TrainingMode=false;

// Event log (simple ring buffer)
static const uint8_t EVT_DEPTH = 20;
static char EVT_TEXT[EVT_DEPTH][48];
static uint8_t EVT_HEAD=0;

static void logEvent(const char* msg) {
  strncpy(EVT_TEXT[EVT_HEAD], msg, sizeof(EVT_TEXT[EVT_HEAD])-1);
  EVT_TEXT[EVT_HEAD][sizeof(EVT_TEXT[EVT_HEAD])-1]=0;
  EVT_HEAD = (uint8_t)((EVT_HEAD + 1) % EVT_DEPTH);
}

static const char* modeName(Mode m) {
  switch(m){
    case MODE_OFF: return "OFF";
    case MODE_IDLE_READY: return "IDLE_READY";
    case MODE_PRECHECK: return "PRECHECK";
    case MODE_FLOW_PROVE: return "FLOW_PROVE";
    case MODE_EEV_HOME: return "EEV_HOME";
    case MODE_START_VFD: return "START_VFD";
    case MODE_RUN_STABILIZE: return "RUN_STABILIZE";
    case MODE_RUN: return "RUN";
    case MODE_UNLOAD: return "UNLOAD";
    case MODE_SOFT_STOP: return "SOFT_STOP";
    case MODE_FAULT: return "FAULT";
    case MODE_LOCKOUT: return "LOCKOUT";
    default: return "UNK";
  }
}

// -----------------------------
// Sensors
// -----------------------------
DS18B20Sensors tempSensors(ONEWIRE_PIN);

// temp keys (match your existing HMI mapping)
struct TempKeyMap { TempSensorID id; const char* key; };
static const TempKeyMap TEMP_MAP[] = {
  {COND_EWT, "COND_EWT_F"},
  {ECON_SUCTION_TEMP, "ECON_SUCTION_F"},
  {AHU_SUPPLY_TEMP, "AHU_SUPPLY_F"},
  {EVAP_LWT, "EVAP_LWT_F"},
  {LIQUID_POST_ECON, "LIQ_POST_ECON_F"},
  {COND_LWT, "COND_LWT_F"},
  {AHU_RETURN_TEMP, "AHU_RETURN_F"},
  {SUCTION_TEMP, "SUCTION_F"},
  {EVAP_EWT, "EVAP_EWT_F"},
  {DISCHARGE_TEMP, "DISCHARGE_F"},
  {POST_FILTER_DRIER_TEMP, "POST_FILTER_DRIER_F"},
  {LIQUID_LINE_TEMP, "LIQ_LINE_F"},
  {ECON_2PHASE_TEMP, "ECON_2PHASE_F"},
  {TWO_PHASE_TEMP, "TWO_PHASE_F"},
};

// TEMP_MAP indices (used for calibration offsets)
enum TempIdx : uint8_t {
  IDX_COND_EWT = 0,
  IDX_ECON_SUCTION = 1,
  IDX_AHU_SUPPLY = 2,
  IDX_EVAP_LWT = 3,
  IDX_LIQ_POST_ECON = 4,
  IDX_COND_LWT = 5,
  IDX_AHU_RETURN = 6,
  IDX_SUCTION_T = 7,
  IDX_EVAP_EWT = 8,
  IDX_DISCHARGE_T = 9,
  IDX_POST_FILTER_DRIER = 10,
  IDX_LIQ_LINE_T = 11,
  IDX_ECON_2PHASE_T = 12,
  IDX_TWO_PHASE_T = 13
};

static const uint8_t NUM_TEMP = sizeof(TEMP_MAP)/sizeof(TEMP_MAP[0]);

// Flow pulse counters
volatile uint32_t pulsesAHU=0, pulsesEVAP=0, pulsesCOND=0;

static void isrAhu(){ pulsesAHU++; }
static void isrEvap(){ pulsesEVAP++; }
static void isrCond(){ pulsesCOND++; }

static float F_AHU_GPM=0, F_EVAP_GPM=0, F_COND_GPM=0;
static uint32_t lastFlowCalcMs=0;
static uint32_t lastPulsesAHU=0, lastPulsesEVAP=0, lastPulsesCOND=0;

// Pressure engineering values
static float pressPsig[NUM_PRESS];

// Convenience PVs
static float pvEvapLwtF = NAN;
static float pvDischP = NAN;
static float pvSuctP = NAN;

// Training / simulation
static bool SIM_Enable=false;
static char SIM_Fault[24] = "NONE";

// Service / Manual override framework
static bool STAT_ServiceMode = false;

// Manual flags (when TRUE, output is forced regardless of normal state logic)
static bool MAN_EvapPump = false;
static bool MAN_CondPump = false;
static bool MAN_CTPump   = false;
static bool MAN_AlarmBell = false;
static bool MAN_VFDRun   = false;
static bool MAN_TowerFan = false;

// Manual command values
static float MAN_VFDSpeedPct = 0.0f;     // 0-100
static float MAN_TowerFanPct = 0.0f;     // 0-100
static float MAN_EEV_EvapPct = 0.0f;     // 0-100 (simulated unless tied to driver)
static float MAN_EEV_EconPct = 0.0f;     // 0-100

// Calibration offsets (engineering units)
static float CAL_TempOfsF[NUM_TEMP] = {0};   // °F offsets, TEMP_MAP order
static float CAL_PressOfs[NUM_PRESS] = {0};  // psig offsets, PRESS_CHANS order
static float CAL_FlowOfs[3] = {0,0,0};       // GPM offsets: AHU, EVAP, COND

static float clampPct(float v){ if (v<0) return 0; if (v>100) return 100; return v; }

// -----------------------------
// I/O helpers
// -----------------------------
static void relayWrite(uint8_t pin, bool on) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(pin, on ? LOW : HIGH);
  } else {
    digitalWrite(pin, on ? HIGH : LOW);
  }
}

static bool readBI(uint8_t pin, bool defaultVal=false) {
  if (pin == 255) return defaultVal;
  return digitalRead(pin) == HIGH;
}

// -----------------------------
// Engineering conversions
// -----------------------------
static float analogToPsig(int adc, float rangePsig) {
  // Mega ADC: 0..1023 at 0..5V
  float v = (float)adc * (5.0f/1023.0f);
  // 0.5..4.5V -> 0..range
  if (v < 0.1f) return NAN;
  float pv = (v - 0.5f) / 4.0f;
  if (pv < 0) pv = 0;
  if (pv > 1) pv = 1;
  return pv * rangePsig;
}

static void readPressures() {
  for (uint8_t i=0;i<NUM_PRESS;i++){
    int adc = analogRead(PRESS_CHANS[i].pin);
    pressPsig[i] = analogToPsig(adc, PRESS_CHANS[i].rangePsig);
    pressPsig[i] += CAL_PressOfs[i];
  }
  // Convenience mapping
  for (uint8_t i=0;i<NUM_PRESS;i++){
    if (strcmp(PRESS_CHANS[i].key,"P_COMP_DISCH")==0) pvDischP = pressPsig[i];
    if (strcmp(PRESS_CHANS[i].key,"P_SUCTION")==0) pvSuctP = pressPsig[i];
  }
}

static void computeFlow() {
  uint32_t now = millis();
  if ((uint32_t)(now - lastFlowCalcMs) < FLOW_SAMPLE_MS) return;
  uint32_t dtms = now - lastFlowCalcMs;
  lastFlowCalcMs = now;

  noInterrupts();
  uint32_t pA = pulsesAHU;
  uint32_t pE = pulsesEVAP;
  uint32_t pC = pulsesCOND;
  interrupts();

  uint32_t dA = pA - lastPulsesAHU;
  uint32_t dE = pE - lastPulsesEVAP;
  uint32_t dC = pC - lastPulsesCOND;
  lastPulsesAHU=pA; lastPulsesEVAP=pE; lastPulsesCOND=pC;

  float seconds = (float)dtms/1000.0f;
  // pulses -> gallons
  float gA = (FLOW_PULSES_PER_GAL>0)? (dA / FLOW_PULSES_PER_GAL) : 0;
  float gE = (FLOW_PULSES_PER_GAL>0)? (dE / FLOW_PULSES_PER_GAL) : 0;
  float gC = (FLOW_PULSES_PER_GAL>0)? (dC / FLOW_PULSES_PER_GAL) : 0;

  // gallons/sec -> gpm
  F_AHU_GPM  = (seconds>0)? (gA/seconds*60.0f) : 0;
  F_AHU_GPM  = max(0.0f, F_AHU_GPM + CAL_FlowOfs[0]);
  F_EVAP_GPM = (seconds>0)? (gE/seconds*60.0f) : 0;
  F_EVAP_GPM = max(0.0f, F_EVAP_GPM + CAL_FlowOfs[1]);
  F_COND_GPM = (seconds>0)? (gC/seconds*60.0f) : 0;
  F_COND_GPM = max(0.0f, F_COND_GPM + CAL_FlowOfs[2]);
}

static void updateTemps() {
  tempSensors.readAll();
  float f;
  if (tempSensors.getF(EVAP_LWT, f)) {
    pvEvapLwtF = f + CAL_TempOfsF[IDX_EVAP_LWT];
  } else {
    pvEvapLwtF = NAN;
  }
}


// -----------------------------
// Alarm / trip utilities
// -----------------------------
static void clearAlarms(bool clearLockoutsToo=false) {
  ALM_EvapFlowFailToProve=false;
  ALM_EvapFlowLost=false;
  ALM_CondFlowFailToProve=false;
  ALM_CondFlowLost=false;
  ALM_HiDischP=false;
  ALM_LoSuctionP=false;
  ALM_VFDFault=false;
  ALM_SensorFault=false;
  if (clearLockoutsToo) {
    // nothing extra yet
  }
}

static void goMode(Mode m, const char* why=nullptr) {
  mode = m;
  modeEnterMs = millis();
  if (why) logEvent(why);
}

static void allOutputsSafe() {
  relayWrite(PIN_VFD_RUN_RELAY, false);
  relayWrite(PIN_EVAP_PUMP_RELAY, false);
  relayWrite(PIN_COND_PUMP_RELAY, false);
  relayWrite(PIN_CT_PUMP_RELAY, false);
  relayWrite(PIN_FILTER_PUMP_RELAY, false);
  relayWrite(PIN_ALARM_BELL, false);
}

// -----------------------------
// Control outputs (stubs)
// -----------------------------
static float AO_CompSpeedCmdPct = 0.0f;

// Additional BAS-style command points (currently stubs / UI-visible only).
// These are defined so SERVICE mode can command them even if the physical
// output hardware (DAC/driver) is not yet wired.
static float AO_TowerFanCmdPct   = 0.0f;  // 0-100%
static float AO_EEV_EvapCmdPct   = 0.0f;  // 0-100%
static float AO_EEV_EconCmdPct   = 0.0f;  // 0-100%

static void setCompSpeedPct(float pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  AO_CompSpeedCmdPct = pct;
  // TODO: output to DAC/PWM here (PATCH_POINT_VFD)
}

// ramp helper
static float rampTo(float current, float target, float ratePctPerSec, uint32_t dtms) {
  float step = ratePctPerSec * ((float)dtms/1000.0f);
  if (current < target) {
    current += step;
    if (current > target) current = target;
  } else if (current > target) {
    current -= step;
    if (current < target) current = target;
  }
  return current;
}

// -----------------------------
// Safety checks / permissives
// -----------------------------
static bool callForCooling() {
  if (isnan(pvEvapLwtF)) return false;
  // Simple hysteresis
  static bool call=false;
  if (!call && pvEvapLwtF >= (SP_LWT_F + SP_LWT_DB_F)) call=true;
  if (call && pvEvapLwtF <= SP_LWT_F) call=false;
  return call;
}

static bool vfdReady() { return readBI(PIN_VFD_READY, true); }
static bool vfdFault() { return readBI(PIN_VFD_FAULT, false); }
static bool vfdRunFb() { return readBI(PIN_VFD_RUNFB, false); }

static bool pressuresValidForStart() {
  if (isnan(pvDischP) || isnan(pvSuctP)) { ALM_SensorFault=true; return false; }
  if (pvDischP > LIM_DISCH_P_HIGH_TRIP) { ALM_HiDischP=true; return false; }
  if (pvSuctP < LIM_SUCTION_P_LOW_TRIP) { ALM_LoSuctionP=true; return false; }
  return true;
}

static bool evapFlowOk() { return F_EVAP_GPM >= EVAP_MIN_GPM; }
static bool condFlowOk() { return F_COND_GPM >= COND_MIN_GPM; }

// -----------------------------
// Training fault injection
// -----------------------------
static void applySimulationDeltas() {
  if (!SIM_Enable) return;

  ALM_TrainingMode = true;

  if (strcmp(SIM_Fault,"EVAP_FLOW_FAIL")==0) {
    F_EVAP_GPM = 0.0f;
  } else if (strcmp(SIM_Fault,"EVAP_FLOW_LOST")==0) {
    // drop during run handled by caller; we just force low
    F_EVAP_GPM = 0.0f;
  } else if (strcmp(SIM_Fault,"COND_FLOW_FAIL")==0) {
    F_COND_GPM = 0.0f;
  } else if (strcmp(SIM_Fault,"COND_FLOW_LOST")==0) {
    F_COND_GPM = 0.0f;
  } else if (strcmp(SIM_Fault,"HI_DISCH_P")==0) {
    pvDischP = LIM_DISCH_P_HIGH_TRIP + 25.0f;
  } else if (strcmp(SIM_Fault,"LO_SUCTION_P")==0) {
    pvSuctP = LIM_SUCTION_P_LOW_TRIP - 5.0f;
  } else if (strcmp(SIM_Fault,"VFD_FAULT_START")==0) {
    // emulate fault input
    ALM_VFDFault = true;
  } else if (strcmp(SIM_Fault,"SENSOR_STUCK_LWT")==0) {
    // freeze LWT at a constant by not updating pvEvapLwtF elsewhere (handled by command)
  }
}

// -----------------------------
// Core state machine
// -----------------------------
static uint32_t evapFlowBadSince=0;
static uint32_t condFlowBadSince=0;

static void updateFlowLossTimers() {
  uint32_t now=millis();
  if (!evapFlowOk()) {
    if (evapFlowBadSince==0) evapFlowBadSince=now;
  } else {
    evapFlowBadSince=0;
  }
  if (!condFlowOk()) {
    if (condFlowBadSince==0) condFlowBadSince=now;
  } else {
    condFlowBadSince=0;
  }
}

static bool flowBadTooLong(uint32_t badSince, uint32_t delayMs){
  if (badSince==0) return false;
  return (uint32_t)(millis()-badSince) >= delayMs;
}

static void tripToFault(const char* reason) {
  allOutputsSafe();
  setCompSpeedPct(0);
  lastStopMs = millis();
  goMode(MODE_FAULT, reason);
}

static void loopStateMachine() {
  uint32_t now = millis();

  // global immediate faults
  if (vfdFault() || ALM_VFDFault) {
    ALM_VFDFault=true;
    tripToFault("Trip: VFD Fault");
    return;
  }
  if (!pressuresValidForStart() && (mode==MODE_START_VFD || mode==MODE_RUN_STABILIZE || mode==MODE_RUN)) {
    // treat as lockout in real life; baseline uses FAULT
    tripToFault("Trip: Pressure Limit");
    return;
  }

  updateFlowLossTimers();
  if ((mode==MODE_RUN_STABILIZE || mode==MODE_RUN) && flowBadTooLong(evapFlowBadSince, FLOW_LOSS_MS)) {
    ALM_EvapFlowLost=true;
    tripToFault("Trip: Evap Flow Lost");
    return;
  }
  if ((mode==MODE_RUN_STABILIZE || mode==MODE_RUN) && flowBadTooLong(condFlowBadSince, FLOW_LOSS_MS)) {
    ALM_CondFlowLost=true;
    tripToFault("Trip: Cond Flow Lost");
    return;
  }

  // state actions
  switch(mode){

    case MODE_OFF:
      allOutputsSafe();
      setCompSpeedPct(0);
      if (callForCooling() && (uint32_t)(now-lastStopMs) >= MIN_OFF_MS) {
        clearAlarms();
        goMode(MODE_PRECHECK, "Mode: PRECHECK");
      } else {
        goMode(MODE_IDLE_READY, "Mode: IDLE_READY");
      }
      break;

    case MODE_IDLE_READY:
      allOutputsSafe();
      setCompSpeedPct(0);
      if (callForCooling() && (uint32_t)(now-lastStopMs) >= MIN_OFF_MS) {
        clearAlarms();
        goMode(MODE_PRECHECK, "Mode: PRECHECK");
      }
      break;

    case MODE_PRECHECK:
      // Start pumps
      relayWrite(PIN_EVAP_PUMP_RELAY, true);
      relayWrite(PIN_COND_PUMP_RELAY, true);
      // Optionally start tower pumps here too if they are same as condenser
      // Begin proving
      goMode(MODE_FLOW_PROVE, "Mode: FLOW_PROVE");
      break;

    case MODE_FLOW_PROVE: {
      relayWrite(PIN_EVAP_PUMP_RELAY, true);
      relayWrite(PIN_COND_PUMP_RELAY, true);

      uint32_t elapsed = now - modeEnterMs;
      bool evapOk = evapFlowOk();
      bool condOk = condFlowOk();

      if (elapsed < FLOW_PROVE_MS) {
        // wait
        (void)evapOk; (void)condOk;
      } else {
        if (!evapOk) { ALM_EvapFlowFailToProve=true; tripToFault("Start Fail: Evap Flow"); break; }
        if (!condOk) { ALM_CondFlowFailToProve=true; tripToFault("Start Fail: Cond Flow"); break; }
        // Next checks
        goMode(MODE_EEV_HOME, "Mode: EEV_HOME");
      }
    } break;

    case MODE_EEV_HOME:
      // In real implementation, command EEV close/home and wait for BI
      if (!BI_EEV_EvapHomed || !BI_EEV_EconHomed) {
        // wait up to a timeout
        if ((uint32_t)(now-modeEnterMs) > 15000) {
          tripToFault("Start Fail: EEV Home");
        }
      } else {
        goMode(MODE_START_VFD, "Mode: START_VFD");
      }
      break;

    case MODE_START_VFD:
      relayWrite(PIN_EVAP_PUMP_RELAY, true);
      relayWrite(PIN_COND_PUMP_RELAY, true);

      if (!vfdReady()) {
        // wait a bit, then fault
        if ((uint32_t)(now-modeEnterMs) > 10000) {
          ALM_VFDFault=true;
          tripToFault("Start Fail: VFD Not Ready");
        }
        break;
      }

      relayWrite(PIN_VFD_RUN_RELAY, true);
      // ramp to min speed
      setCompSpeedPct(LIM_COMP_MIN_SPEED_PCT);
      lastStartMs = now;
      goMode(MODE_RUN_STABILIZE, "Mode: RUN_STABILIZE");
      break;

    case MODE_RUN_STABILIZE:
      relayWrite(PIN_EVAP_PUMP_RELAY, true);
      relayWrite(PIN_COND_PUMP_RELAY, true);
      relayWrite(PIN_VFD_RUN_RELAY, true);

      // hold min speed during stabilize
      setCompSpeedPct(LIM_COMP_MIN_SPEED_PCT);

      if ((uint32_t)(now-modeEnterMs) >= STABILIZE_MS) {
        goMode(MODE_RUN, "Mode: RUN");
      }
      // If call drops during stabilize, stop
      if (!callForCooling() && (uint32_t)(now-lastStartMs) >= MIN_ON_MS) {
        goMode(MODE_SOFT_STOP, "Mode: SOFT_STOP");
      }
      break;

    case MODE_RUN: {
      relayWrite(PIN_EVAP_PUMP_RELAY, true);
      relayWrite(PIN_COND_PUMP_RELAY, true);
      relayWrite(PIN_VFD_RUN_RELAY, true);

      // Simple proportional control on LWT error as a baseline
      // (Replace with PID later if desired)
      if (isnan(pvEvapLwtF)) {
        ALM_SensorFault=true;
        tripToFault("Trip: LWT Sensor");
        break;
      }

      float err = pvEvapLwtF - SP_LWT_F; // positive = too warm -> speed up
      float target = LIM_COMP_MIN_SPEED_PCT + err * 10.0f; // 10% per °F (starter)
      if (target > LIM_COMP_MAX_SPEED_PCT) target = LIM_COMP_MAX_SPEED_PCT;
      if (target < LIM_COMP_MIN_SPEED_PCT) target = LIM_COMP_MIN_SPEED_PCT;

      static uint32_t lastCtrlMs=0;
      uint32_t dtms = (lastCtrlMs==0)? 200 : (now-lastCtrlMs);
      lastCtrlMs = now;

      float rate = (target>AO_CompSpeedCmdPct)? LIM_COMP_RAMP_UP_PCT_PER_SEC : LIM_COMP_RAMP_DOWN_PCT_PER_SEC;
      float newOut = rampTo(AO_CompSpeedCmdPct, target, rate, dtms);
      setCompSpeedPct(newOut);

      // Stop request
      if (!callForCooling() && (uint32_t)(now-lastStartMs) >= MIN_ON_MS) {
        goMode(MODE_SOFT_STOP, "Mode: SOFT_STOP");
      }
    } break;

    case MODE_SOFT_STOP:
      // Ramp down then stop
      relayWrite(PIN_EVAP_PUMP_RELAY, true);
      relayWrite(PIN_COND_PUMP_RELAY, true);
      relayWrite(PIN_VFD_RUN_RELAY, true);

      {
        static uint32_t lastMs=0;
        uint32_t dtms = (lastMs==0)? 200 : (now-lastMs);
        lastMs=now;
        float newOut = rampTo(AO_CompSpeedCmdPct, 0.0f, LIM_COMP_RAMP_DOWN_PCT_PER_SEC, dtms);
        setCompSpeedPct(newOut);
        if (AO_CompSpeedCmdPct <= 0.5f) {
          relayWrite(PIN_VFD_RUN_RELAY, false);
          setCompSpeedPct(0);
          // post-run pumps
          goMode(MODE_UNLOAD, "Mode: POSTRUN");
        }
      }
      break;

    case MODE_UNLOAD:
      // Post-run
      relayWrite(PIN_EVAP_PUMP_RELAY, true);
      relayWrite(PIN_COND_PUMP_RELAY, true);
      if ((uint32_t)(now-modeEnterMs) >= POSTRUN_MS) {
        relayWrite(PIN_EVAP_PUMP_RELAY, false);
        relayWrite(PIN_COND_PUMP_RELAY, false);
        lastStopMs = now;
        goMode(MODE_IDLE_READY, "Mode: IDLE_READY");
      }
      break;

    case MODE_FAULT:
      allOutputsSafe();
      setCompSpeedPct(0);
      relayWrite(PIN_ALARM_BELL, true);
      // Remain until manual reset command received
      break;

    case MODE_LOCKOUT:
      allOutputsSafe();
      setCompSpeedPct(0);
      relayWrite(PIN_ALARM_BELL, true);
      break;
  }
}

// -----------------------------
// JSON publishing
// -----------------------------
static const uint32_t PUBLISH_MS = 250; // fast UI updates
static uint32_t lastPublish=0;

static void jsonPrintEsc(const char* s){
  // minimal escaping
  for (; *s; s++){
    char c=*s;
    if (c=='\"' || c=='\\') { Serial.print('\\'); Serial.print(c); }
    else Serial.print(c);
  }
}

static void printJson() {
  Serial.print('{');

  // Mode/status
  Serial.print("\"STAT_Mode\":\""); Serial.print(modeName(mode)); Serial.print("\",");
  Serial.print("\"STAT_ModeTimer_s\":"); Serial.print((millis()-modeEnterMs)/1000.0f,1); Serial.print(',');
  Serial.print("\"STAT_TrainingMode\":"); Serial.print(SIM_Enable ? "true":"false"); Serial.print(',');
  Serial.print("\"STAT_ServiceMode\":"); Serial.print(STAT_ServiceMode ? "true":"false"); Serial.print(',');
  Serial.print("\"MAN_EvapPump\":"); Serial.print(MAN_EvapPump ? "true":"false"); Serial.print(',');
  Serial.print("\"MAN_CondPump\":"); Serial.print(MAN_CondPump ? "true":"false"); Serial.print(',');
  Serial.print("\"MAN_CTPump\":"); Serial.print(MAN_CTPump ? "true":"false"); Serial.print(',');
  Serial.print("\"MAN_VFDRun\":"); Serial.print(MAN_VFDRun ? "true":"false"); Serial.print(',');
  Serial.print("\"MAN_VFDSpeedPct\":"); Serial.print(MAN_VFDSpeedPct,1); Serial.print(',');
  Serial.print("\"MAN_TowerFan\":"); Serial.print(MAN_TowerFan ? "true":"false"); Serial.print(',');
  Serial.print("\"MAN_TowerFanPct\":"); Serial.print(MAN_TowerFanPct,1); Serial.print(',');

  // Setpoints
  Serial.print("\"SP_LWT\":"); Serial.print(SP_LWT_F,1); Serial.print(',');
  Serial.print("\"SP_LWT_DB\":"); Serial.print(SP_LWT_DB_F,1); Serial.print(',');
  Serial.print("\"CAL_EvapLWT_OfsF\":"); Serial.print(CAL_TempOfsF[IDX_EVAP_LWT],2); Serial.print(',');
  Serial.print("\"CAL_EvapFlow_OfsGPM\":"); Serial.print(CAL_FlowOfs[1],2); Serial.print(',');
  Serial.print("\"CAL_CondFlow_OfsGPM\":"); Serial.print(CAL_FlowOfs[2],2); Serial.print(',');

  // Commands
  Serial.print("\"AO_CompSpeedCmd_pct\":"); Serial.print(AO_CompSpeedCmdPct,1); Serial.print(',');

  // Temps
  for (uint8_t i=0;i<NUM_TEMP;i++){
    float f;
    bool ok = tempSensors.getF(TEMP_MAP[i].id, f);
    Serial.print('\"'); Serial.print(TEMP_MAP[i].key); Serial.print('\"'); Serial.print(':');
    if (ok && !isnan(f)) { f += CAL_TempOfsF[i]; Serial.print(f,1); }
    else Serial.print("null");
    Serial.print(',');
  }

  // Pressures
  for (uint8_t i=0;i<NUM_PRESS;i++){
    Serial.print('\"'); Serial.print(PRESS_CHANS[i].key); Serial.print('\"'); Serial.print(':');
    if (!isnan(pressPsig[i])) Serial.print(pressPsig[i],1);
    else Serial.print("null");
    Serial.print(',');
  }

  // Flows
  Serial.print("\"F_AHU_GPM\":"); Serial.print(F_AHU_GPM,2); Serial.print(',');
  Serial.print("\"F_EVAP_GPM\":"); Serial.print(F_EVAP_GPM,2); Serial.print(',');
  Serial.print("\"F_COND_GPM\":"); Serial.print(F_COND_GPM,2); Serial.print(',');

  // VFD BIs
  Serial.print("\"BI_VFD_Ready\":"); Serial.print(vfdReady() ? "true":"false"); Serial.print(',');
  Serial.print("\"BI_VFD_Fault\":"); Serial.print(vfdFault() ? "true":"false"); Serial.print(',');
  Serial.print("\"BI_VFD_Run\":"); Serial.print(vfdRunFb() ? "true":"false"); Serial.print(',');

  // Alarms
  Serial.print("\"ALM_EvapFlowFailToProve\":"); Serial.print(ALM_EvapFlowFailToProve?"true":"false"); Serial.print(',');
  Serial.print("\"ALM_EvapFlowLost\":"); Serial.print(ALM_EvapFlowLost?"true":"false"); Serial.print(',');
  Serial.print("\"ALM_CondFlowFailToProve\":"); Serial.print(ALM_CondFlowFailToProve?"true":"false"); Serial.print(',');
  Serial.print("\"ALM_CondFlowLost\":"); Serial.print(ALM_CondFlowLost?"true":"false"); Serial.print(',');
  Serial.print("\"ALM_HiDischP\":"); Serial.print(ALM_HiDischP?"true":"false"); Serial.print(',');
  Serial.print("\"ALM_LoSuctionP\":"); Serial.print(ALM_LoSuctionP?"true":"false"); Serial.print(',');
  Serial.print("\"ALM_VFDFault\":"); Serial.print(ALM_VFDFault?"true":"false"); Serial.print(',');
  Serial.print("\"ALM_SensorFault\":"); Serial.print(ALM_SensorFault?"true":"false"); Serial.print(',');
  Serial.print("\"ALM_TrainingMode\":"); Serial.print(ALM_TrainingMode?"true":"false"); Serial.print(',');

  // Last event (most recent prior entry)
  uint8_t last = (EVT_HEAD==0)? (EVT_DEPTH-1) : (EVT_HEAD-1);
  Serial.print("\"EVT_Last\":\""); jsonPrintEsc(EVT_TEXT[last]); Serial.print("\"");

  Serial.println('}');
}

// -----------------------------
// Serial command parser (very small, forgiving)
// -----------------------------
static void handleCommand(const String& line) {
  String s=line;
  s.trim();
  if (s.length()==0) return;

  if (s.equalsIgnoreCase("RESET")) {
    clearAlarms(true);
    ALM_TrainingMode=false;
    SIM_Enable=false;
    strcpy(SIM_Fault,"NONE");
    STAT_ServiceMode=false;
    MAN_EvapPump=false; MAN_CondPump=false; MAN_CTPump=false;
    MAN_AlarmBell=false; MAN_VFDRun=false; MAN_TowerFan=false;
    MAN_VFDSpeedPct=0; MAN_TowerFanPct=0; MAN_EEV_EvapPct=0; MAN_EEV_EconPct=0;
    relayWrite(PIN_ALARM_BELL, false);
    goMode(MODE_IDLE_READY, "Cmd: RESET");
    return;
  }

  if (s.equalsIgnoreCase("MODE AUTO"))    { STAT_ServiceMode=false; logEvent("Cmd: MODE AUTO"); return; }
  if (s.equalsIgnoreCase("MODE SERVICE")) { STAT_ServiceMode=true;  logEvent("Cmd: MODE SERVICE"); return; }

  if (s.equalsIgnoreCase("SIM ON"))  { SIM_Enable=true;  ALM_TrainingMode=true; logEvent("Cmd: SIM ON"); return; }
  if (s.equalsIgnoreCase("SIM OFF")) { SIM_Enable=false; ALM_TrainingMode=false; strcpy(SIM_Fault,"NONE"); logEvent("Cmd: SIM OFF"); return; }

  if (s.startsWith("SIM FAULT")) {
    char name[24]={0};
    if (sscanf(s.c_str(), "SIM FAULT %23s", name)==1) {
      strncpy(SIM_Fault, name, sizeof(SIM_Fault)-1);
      SIM_Fault[sizeof(SIM_Fault)-1]=0;
      SIM_Enable=true;
      ALM_TrainingMode=true;
      logEvent("Cmd: SIM FAULT");
    }
    return;
  }

  if (s.startsWith("PUMP ")) {
    char which[12]={0}, state[8]={0};
    if (sscanf(s.c_str(), "PUMP %11s %7s", which, state)==2) {
      bool on = (strcasecmp(state,"ON")==0);
      if (strcasecmp(which,"EVAP")==0) MAN_EvapPump=on;
      else if (strcasecmp(which,"COND")==0) MAN_CondPump=on;
      else if (strcasecmp(which,"CT")==0) MAN_CTPump=on;
      logEvent("Cmd: PUMP");
    }
    return;
  }

  if (s.startsWith("ALARM_BELL ")) {
    char state[8]={0};
    if (sscanf(s.c_str(), "ALARM_BELL %7s", state)==1) {
      MAN_AlarmBell = (strcasecmp(state,"ON")==0);
      logEvent("Cmd: ALARM_BELL");
    }
    return;
  }

  if (s.startsWith("VFD RUN ")) {
    char state[8]={0};
    if (sscanf(s.c_str(), "VFD RUN %7s", state)==1) {
      MAN_VFDRun = (strcasecmp(state,"ON")==0);
      logEvent("Cmd: VFD RUN");
    }
    return;
  }

  if (s.startsWith("VFD SPEED ")) {
    float pct=0;
    if (sscanf(s.c_str(), "VFD SPEED %f", &pct)==1) {
      MAN_VFDSpeedPct = clampPct(pct);
      logEvent("Cmd: VFD SPEED");
    }
    return;
  }

  if (s.startsWith("TOWER FAN ")) {
    float pct=0;
    if (sscanf(s.c_str(), "TOWER FAN %f", &pct)==1) {
      MAN_TowerFan = true;
      MAN_TowerFanPct = clampPct(pct);
      logEvent("Cmd: TOWER FAN");
    }
    return;
  }

  if (s.startsWith("EEV EVAP ")) {
    float pct=0;
    if (sscanf(s.c_str(), "EEV EVAP %f", &pct)==1) {
      MAN_EEV_EvapPct = clampPct(pct);
      logEvent("Cmd: EEV EVAP");
    }
    return;
  }

  if (s.startsWith("EEV ECON ")) {
    float pct=0;
    if (sscanf(s.c_str(), "EEV ECON %f", &pct)==1) {
      MAN_EEV_EconPct = clampPct(pct);
      logEvent("Cmd: EEV ECON");
    }
    return;
  }

  if (s.startsWith("SP ")) {
    char name[16]={0}; float val=0;
    if (sscanf(s.c_str(), "SP %15s %f", name, &val)==2) {
      if (strcasecmp(name,"LWT")==0) SP_LWT_F = val;
      else if (strcasecmp(name,"LWT_DB")==0) SP_LWT_DB_F = val;
      logEvent("Cmd: SP");
    }
    return;
  }

  if (s.startsWith("CAL TEMP ")) {
    char key[20]={0}; float val=0;
    if (sscanf(s.c_str(), "CAL TEMP %19s %f", key, &val)==2) {
      if (strcasecmp(key,"EVAP_LWT")==0) CAL_TempOfsF[IDX_EVAP_LWT]=val;
      else if (strcasecmp(key,"EVAP_EWT")==0) CAL_TempOfsF[IDX_EVAP_EWT]=val;
      else if (strcasecmp(key,"COND_LWT")==0) CAL_TempOfsF[IDX_COND_LWT]=val;
      else if (strcasecmp(key,"COND_EWT")==0) CAL_TempOfsF[IDX_COND_EWT]=val;
      logEvent("Cmd: CAL TEMP");
    }
    return;
  }

  if (s.startsWith("CAL PRESS ")) {
    char key[24]={0}; float val=0;
    if (sscanf(s.c_str(), "CAL PRESS %23s %f", key, &val)==2) {
      for (uint8_t i=0;i<NUM_PRESS;i++){
        if (strcasecmp(key, PRESS_CHANS[i].key)==0) { CAL_PressOfs[i]=val; break; }
      }
      logEvent("Cmd: CAL PRESS");
    }
    return;
  }

  if (s.startsWith("CAL FLOW ")) {
    char which[12]={0}; float val=0;
    if (sscanf(s.c_str(), "CAL FLOW %11s %f", which, &val)==2) {
      if (strcasecmp(which,"AHU")==0) CAL_FlowOfs[0]=val;
      else if (strcasecmp(which,"EVAP")==0) CAL_FlowOfs[1]=val;
      else if (strcasecmp(which,"COND")==0) CAL_FlowOfs[2]=val;
      logEvent("Cmd: CAL FLOW");
    }
    return;
  }
}


// read lines
static void pollSerialCommands() {
  static String buf;
  while (Serial.available()) {
    char c=(char)Serial.read();
    if (c=='\n' || c=='\r') {
      if (buf.length()>0) handleCommand(buf);
      buf="";
    } else {
      if (buf.length()<120) buf += c;
    }
  }
}

// -----------------------------
// Setup / loop
// -----------------------------
void setup() {
  Serial.begin(115200);

  pinMode(PIN_EVAP_PUMP_RELAY, OUTPUT);
  pinMode(PIN_COND_PUMP_RELAY, OUTPUT);
  pinMode(PIN_CT_PUMP_RELAY, OUTPUT);
  pinMode(PIN_FILTER_PUMP_RELAY, OUTPUT);
  pinMode(PIN_VFD_RUN_RELAY, OUTPUT);
  pinMode(PIN_ALARM_BELL, OUTPUT);

  pinMode(PIN_VFD_READY, INPUT_PULLUP);
  pinMode(PIN_VFD_FAULT, INPUT_PULLUP);
  pinMode(PIN_VFD_RUNFB, INPUT_PULLUP);

  allOutputsSafe();

  // Flow inputs
  pinMode(PIN_FLOW_AHU, INPUT_PULLUP);
  pinMode(PIN_FLOW_EVAP, INPUT_PULLUP);
  pinMode(PIN_FLOW_COND, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_AHU),  isrAhu,  RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_EVAP), isrEvap, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_COND), isrCond, RISING);

  tempSensors.begin();
  lastFlowCalcMs = millis();
  lastStopMs = millis();

  logEvent("Boot");
  goMode(MODE_IDLE_READY, "Mode: IDLE_READY");
}


void applyManualOverrides() {
  if (!STAT_ServiceMode) return;

  if (MAN_EvapPump) relayWrite(PIN_EVAP_PUMP_RELAY, true);
  if (MAN_CondPump) relayWrite(PIN_COND_PUMP_RELAY, true);
  if (MAN_CTPump)   relayWrite(PIN_CT_PUMP_RELAY, true);

  if (MAN_AlarmBell) relayWrite(PIN_ALARM_BELL, true);

  if (MAN_VFDRun) {
    AO_CompSpeedCmdPct = clampPct(MAN_VFDSpeedPct);
    // In SERVICE mode, force the VFD run relay ON.
    relayWrite(PIN_VFD_RUN_RELAY, true);
  }

  if (MAN_TowerFan) {
    AO_TowerFanCmdPct = clampPct(MAN_TowerFanPct);
    // TODO: output to tower fan DAC/PWM here (PATCH_POINT_TOWER_FAN)
  }

  AO_EEV_EvapCmdPct = clampPct(MAN_EEV_EvapPct);
  AO_EEV_EconCmdPct = clampPct(MAN_EEV_EconPct);
  // TODO: output to EEV drivers here (PATCH_POINT_EEV)
}

void loop() {
  pollSerialCommands();

  computeFlow();
  readPressures();

  // Update temps slower than 250ms to reduce DS18B20 bus traffic
  static uint32_t lastTempMs=0;
  uint32_t now=millis();
  if ((uint32_t)(now-lastTempMs) >= 1000) {
    lastTempMs = now;
    if (!(SIM_Enable && strcmp(SIM_Fault,"SENSOR_STUCK_LWT")==0)) {
      updateTemps();
    }
  }

  // Apply training deltas AFTER reading sensors
  applySimulationDeltas();

  loopStateMachine();
  applyManualOverrides();

  if ((uint32_t)(now-lastPublish) >= PUBLISH_MS) {
    lastPublish = now;
    printJson();
  }
}
