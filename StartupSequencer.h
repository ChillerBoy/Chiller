#pragma once
#include <Arduino.h>
#include "config_pins.h"
#include "AnalogOutputs.h"
#include "SensorLabels.h"

class StartupSequencer {
public:
  void begin() { _state = Idle; _t0 = millis(); }
  void requestStart() { _wantRun = true; }
  void requestStop()  { _wantRun = false; }

  bool update() {
    switch (_state) {
      case Idle:
        if (_wantRun){ _state = Fan1; _t0 = millis(); }
        break;
      case Fan1:
        command(Label::AO_TOWER_FAN, 35);
        if (elapsed() > START_STAGE_DELAY_MS) { _state = Pumps; _t0 = millis(); }
        break;
      case Pumps:
        command(Label::AO_AHU_PUMP,  40);
        command(Label::AO_EVAP_PUMP, 40);
        if (elapsed() > START_STAGE_DELAY_MS) { _state = Running; }
        break;
      case Running:
        if (!_wantRun) { command(Label::AO_TOWER_FAN,0); command(Label::AO_AHU_PUMP,0); command(Label::AO_EVAP_PUMP,0); _state = Idle; }
        break;
    }
    return _state == Running;
  }

private:
  enum State { Idle, Fan1, Pumps, Running } _state{Idle};
  bool _wantRun{false};
  unsigned long _t0{0};
  unsigned long elapsed() const { return millis() - _t0; }
  void command(const char* name, float pct) {
    if (pct > 0.0f) pct = max(pct, 30.0f);
    analogOutputs.setByName(name, pct);
  }
};
