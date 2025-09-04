#pragma once
#include <Arduino.h>

class CondenserFanControl {
public:
  void configure(float kp, float ki, float kd, float outMin = 0.0f, float outMax = 100.0f) {
    _kp = kp; _ki = ki; _kd = kd; _outMin = outMin; _outMax = outMax;
  }
  void setNonzeroBiasPercent(float pct) { _minNonzero = constrain(pct, 0.0f, 100.0f); }
  void setRampLimits(float upPctPerSec, float downPctPerSec) {
    _rampUp = max(0.0f, upPctPerSec); _rampDn = max(0.0f, downPctPerSec);
  }
  void reset() { _integral=0; _prevErr=0; _prevOut=0; _lastMs=millis(); }

  float update(float setpointF, float measuredF) {
    unsigned long now = millis();
    float dt = (now - _lastMs) * 0.001f;
    if (dt <= 0.0f || dt > 5.0f) { _lastMs = now; return _prevOut; }
    _lastMs = now;
    float err = measuredF - setpointF;       // positive -> need more fan
    _integral += err * dt;
    float iMax = 100.0f / max(0.001f, _ki);
    _integral = constrain(_integral, -iMax, iMax);
    float deriv = (err - _prevErr) / dt;
    float out = _kp*err + _ki*_integral + _kd*deriv;
    out = constrain(out, _outMin, _outMax);
    float up = _rampUp*dt, dn=_rampDn*dt;
    if (out > _prevOut) out = min(out, _prevOut + up);
    else                out = max(out, _prevOut - dn);
    if (out > 0.0f) out = max(out, _minNonzero);
    _prevErr = err; _prevOut = out; return out;
  }

  float outputPercent() const { return _prevOut; }
  float lastError() const { return _prevErr; }

private:
  float _kp{0.7f}, _ki{0.05f}, _kd{0.0f};
  float _integral{0}, _prevErr{0}, _prevOut{0};
  float _outMin{0}, _outMax{100};
  float _minNonzero{30};
  float _rampUp{20}, _rampDn{30};
  unsigned long _lastMs{0};
};
