#ifndef REFTHERMO_H
#define REFTHERMO_H

#include <Arduino.h>

// R-410A saturation temperature [°F] from pressure [psig].
// Uses a compact lookup table with linear interpolation.
// Returns NAN if psig is out of table bounds.
float r410a_sat_tempF_from_psig(float psig);

#endif
