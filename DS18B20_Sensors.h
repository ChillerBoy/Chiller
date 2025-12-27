#ifndef DS18B20_SENSORS_H
#define DS18B20_SENSORS_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "TempSensorIDs.h"

#ifndef ONEWIRE_PIN
#define ONEWIRE_PIN 2   // Default OneWire bus pin for your project
#endif

struct TempSensorDef {
  const char* name;
  uint8_t address[8];
};

class DS18B20Sensors {
public:
  DS18B20Sensors(uint8_t oneWirePin = ONEWIRE_PIN);

  void begin();
  uint8_t readAll();
  bool getF(TempSensorID id, float& outF) const;
  bool getC(TempSensorID id, float& outC) const;
  const char* nameOf(TempSensorID id) const;
  void printTable(Stream& s = Serial) const;

private:
  OneWire oneWire;
  DallasTemperature dallas;
  float lastC[TEMP_SENSOR_COUNT];
  bool ok[TEMP_SENSOR_COUNT];
  bool readOne(TempSensorID id);
};

#endif // DS18B20_SENSORS_H
