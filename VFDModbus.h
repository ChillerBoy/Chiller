#pragma once
#include <ModbusMaster.h>

class VFDModbus {
public:
  VFDModbus(HardwareSerial& port, uint8_t slaveId);

  void begin(long baud = 19200);
  bool writeControlWord(uint16_t cw);
  bool setSpeedPercent(float pct);  // pct = 0–100% → counts 0–20000
  bool readStatus(uint16_t& sw);
  bool readActual(uint16_t& act);

private:
  ModbusMaster node;
  HardwareSerial& serial;
  uint8_t id;
};
