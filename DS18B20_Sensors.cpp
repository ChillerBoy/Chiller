#include "DS18B20_Sensors.h"

static const TempSensorDef TEMP_DEFS[TEMP_SENSOR_COUNT] = {
  { "Cond EWT",                {0x28,0x20,0x46,0x53,0x00,0x00,0x00,0x8D} }, // 0
  { "Economizer Suction Temp", {0x28,0xF0,0x21,0x37,0x00,0x00,0x00,0x97} }, // 1
  { "AHU Supply Temp",         {0x28,0xF0,0x3D,0x36,0x00,0x00,0x00,0x45} }, // 2
  { "Evap LWT",                {0x28,0x48,0x84,0x51,0x00,0x00,0x00,0x49} }, // 3
  { "Liquid Temp Post Econ",   {0x28,0x38,0x1B,0x53,0x00,0x00,0x00,0x0E} }, // 4
  { "Cond LWT",                {0x28,0xEC,0x6B,0x51,0x00,0x00,0x00,0x7C} }, // 5
  { "AHU Return Temp",         {0x28,0x4A,0x76,0x37,0x00,0x00,0x00,0x9B} }, // 6
  { "Suction Temp",            {0x28,0xE6,0x22,0x53,0x00,0x00,0x00,0xAE} }, // 7
  { "Evap EWT",                {0x28,0x56,0x5C,0x54,0x00,0x00,0x00,0xE0} }, // 8
  { "Discharge Temp",          {0x28,0x56,0xDE,0x53,0x00,0x00,0x00,0x2E} }, // 9
  { "Post Filter Drier Temp",  {0x28,0xFE,0x82,0x57,0x00,0x00,0x00,0x69} }, // 10
  { "Liquid Line Temp",        {0x28,0x89,0x90,0x55,0x00,0x00,0x00,0x8C} }, // 11
  { "Economizer 2 Phase Temp", {0x28,0x85,0x67,0x36,0x00,0x00,0x00,0x1E} }, // 12
  { "Two Phase Temp",          {0x28,0x15,0x28,0x50,0x00,0x00,0x00,0xB0} }  // 13
};

DS18B20Sensors::DS18B20Sensors(uint8_t oneWirePin)
: oneWire(oneWirePin), dallas(&oneWire) {
  for (uint8_t i = 0; i < TEMP_SENSOR_COUNT; ++i) {
    lastC[i] = NAN;
    ok[i] = false;
  }
}

void DS18B20Sensors::begin() {
  dallas.begin();
  dallas.setResolution(12);
  dallas.setWaitForConversion(false);
}

uint8_t DS18B20Sensors::readAll() {
  dallas.requestTemperatures();
  delay(750);
  uint8_t good = 0;
  for (uint8_t i = 0; i < TEMP_SENSOR_COUNT; ++i) {
    if (readOne(static_cast<TempSensorID>(i))) good++;
  }
  return good;
}

bool DS18B20Sensors::readOne(TempSensorID id) {
  float c = dallas.getTempC(TEMP_DEFS[id].address);
  if (c == DEVICE_DISCONNECTED_C) {
    ok[id] = false;
    lastC[id] = NAN;
    return false;
  }
  ok[id] = true;
  lastC[id] = c;
  return true;
}

bool DS18B20Sensors::getF(TempSensorID id, float& outF) const {
  if (!ok[id]) return false;
  outF = lastC[id] * 9.0f / 5.0f + 32.0f;
  return true;
}

bool DS18B20Sensors::getC(TempSensorID id, float& outC) const {
  if (!ok[id]) return false;
  outC = lastC[id];
  return true;
}

const char* DS18B20Sensors::nameOf(TempSensorID id) const {
  return TEMP_DEFS[id].name;
}

void DS18B20Sensors::printTable(Stream& s) const {
  s.println(F("Idx  Name                          (C)      (F)     OK  ROM"));
  for (uint8_t i = 0; i < TEMP_SENSOR_COUNT; ++i) {
    float c = lastC[i];
    float f = c * 9.0f / 5.0f + 32.0f;
    s.print(i); s.print(F("   "));
    s.print(TEMP_DEFS[i].name); s.print(F("  "));
    if (ok[i]) {
      s.print(c, 3); s.print(F("  ")); s.print(f, 3); s.print(F("   1   "));
    } else {
      s.print(F("NaN     NaN     0   "));
    }
    const uint8_t* a = TEMP_DEFS[i].address;
    for (uint8_t b = 0; b < 8; ++b) {
      if (b) s.print(':');
      if (a[b] < 16) s.print('0');
      s.print(a[b], HEX);
    }
    s.println();
  }
}
