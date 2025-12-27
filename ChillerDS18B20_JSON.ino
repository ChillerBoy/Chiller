\
/*
  ChillerDS18B20_JSON.ino
  Board: Arduino Mega 2560
  Serial: 115200 baud, JSON-only, newline-delimited
  OneWire bus pin: 2 (override by defining ONEWIRE_PIN before including DS18B20_Sensors.h)
  Publish interval: 1000 ms
*/

#include <Arduino.h>
#define ONEWIRE_PIN 2
#include "DS18B20_Sensors.h"

// Map enum IDs to JSON keys expected by Node-RED
struct MapItem { TempSensorID id; const char* key; };

static const MapItem MAP[] = {
  { COND_EWT,                "Cond_EWT" },
  { ECON_SUCTION_TEMP,       "Econ_Suct_Temp" },
  { AHU_SUPPLY_TEMP,         "AHU_Supply_Temp" },
  { EVAP_LWT,                "Evap_LWT" },
  { LIQUID_POST_ECON,        "Liquid_Post_Econ" },
  { COND_LWT,                "Cond_LWT" },
  { AHU_RETURN_TEMP,         "AHU_Return_Temp" },
  { SUCTION_TEMP,            "Suction_Temp" },
  { EVAP_EWT,                "Evap_EWT" },
  { DISCHARGE_TEMP,          "Discharge_Temp" },
  { POST_FILTER_DRIER_TEMP,  "Post_Filter_Drier_Temp" },
  { LIQUID_LINE_TEMP,        "Liquid_Line_Temp" },
  { ECON_2PHASE_TEMP,        "Econ_2Phase_Temp" },
  { TWO_PHASE_TEMP,          "Two_Phase_Temp" }
};

static const uint8_t MAP_LEN = sizeof(MAP) / sizeof(MAP[0]);
static const uint32_t PUBLISH_MS = 1000;

DS18B20Sensors sensors(ONEWIRE_PIN);
uint32_t lastMs = 0;

static inline void printJsonLine() {
  Serial.write('{');
  for (uint8_t i = 0; i < MAP_LEN; ++i) {
    float f = NAN;
    bool ok = sensors.getF(MAP[i].id, f);
    Serial.write('"');
    Serial.print(MAP[i].key);
    Serial.write('"');
    Serial.write(':');
    if (ok && isfinite(f)) {
      // Print with 3 decimals; Node-RED json node will coerce to number
      Serial.print(f, 3);
    } else {
      Serial.print(F("null"));
    }
    if (i + 1 < MAP_LEN) Serial.write(',');
  }
  Serial.write('}');
  Serial.write('\n');
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait for native USB, harmless on Mega */ }
  sensors.begin();
}

void loop() {
  const uint32_t now = millis();
  if ((uint32_t)(now - lastMs) >= PUBLISH_MS) {
    lastMs = now;
    // Trigger a non-blocking conversion and read cached results
    sensors.readAll();
    printJsonLine();   // JSON only, no table or extra prints
  }
}
