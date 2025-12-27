
/*
  Chiller_Sensors_JSON.ino
  Board: Arduino Mega 2560
  Baud:  115200
  Output: newline-delimited JSON with temperatures (°F), pressures (psig), flows (GPM)

  - DS18B20 temps on OneWire bus (pin 2 by default via DS18B20_Sensors.h)
  - 9 analog pressure sensors (0.5–4.5 V => 0–range)
  - 3 FL-608 pulse flow meters (GPM)
*/

#include <Arduino.h>
#define ONEWIRE_PIN 2
#include "DS18B20_Sensors.h"   // uses TempSensorIDs.h

// ========== Temperature JSON mapping ==========
struct TempJsonMap {
  TempSensorID id;
  const char*  key;   // JSON key
  const char*  label; // human label (not used in JSON, but for reference)
};

TempJsonMap TEMP_MAP[] = {
  { COND_EWT,              "COND_EWT_F",          "Cond EWT" },
  { ECON_SUCTION_TEMP,     "ECON_SUCTION_F",      "Economizer Suction Temp" },
  { AHU_SUPPLY_TEMP,       "AHU_SUPPLY_F",        "AHU Supply Temp" },
  { EVAP_LWT,              "EVAP_LWT_F",          "Evap LWT" },
  { LIQUID_POST_ECON,      "LIQ_POST_ECON_F",     "Liquid Temp Post Econ" },
  { COND_LWT,              "COND_LWT_F",          "Cond LWT" },
  { AHU_RETURN_TEMP,       "AHU_RETURN_F",        "AHU Return Temp" },
  { SUCTION_TEMP,          "SUCTION_F",           "Suction Temp" },
  { EVAP_EWT,              "EVAP_EWT_F",          "Evap EWT" },
  { DISCHARGE_TEMP,        "DISCHARGE_F",         "Discharge Temp" },
  { POST_FILTER_DRIER_TEMP,"POST_FILTER_DRIER_F", "Post Filter Drier Temp" },
  { LIQUID_LINE_TEMP,      "LIQ_LINE_F",          "Liquid Line Temp" },
  { ECON_2PHASE_TEMP,      "ECON_2PHASE_F",       "Economizer 2 Phase Temp" },
  { TWO_PHASE_TEMP,        "TWO_PHASE_F",         "Two Phase Temp" }
};

const uint8_t NUM_TEMPS = sizeof(TEMP_MAP) / sizeof(TEMP_MAP[0]);

DS18B20Sensors tempSensors(ONEWIRE_PIN);

// ========== Pressure & Flow section (from Pressure_Flow_Sensors.ino) ==========

// Analog inputs for pressure sensors
#define AI_AHU_PUMP_SUCTION_P         A0
#define AI_AHU_PUMP_DISCH_P           A4
#define AI_EVAP_PUMP_DISCH_P          A8
#define AI_EVAP_PUMP_SUCTION_P        A12
#define AI_CTOWER_DISCH_P             A1
#define AI_AHU_GPM_UNUSED_ANALOG      A5   // not used as analog anymore
#define AI_EVAP_GPM_UNUSED_ANALOG     A9   // not used as analog anymore
#define AI_COND_GPM_UNUSED_ANALOG     A13  // not used as analog anymore
#define AI_COMP_DISCH_PRESS           A2
#define AI_LIQUID_LINE_PRESS          A6
#define AI_TWO_PHASE_PRESS            A10
#define AI_SUCTION_PRESS              A14

// Flow meter digital pins
#define PIN_AHU_FLOW_PULSE            20
#define PIN_EVAP_FLOW_PULSE           3
#define PIN_COND_FLOW_PULSE           21

// Ranges (psig)
const float AHU_PUMP_P_RANGE_PSI    = 80.0f;
const float EVAP_PUMP_P_RANGE_PSI   = 80.0f;
const float CTOWER_P_RANGE_PSI      = 80.0f;
const float REFRIG_P_RANGE_PSI      = 435.0f;  // high side / suction style

