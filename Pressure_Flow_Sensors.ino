// Pressure and Flow Monitor for Chiller Project (with FL-608 pulse flow meters)
// - Reads 9 analog pressure sensors (0.5–4.5 V = 0–range)
// - Reads 3 FL-608 Hall-effect pulse flow meters using interrupts
//
// Assumptions for FL-608:
//   F(Hz) = 5.5 * Q(L/min)
//   Q_Lmin = F / 5.5
//   GPM = Q_Lmin * 0.264172
//
// HARDWARE WIRING (Arduino Mega 2560):
//   Pressure transducers: 0.5–4.5 V outputs wired to analog pins listed below.
//   Flow meters (FL-608, open-collector):
//     - Red   -> +5V (check your sensor spec, often 5–24V is OK; 5V is safest with Arduino)
//     - Black -> GND
//     - Yellow (signal) per meter:
//         AHU  -> pin 2
//         EVAP -> pin 3
//         COND -> pin 18
//     - In code we use INPUT_PULLUP, so the sensor must pull the line LOW on pulses.
//   NOTE: The older mapping to A5/A9/A13 is NOT used for flow anymore; use the digital pins above.

#include <Arduino.h>

// ---------- Pressure sensor pin assignments (from user) ----------
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

// ---------- Flow meter digital pin assignments ----------
#define PIN_AHU_FLOW_PULSE            18
#define PIN_EVAP_FLOW_PULSE           3
#define PIN_COND_FLOW_PULSE           19

// ---------- Configurable ranges ----------
// Water-side differential/line pressure (psig)
const float AHU_PUMP_P_RANGE_PSI    = 80.0f;   // adjust as needed
const float EVAP_PUMP_P_RANGE_PSI   = 80.0f;   // adjust as needed
const float CTOWER_P_RANGE_PSI      = 80.0f;   // adjust as needed

// Refrigerant pressures (psig)
const float REFRIG_P_RANGE_PSI      = 435.0f;  // typical 0–30 bar style sensors

// Flow meter scaling (for FL-608)
const float FL608_PULSES_PER_LPM    = 5.5f;          // F = 5.5 * Q(L/min)
const float LPM_TO_GPM              = 0.264172f;     // 1 L/min = 0.264172 GPM

// Sample interval for flow calculation (ms)
const unsigned long FLOW_SAMPLE_INTERVAL_MS = 1000;  // 1-second window

// ---------- Helper structs ----------
struct AnalogSensorConfig {
  uint8_t pin;
  float   range;
  const char* label;
  const char* units;
};

// ---------- Pressure sensor configuration ----------
AnalogSensorConfig pressureSensors[] = {
  { AI_AHU_PUMP_SUCTION_P,  AHU_PUMP_P_RANGE_PSI,   "AHU Pump Suction P",   "psig" },
  { AI_AHU_PUMP_DISCH_P,    AHU_PUMP_P_RANGE_PSI,   "AHU Pump Disch P",     "psig" },
  { AI_EVAP_PUMP_DISCH_P,   EVAP_PUMP_P_RANGE_PSI,  "Evap Pump Disch P",    "psig" },
  { AI_EVAP_PUMP_SUCTION_P, EVAP_PUMP_P_RANGE_PSI,  "Evap Pump Suction P",  "psig" },
  { AI_CTOWER_DISCH_P,      CTOWER_P_RANGE_PSI,     "CTower Disch P",       "psig" },
  { AI_COMP_DISCH_PRESS,    REFRIG_P_RANGE_PSI,     "Comp Disch P",         "psig" },
  { AI_LIQUID_LINE_PRESS,   REFRIG_P_RANGE_PSI,     "Liquid Line P",        "psig" },
  { AI_TWO_PHASE_PRESS,     REFRIG_P_RANGE_PSI,     "Two-Phase P",          "psig" },
  { AI_SUCTION_PRESS,       REFRIG_P_RANGE_PSI,     "Suction P",            "psig" }
};

const uint8_t NUM_PRESSURE = sizeof(pressureSensors) / sizeof(pressureSensors[0]);

// ---------- Flow meter pulse counters ----------
volatile unsigned long ahuFlowPulseCount  = 0;
volatile unsigned long evapFlowPulseCount = 0;
volatile unsigned long condFlowPulseCount = 0;

// Simple ISRs for Arduino Mega (no IRAM_ATTR)
void ahuFlowISR() {
  ahuFlowPulseCount++;
}

void evapFlowISR() {
  evapFlowPulseCount++;
}

void condFlowISR() {
  condFlowPulseCount++;
}

