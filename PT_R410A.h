#ifndef PT_R410A_H
#define PT_R410A_H

#include <Arduino.h>

class PT_R410A {
public:
  static float pressureToTemp(float psig); // return saturation temp (°F)

private:
  static const uint8_t tableSize;
  static const float pressures[]; // psig
  static const float temps[];     // °F
};

#endif
