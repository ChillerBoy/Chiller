#include "RefThermo.h"

// Table derived from published R-410A PT charts (psig ↔ °F).
// Selected points cover typical HVAC ranges for evap/condensing.
// Sources: BPA, ES-Refrigerants, Hudson (R-410A PT charts).
// psig:  114,  140,  160,  180,  200,  216,  240,  260,  280,  300,  318,  340,  360,  380,  400,  420,  440,  460,  480,  500
// tempF: 37.8, 47.0, 54.0, 61.0, 68.0, 74.3, 82.0, 88.0, 93.5, 97.5, 100.2,104.0,108.0,112.0,116.0,120.7,124.0,127.0,130.0,134.0
static const float P_PSIG[] = {
  114, 140, 160, 180, 200, 216, 240, 260, 280, 300, 318, 340, 360, 380, 400, 420, 440, 460, 480, 500
};
static const float T_F[] = {
  37.8, 47.0, 54.0, 61.0, 68.0, 74.3, 82.0, 88.0, 93.5, 97.5, 100.2,104.0,108.0,112.0,116.0,120.7,124.0,127.0,130.0,134.0
};
static const size_t N = sizeof(P_PSIG)/sizeof(P_PSIG[0]);

float r410a_sat_tempF_from_psig(float psig) {
  if (psig < P_PSIG[0] || psig > P_PSIG[N-1]) return NAN;
  for (size_t i = 1; i < N; ++i) {
    if (psig <= P_PSIG[i]) {
      float p0 = P_PSIG[i-1], p1 = P_PSIG[i];
      float t0 = T_F[i-1],    t1 = T_F[i];
      float f  = (psig - p0) / (p1 - p0);
      return t0 + f * (t1 - t0);
    }
  }
  return T_F[N-1];
}
