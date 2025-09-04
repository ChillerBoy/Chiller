#ifndef REFTEMP_SENSORS_H
#define REFTEMP_SENSORS_H

#include "src/DS18B20_Sensors.h"

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

private:
  DS18B20Sensors& bus;
};
#endif
