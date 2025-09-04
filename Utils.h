#pragma once
#include <Arduino.h>
struct RateLimiter {
  float rateUp{100}, rateDn{100};
  float prev{0}; unsigned long last{0};
  void reset(float v=0){ prev=v; last=millis(); }
  float step(float target){
    unsigned long now = millis(); float dt=(now-last)*0.001f; last=now;
    float up=rateUp*dt, dn=rateDn*dt, out=target;
    if (out>prev) out=min(out, prev+up); else out=max(out, prev-dn);
    prev=out; return out;
  }
};
template<int N>
struct MovingAvg {
  float buf[N]{0}; int i{0}; int filled{0};
  float add(float v){ buf[i]=v; i=(i+1)%N; if (filled<N) filled++; float s=0; for(int k=0;k<filled;k++) s+=buf[k]; return s/filled; }
};