// Structure to process each meter
struct FlowMeter {
  const char* label;
  volatile unsigned long* pulseCounter;
  float gpm;  // last computed value
};

FlowMeter flowMeters[] = {
  { "AHU Flow",  &ahuFlowPulseCount,  0.0f },
  { "Evap Flow", &evapFlowPulseCount, 0.0f },
  { "Cond Flow", &condFlowPulseCount, 0.0f }
};

const uint8_t NUM_FLOW = sizeof(flowMeters) / sizeof(flowMeters[0]);

// ---------- Analog reading helper ----------
// 0.5–4.5 V => 0–range
float readScaled(uint8_t pin, float range) {
  const uint16_t samples = 8;   // simple averaging
  uint32_t acc = 0;
  for (uint16_t i = 0; i < samples; i++) {
    acc += analogRead(pin);
  }

  float adc = float(acc) / samples;        // averaged ADC reading
  float v   = (adc / 1023.0f) * 5.0f;      // convert to volts

  float x = (v - 0.5f) / 4.0f;             // 0 -> 0.5 V, 1 -> 4.5 V
  if (x < 0.0f) x = 0.0f;
  if (x > 1.0f) x = 1.0f;

  return x * range;                        // scale to engineering units
}

// ---------- Setup / Loop ----------
unsigned long lastFlowSampleMillis = 0;

void setup() {
  Serial.begin(115200);
  analogReference(DEFAULT);  // 5 V reference on Mega

  // Banner
  Serial.println();
  Serial.println(F("=== Chiller Pressure & FL-608 Flow Monitor ==="));
  Serial.println(F("Pressure sensors: 0.5–4.5 V -> 0–range (psig)."));
  Serial.println(F("Flow (FL-608): F(Hz) = 5.5 * Q(L/min), GPM from pulses."));
  Serial.println();

  // Flow meter pins (input with pullup)
  pinMode(PIN_AHU_FLOW_PULSE,  INPUT_PULLUP);
  pinMode(PIN_EVAP_FLOW_PULSE, INPUT_PULLUP);
  pinMode(PIN_COND_FLOW_PULSE, INPUT_PULLUP);

  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(PIN_AHU_FLOW_PULSE),  ahuFlowISR,  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_EVAP_FLOW_PULSE), evapFlowISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_COND_FLOW_PULSE), condFlowISR, FALLING);

  lastFlowSampleMillis = millis();
}

void computeFlowFromPulses() {
  unsigned long now = millis();
  unsigned long dt = now - lastFlowSampleMillis;
  if (dt < FLOW_SAMPLE_INTERVAL_MS) {
    return; // not time yet
  }

  // Capture and reset pulse counters atomically
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

  // Compute GPM for each meter
  float dtSeconds = dt / 1000.0f;
  if (dtSeconds <= 0.0f) dtSeconds = 1.0f; // avoid div by zero

  for (uint8_t i = 0; i < NUM_FLOW; i++) {
    float freqHz = counts[i] / dtSeconds;          // pulses per second
    float qLmin  = freqHz / FL608_PULSES_PER_LPM;  // L/min
    if (qLmin < 0.0f) qLmin = 0.0f;
    float gpm    = qLmin * LPM_TO_GPM;
    if (gpm < 0.01f) gpm = 0.0f;                   // small deadband to zero noise
    flowMeters[i].gpm = gpm;
  }
}

void printAllSensors() {
  // Read & print pressures
  Serial.println(F("--- Pressure Sensors ---"));
  for (uint8_t i = 0; i < NUM_PRESSURE; i++) {
    float value = readScaled(pressureSensors[i].pin, pressureSensors[i].range);
    Serial.print(pressureSensors[i].label);
    Serial.print(F(": "));
    Serial.print(value, 1);  // 0.1 psig resolution
    Serial.print(F(" "));
    Serial.println(pressureSensors[i].units);
  }

  // Print last computed flow values
  Serial.println(F("--- Flow Sensors (FL-608) ---"));
  for (uint8_t i = 0; i < NUM_FLOW; i++) {
    Serial.print(flowMeters[i].label);
    Serial.print(F(": "));
    Serial.print(flowMeters[i].gpm, 2);  // 0.01 GPM resolution
    Serial.println(F(" gpm"));
  }

  Serial.println();
}

void loop() {
  // Update flow readings from pulses (runs once per FLOW_SAMPLE_INTERVAL_MS)
  computeFlowFromPulses();

  // Print everything once per second
  static unsigned long lastPrint = 0;
  unsigned long now = millis();
  if (now - lastPrint >= 1000) {
    lastPrint = now;
    printAllSensors();
  }
}
