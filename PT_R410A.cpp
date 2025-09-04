#include "PT_R410A.h"

// Example lookup table (simplified, expand as needed)
const uint8_t PT_R410A::tableSize = 6;
const float PT_R410A::pressures[] = { 100, 150, 200, 250, 300, 350 };
const float PT_R410A::temps[]     = { 40.5, 55.3, 68.0, 78.5, 87.9, 96.0 };

float PT_R410A::pressureToTemp(float psig) {
  if (psig <= pressures[0]) return temps[0];
  if (psig >= pressures[tableSize-1]) return temps[tableSize-1];

  for (uint8_t i = 0; i < tableSize - 1; i++) {
    if (psig >= pressures[i] && psig <= pressures[i+1]) {
      float t = temps[i] + 
                (temps[i+1] - temps[i]) * 
                ((psig - pressures[i]) / (pressures[i+1] - pressures[i]));
      return t;
    }
  }
  return NAN;
}
