// ChillerCode.ino — DS18B20 unified patch (2025-09-04)
#include <OneWire.h>
#include <DallasTemperature.h>
#include "src/TempSensorIDs.h"
#include "src/DS18B20_Sensors.h"
#include "WaterTempSensors.h"
#include "RefTempSensors.h"
#include "SensorLabels.h"
#include "register_map.h"

DS18B20Sensors dsBus;         // default ONEWIRE_PIN=2 (override with #define ONEWIRE_PIN <pin> before includes)
WaterTempSensors water(dsBus);
RefTempSensors   refTemps(dsBus);

unsigned long lastMs = 0;

struct TVal { float v; bool ok; };
TVal t[ TEMP_SENSOR_COUNT ];

void setup(){
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Starting Chiller with DS18B20 unified map..."));
  dsBus.begin();
}

static void sampleTemps(){
  dsBus.readAll();
  float f;
  // Water side
  t[COND_EWT].ok = dsBus.getF(COND_EWT, f); t[COND_EWT].v = f;
  t[COND_LWT].ok = dsBus.getF(COND_LWT, f); t[COND_LWT].v = f;
  t[EVAP_EWT].ok = dsBus.getF(EVAP_EWT, f); t[EVAP_EWT].v = f;
  t[EVAP_LWT].ok = dsBus.getF(EVAP_LWT, f); t[EVAP_LWT].v = f;
  t[AHU_SUPPLY_TEMP].ok = dsBus.getF(AHU_SUPPLY_TEMP, f); t[AHU_SUPPLY_TEMP].v = f;
  t[AHU_RETURN_TEMP].ok = dsBus.getF(AHU_RETURN_TEMP, f); t[AHU_RETURN_TEMP].v = f;
  // Refrigeration side
  t[SUCTION_TEMP].ok = dsBus.getF(SUCTION_TEMP, f); t[SUCTION_TEMP].v = f;
  t[DISCHARGE_TEMP].ok = dsBus.getF(DISCHARGE_TEMP, f); t[DISCHARGE_TEMP].v = f;
  t[LIQUID_LINE_TEMP].ok = dsBus.getF(LIQUID_LINE_TEMP, f); t[LIQUID_LINE_TEMP].v = f;
  t[POST_FILTER_DRIER_TEMP].ok = dsBus.getF(POST_FILTER_DRIER_TEMP, f); t[POST_FILTER_DRIER_TEMP].v = f;
  t[ECON_SUCTION_TEMP].ok = dsBus.getF(ECON_SUCTION_TEMP, f); t[ECON_SUCTION_TEMP].v = f;
  t[ECON_2PHASE_TEMP].ok = dsBus.getF(ECON_2PHASE_TEMP, f); t[ECON_2PHASE_TEMP].v = f;
  t[TWO_PHASE_TEMP].ok = dsBus.getF(TWO_PHASE_TEMP, f); t[TWO_PHASE_TEMP].v = f;
  t[LIQUID_POST_ECON].ok = dsBus.getF(LIQUID_POST_ECON, f); t[LIQUID_POST_ECON].v = f;
}

static void printTemps(){
  Serial.println(F("=== Temps (F) ==="));
  for(uint8_t i=0;i<TEMP_SENSOR_COUNT;i++){
    if(t[i].ok){ Serial.print(i); Serial.print(F(" ")); Serial.print(TEMP_SENSOR_LABELS[i]); Serial.print(F(": "));
      Serial.println(t[i].v,1);
    }
  }
}

void loop(){
  unsigned long now = millis();
  if(now - lastMs >= 1000){
    lastMs = now;
    sampleTemps();
    printTemps();
  }
}
