
#include <Arduino.h>
#include <math.h>

// Externs that may be provided elsewhere:
extern float satDischargeF;
extern float tPostEconF();
extern void  broadcast(const char* msg);
extern float subcoolF;
extern float econSubcoolF;
extern float targetSubcoolMin;
extern float targetSubcoolMax;

// Provide a safe wrapper function instead of top-level code
static void economizer_debug_tick_internal() {
  if (!isnan(satDischargeF) && !isnan(tPostEconF())) {
    if (tPostEconF() < satDischargeF) {
      broadcast("Economizer lowering discharge temp");
    } else {
      broadcast("Economizer not reducing discharge temp");
    }
  }

  if (!isnan(subcoolF) && subcoolF >= targetSubcoolMin && subcoolF <= targetSubcoolMax) {
    // target satisfied without economizer
  } else {
    if (!isnan(econSubcoolF)) {
      if (econSubcoolF >= targetSubcoolMin && econSubcoolF <= targetSubcoolMax) {
        broadcast("Economizer achieving subcooling target");
      } else {
        broadcast("Economizer unable to meet subcooling target");
      }
    }
  }
}
