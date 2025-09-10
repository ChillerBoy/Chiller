#ifndef REFTEMP_SENSORS_H
#define REFTEMP_SENSORS_H

#include "DS18B20_Sensors.h"

class RefTempSensors {
public:
  RefTempSensors(DS18B20Sensors& shared) : bus(shared) {}
  void begin() {/* shared bus is begun externally */}
  void read() {/* batch read is done externally via bus.readAll() */}

  bool getSuction_F(float& f)        { return bus.getF(SUCTION_TEMP, f); }
  bool getDischarge_F(float& f)      { return bus.getF(DISCHARGE_TEMP, f); }
  bool getLiquidLine_F(float& f)     { return bus.getF(LIQUID_LINE_TEMP, f); }
  bool getPostDrier_F(float& f)      { return bus.getF(POST_FILTER_DRIER_TEMP, f); }
  bool getEconSuction_F(float& f)    { return bus.getF(ECON_SUCTION_TEMP, f); }
  bool getEcon2Phase_F(float& f)     { return bus.getF(ECON_2PHASE_TEMP, f); }
  bool getTwoPhase_F(float& f)       { return bus.getF(TWO_PHASE_TEMP, f); }
  bool getLiquidPostEcon_F(float& f) { return bus.getF(LIQUID_POST_ECON, f); }


  // Compatibility with legacy HMI.cpp bulk-send loops
  inline int getCount() const { return 8; }
  inline float getTempC(int i) const {
    float c = NAN; bool ok=false;
    switch(i){
      case 0: ok = bus.getC(SUCTION_TEMP, c); break;
      case 1: ok = bus.getC(DISCHARGE_TEMP, c); break;
      case 2: ok = bus.getC(LIQUID_LINE_TEMP, c); break;
      case 3: ok = bus.getC(POST_FILTER_DRIER_TEMP, c); break;
      case 4: ok = bus.getC(ECON_SUCTION_TEMP, c); break;
      case 5: ok = bus.getC(ECON_2PHASE_TEMP, c); break;
      case 6: ok = bus.getC(TWO_PHASE_TEMP, c); break;
      case 7: ok = bus.getC(LIQUID_POST_ECON, c); break;
      default: return NAN;
    }
    return ok ? c : NAN;
  }

private:
  DS18B20Sensors& bus;
};
#endif
