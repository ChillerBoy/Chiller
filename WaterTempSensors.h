#ifndef WATER_TEMP_SENSORS_H
#define WATER_TEMP_SENSORS_H

#include "DS18B20_Sensors.h"

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


  // Compatibility with legacy HMI.cpp bulk-send loops
  inline int getCount() const { return 6; }
  inline float getTempC(int i) const {
    float c = NAN; bool ok=false;
    switch(i){
      case 0: ok = bus.getC(COND_EWT, c); break;
      case 1: ok = bus.getC(COND_LWT, c); break;
      case 2: ok = bus.getC(EVAP_EWT, c); break;
      case 3: ok = bus.getC(EVAP_LWT, c); break;
      case 4: ok = bus.getC(AHU_SUPPLY_TEMP, c); break;
      case 5: ok = bus.getC(AHU_RETURN_TEMP, c); break;
      default: return NAN;
    }
    return ok ? c : NAN;
  }

private:
  DS18B20Sensors& bus;
};
#endif
