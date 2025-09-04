#include "VFDModbus.h"
static const uint16_t REG_CW  = 0;  // 400001
static const uint16_t REG_REF1= 1;  // 400002
static const uint16_t REG_SW  = 3;  // 400004
static const uint16_t REG_ACT = 4;  // 400005

VFDModbus::VFDModbus(HardwareSerial& port, uint8_t slaveId)
: serial(port), id(slaveId) {}

void VFDModbus::begin(long baud) {
  serial.begin(baud, SERIAL_8N2);
  node.begin(id, serial);
}

bool VFDModbus::writeControlWord(uint16_t cw) {
  return node.writeSingleRegister(REG_CW, cw) == node.ku8MBSuccess;
}

bool VFDModbus::setSpeedPercent(float pct) {
  pct = constrain(pct, 0, 100);
  uint16_t val = (uint16_t)(pct * 200.0f); // Scale 0–100% → 0–20000
  return node.writeSingleRegister(REG_REF1, val) == node.ku8MBSuccess;
}

bool VFDModbus::readStatus(uint16_t& sw) {
  if(node.readHoldingRegisters(REG_SW,1)==node.ku8MBSuccess) {
    sw = node.getResponseBuffer(0);
    return true;
  }
  return false;
}

bool VFDModbus::readActual(uint16_t& act) {
  if(node.readHoldingRegisters(REG_ACT,1)==node.ku8MBSuccess) {
    act = node.getResponseBuffer(0);
    return true;
  }
  return false;
}
