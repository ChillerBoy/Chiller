#pragma once
#include <Arduino.h>
#include "AnalogOutputs.h"

// Simple start-up sequence: ramp tower fan, then pumps
class StartupSequencer {
public:
  StartupSequencer(AnalogOutputs& ao, uint8_t idxTowerFan, uint8_t idxAhuPump, uint8_t idxEvapPump)
  : _ao(ao), _iFan(idxTowerFan), _iAhu(idxAhuPump), _iEvap(idxEvapPump) {}

  void begin() { _state = Idle; _t0 = millis(); }
  void requestStart() { _wantRun = true; }
  void requestStop()  { _wantRun = false; }

  // returns true while running
  bool update() {
    switch (_state) {
      case Idle:
        if (_wantRun){ _state = Fan1; _t0 = millis(); }
        break;
      case Fan1:
        // spin tower fan to 35% for 3s
        cmdIndex(_iFan, 35);
        if (! _wantRun){ allOff(); _state = Idle; break; }
        if (elapsed() > 3000) { _state = Pumps; _t0 = millis(); }
        break;
      case Pumps:
        // bring both pumps to 40% for 3s
        cmdIndex(_iAhu, 40);
        cmdIndex(_iEvap, 40);
        if (! _wantRun){ allOff(); _state = Idle; break; }
        if (elapsed() > 3000) { _state = Running; _t0 = millis(); }
        break;
      case Running:
        if (! _wantRun){ allOff(); _state = Idle; }
        break;
    }
    return _state == Running;
  }

private:
  enum State { Idle, Fan1, Pumps, Running } _state{Idle};
  bool _wantRun{false};
  unsigned long _t0{0};
  AnalogOutputs& _ao;
  uint8_t _iFan, _iAhu, _iEvap;

  unsigned long elapsed() const { return millis() - _t0; }

  void cmdIndex(uint8_t idx, float pct){
    if (pct > 0.0f) pct = max(pct, 30.0f); // honor 30% min when non-zero
    _ao.setPercent(idx, pct);
  }
  void allOff(){
    _ao.setPercent(_iFan, 0);
    _ao.setPercent(_iAhu, 0);
    _ao.setPercent(_iEvap, 0);
  }
};