// Flow meter scaling (FL-608)
const float FL608_PULSES_PER_LPM    = 5.5f;        // F = 5.5 * Q(L/min)
const float LPM_TO_GPM              = 0.264172f;   // 1 L/min = 0.264172 GPM
const unsigned long FLOW_SAMPLE_INTERVAL_MS = 1000;

// Config for pressure sensors
struct PressureDef {
  uint8_t pin;
  float   range;
  const char* key;    // JSON key
  const char* label;  // human label
  const char* units;
};

PressureDef PRESSURE_SENSORS[] = {
  { AI_AHU_PUMP_SUCTION_P,  AHU_PUMP_P_RANGE_PSI,   "P_AHU_PUMP_SUCTION",   "AHU Pump Suction P",   "psig" },
  { AI_AHU_PUMP_DISCH_P,    AHU_PUMP_P_RANGE_PSI,   "P_AHU_PUMP_DISCH",     "AHU Pump Disch P",     "psig" },
  { AI_EVAP_PUMP_DISCH_P,   EVAP_PUMP_P_RANGE_PSI,  "P_EVAP_PUMP_DISCH",    "Evap Pump Disch P",    "psig" },
  { AI_EVAP_PUMP_SUCTION_P, EVAP_PUMP_P_RANGE_PSI,  "P_EVAP_PUMP_SUCTION",  "Evap Pump Suction P",  "psig" },
  { AI_CTOWER_DISCH_P,      CTOWER_P_RANGE_PSI,     "P_CTOWER_DISCH",       "CTower Disch P",       "psig" },
  { AI_COMP_DISCH_PRESS,    REFRIG_P_RANGE_PSI,     "P_COMP_DISCH",         "Comp Disch P",         "psig" },
  { AI_LIQUID_LINE_PRESS,   REFRIG_P_RANGE_PSI,     "P_LIQUID_LINE",        "Liquid Line P",        "psig" },
  { AI_TWO_PHASE_PRESS,     REFRIG_P_RANGE_PSI,     "P_TWO_PHASE",          "Two-Phase P",          "psig" },
  { AI_SUCTION_PRESS,       REFRIG_P_RANGE_PSI,     "P_SUCTION",            "Suction P",            "psig" }
};

const uint8_t NUM_PRESSURE = sizeof(PRESSURE_SENSORS) / sizeof(PRESSURE_SENSORS[0]);

// Flow meter counters
volatile unsigned long ahuFlowPulseCount  = 0;
volatile unsigned long evapFlowPulseCount = 0;
volatile unsigned long condFlowPulseCount = 0;

void ahuFlowISR()  { ahuFlowPulseCount++;  }
void evapFlowISR() { evapFlowPulseCount++; }
void condFlowISR() { condFlowPulseCount++; }

struct FlowDef {
  const char* key;     // JSON key
  const char* label;   // human label
  volatile unsigned long* pulseCounter;
  float gpm;           // last computed value
};

FlowDef FLOW_SENSORS[] = {
  { "F_AHU_GPM",  "AHU Flow",  &ahuFlowPulseCount,  0.0f },
  { "F_EVAP_GPM", "Evap Flow", &evapFlowPulseCount, 0.0f },
  { "F_COND_GPM", "Cond Flow", &condFlowPulseCount, 0.0f }
};

const uint8_t NUM_FLOW = sizeof(FLOW_SENSORS) / sizeof(FLOW_SENSORS[0]);

unsigned long lastFlowSampleMillis = 0;

float readScaled(uint8_t pin, float range) {
  const uint16_t samples = 8;
  uint32_t acc = 0;
  for (uint16_t i = 0; i < samples; i++) {
    acc += analogRead(pin);
  }
  float adc = float(acc) / samples;
  float v   = (adc / 1023.0f) * 5.0f;

  float x = (v - 0.5f) / 4.0f; // 0–1
  if (x < 0.0f) x = 0.0f;
  if (x > 1.0f) x = 1.0f;

  return x * range;
}

