\
#include <Wire.h>
#include <Adafruit_MCP4725.h>

// Addresses for Aptinex 0-10V MCP4725 modules
// Evap Pump DAC: 0x60
// AHU Pump DAC:  0x61
static const uint8_t EVAP_ADDR = 0x60;
static const uint8_t AHU_ADDR  = 0x61;

Adafruit_MCP4725 dacEvap;
Adafruit_MCP4725 dacAHU;

// Voltage range (module outputs 0-10V)
const float V_MIN      = 2.0f;   // Minimum output voltage
const float V_MAX      = 10.0f;  // Maximum output voltage
const float V_STEP     = 0.1f;   // Voltage step per update
const unsigned long STEP_DELAY_MS = 100; // 0.1s per step -> 1V/sec (0.1V every 0.1s)

// Convert desired output voltage (0-10V) to DAC code (0-4095)
uint16_t voltageToDAC(float v)
{
  if (v < 0.0f)  v = 0.0f;
  if (v > 10.0f) v = 10.0f;
  // MCP4725 is 12-bit (0-4095) and board scales 0-5V DAC to 0-10V output,
  // so we can treat it as 0-10V range in the math here.
  return (uint16_t)((v / 10.0f) * 4095.0f + 0.5f);
}

// Helper to write both DACs and print status
void setOutputs(float evapV, float ahuV)
{
  uint16_t evapCode = voltageToDAC(evapV);
  uint16_t ahuCode  = voltageToDAC(ahuV);

  dacEvap.setVoltage(evapCode, false);
  dacAHU.setVoltage(ahuCode,  false);

  Serial.print(F("EVAP PUMP: "));
  Serial.print(evapV, 2);
  Serial.print(F(" V    AHU PUMP: "));
  Serial.print(ahuV, 2);
  Serial.println(F(" V"));
}

enum PumpSelect {
  PUMP_EVAP,
  PUMP_AHU
};

// Ramp a single pump up and down while holding the other at V_MIN
void rampPump(PumpSelect which)
{
  float evapV = V_MIN;
  float ahuV  = V_MIN;

  // Ensure both start at minimum
  setOutputs(evapV, ahuV);

  if (which == PUMP_EVAP) {
    Serial.println(F("Starting EVAP pump ramp (2 -> 10 -> 2 V)"));
    // EVAP ramp UP 2 -> 10
    for (evapV = V_MIN; evapV <= V_MAX + 0.0001f; evapV += V_STEP) {
      setOutputs(evapV, ahuV);
      delay(STEP_DELAY_MS);
    }
    // EVAP ramp DOWN 10 -> 2
    for (evapV = V_MAX; evapV >= V_MIN - 0.0001f; evapV -= V_STEP) {
      setOutputs(evapV, ahuV);
      delay(STEP_DELAY_MS);
    }
    Serial.println(F("Completed EVAP pump ramp"));
    evapV = V_MIN;
  } else {
    Serial.println(F("Starting AHU pump ramp (2 -> 10 -> 2 V)"));
    // AHU ramp UP 2 -> 10
    for (ahuV = V_MIN; ahuV <= V_MAX + 0.0001f; ahuV += V_STEP) {
      setOutputs(evapV, ahuV);
      delay(STEP_DELAY_MS);
    }
    // AHU ramp DOWN 10 -> 2
    for (ahuV = V_MAX; ahuV >= V_MIN - 0.0001f; ahuV -= V_STEP) {
      setOutputs(evapV, ahuV);
      delay(STEP_DELAY_MS);
    }
    Serial.println(F("Completed AHU pump ramp"));
    ahuV = V_MIN;
  }

  // Return both to minimum at the end of the ramp
  setOutputs(V_MIN, V_MIN);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial on boards that need it
  }

  Serial.println(F("MCP4725 0-10V Pump Ramp - Commanded Mode"));
  Serial.println(F("EVAP @ 0x60, AHU @ 0x61"));
  Serial.println(F("Type 'E' + Enter to ramp EVAP pump (2-10-2 V)."));
  Serial.println(F("Type 'A' + Enter to ramp AHU pump (2-10-2 V)."));

  Wire.begin();

  dacEvap.begin(EVAP_ADDR);
  dacAHU.begin(AHU_ADDR);

  // Initialize both pumps at minimum voltage
  setOutputs(V_MIN, V_MIN);
}

void loop()
{
  // Listen for simple serial commands:
  // 'E' or 'e' -> ramp Evap pump
  // 'A' or 'a' -> ramp AHU pump
  if (Serial.available() > 0) {
    char c = Serial.read();

    if (c == 'E' || c == 'e') {
      rampPump(PUMP_EVAP);
      Serial.println(F("EVAP ramp completed. Waiting for next command..."));
    } else if (c == 'A' || c == 'a') {
      rampPump(PUMP_AHU);
      Serial.println(F("AHU ramp completed. Waiting for next command..."));
    }
  }

  // Idle here until a command is received
}
