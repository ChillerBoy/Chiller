#ifndef WATER_TEMP_SENSORS_H
#define WATER_TEMP_SENSORS_H

#include "src/DS18B20_Sensors.h"

class WaterTempSensors {
public:
  WaterTempSensors(DS18B20Sensors& shared) : bus(shared) {}
  void begin() {/* shared bus is begun externally */}
  void read() {/* batch read is done externally via bus.readAll() */}

  bool getCondEWT_F(float& f){ return bus.getF(COND_EWT, f); }
  bool getCondLWT_F(float& f){ return bus.getF(COND_LWT, f); }
  bool getEvapEWT_F(float& f){ return bus.getF(EVAP_EWT, f); }
  bool getEvapLWT_F(float& f){ return bus.getF(EVAP_LWT, f); }
  bool getAHUSupply_F(float& f){ return bus.getF(AHU_SUPPLY_TEMP, f); }
  bool getAHUReturn_F(float& f){ return bus.getF(AHU_RETURN_TEMP, f); }

private:
  DS18B20Sensors& bus;
};
#endif