void computeFlowFromPulses() {
  unsigned long now = millis();
  unsigned long dt = now - lastFlowSampleMillis;
  if (dt < FLOW_SAMPLE_INTERVAL_MS) {
    return;
  }

  unsigned long counts[NUM_FLOW];

  noInterrupts();
  counts[0] = ahuFlowPulseCount;
  counts[1] = evapFlowPulseCount;
  counts[2] = condFlowPulseCount;
  ahuFlowPulseCount  = 0;
  evapFlowPulseCount = 0;
  condFlowPulseCount = 0;
  interrupts();

  lastFlowSampleMillis = now;

  float dtSeconds = dt / 1000.0f;
  if (dtSeconds <= 0.0f) dtSeconds = 1.0f;

  for (uint8_t i = 0; i < NUM_FLOW; i++) {
    float freqHz = counts[i] / dtSeconds;
    float qLmin  = freqHz / FL608_PULSES_PER_LPM;
    if (qLmin < 0.0f) qLmin = 0.0f;
    float gpm    = qLmin * LPM_TO_GPM;
    if (gpm < 0.01f) gpm = 0.0f;
    FLOW_SENSORS[i].gpm = gpm;
  }
}

// ========== JSON output ==========

const uint32_t PUBLISH_MS = 1000;
uint32_t lastPublish = 0;

void printJson() {
  bool first = true;
  Serial.print('{');

  // Temperatures (F)
  for (uint8_t i = 0; i < NUM_TEMPS; i++) {
    float f;
    bool ok = tempSensors.getF(TEMP_MAP[i].id, f);
    if (!first) Serial.print(',');
    first = false;
    Serial.print('\"'); Serial.print(TEMP_MAP[i].key); Serial.print('\"');
    Serial.print(':');
    if (ok && !isnan(f)) {
      Serial.print(f, 1); // 0.1°F
    } else {
      Serial.print("null");
    }
  }

  // Pressures (psig)
  for (uint8_t i = 0; i < NUM_PRESSURE; i++) {
    float value = readScaled(PRESSURE_SENSORS[i].pin, PRESSURE_SENSORS[i].range);
    if (!first) Serial.print(',');
    first = false;
    Serial.print('\"'); Serial.print(PRESSURE_SENSORS[i].key); Serial.print('\"');
    Serial.print(':');
    Serial.print(value, 1);
  }

  // Flows (GPM)
  for (uint8_t i = 0; i < NUM_FLOW; i++) {
    float gpm = FLOW_SENSORS[i].gpm;
    if (!first) Serial.print(',');
    first = false;
    Serial.print('\"'); Serial.print(FLOW_SENSORS[i].key); Serial.print('\"');
    Serial.print(':');
    Serial.print(gpm, 2);
  }

  Serial.println('}');
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  // Temps
  tempSensors.begin();

  // Pressure analog pins
  analogReference(DEFAULT);

  // Flow meter pins and interrupts
  pinMode(PIN_AHU_FLOW_PULSE, INPUT_PULLUP);
  pinMode(PIN_EVAP_FLOW_PULSE, INPUT_PULLUP);
  pinMode(PIN_COND_FLOW_PULSE, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_AHU_FLOW_PULSE),  ahuFlowISR,  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_EVAP_FLOW_PULSE), evapFlowISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_COND_FLOW_PULSE), condFlowISR, FALLING);

  lastFlowSampleMillis = millis();
  lastPublish = millis();
}

void loop() {
  computeFlowFromPulses();

  uint32_t now = millis();
  if ((uint32_t)(now - lastPublish) >= PUBLISH_MS) {
    lastPublish = now;
    tempSensors.readAll();  // update temperature cache
    printJson();
  }
}
