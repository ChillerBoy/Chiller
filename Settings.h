#pragma once
#include <Arduino.h>
#include <EEPROM.h>

struct SettingsBlob {
  uint8_t  version;
  uint8_t  reserved0;
  uint16_t crc;
  float sh_target_f;
  float sc_target_f;
  float lp_cutout_psig;
  float hp_warn_psig;
  float hp_trip_psig;
  float cond_ewt_target;
};

class Settings {
public:
  static constexpr uint8_t  kVersion    = 1;
  static constexpr int      kEepromAddr = 0;

  void defaults() {
    blob.version         = kVersion;
    blob.sh_target_f     = 12.0f;
    blob.sc_target_f     = 10.0f;
    blob.lp_cutout_psig  = 80.0f;
    blob.hp_warn_psig    = 350.0f;
    blob.hp_trip_psig    = 400.0f;
    blob.cond_ewt_target = 75.0f;
    updateCRC();
  }

  void begin() { if (!loadFromEEPROM()) { defaults(); saveToEEPROM(); } }
  bool loadFromEEPROM() {
    EEPROM.get(kEepromAddr, blob);
    if (blob.version != kVersion) return false;
    uint16_t expect = blob.crc; updateCRC(); return (expect == blob.crc);
  }
  void saveToEEPROM() { updateCRC(); EEPROM.put(kEepromAddr, blob); }

  SettingsBlob blob;

private:
  static uint16_t crc16(const uint8_t* d, size_t n) {
    uint16_t c=0xFFFF; for (size_t i=0;i<n;i++){ c^=(uint16_t)d[i]<<8;
      for(int b=0;b<8;b++) c = (c&0x8000)?(c<<1)^0x1021:(c<<1); } return c;
  }
  void updateCRC() { blob.crc = crc16((const uint8_t*)&blob + 4, sizeof(SettingsBlob) - 4); }
};
