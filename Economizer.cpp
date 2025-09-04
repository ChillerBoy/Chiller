// Economizer subcooling = cond sat temp - post-econ temp
if (!isnan(satDischargeF) && !isnan(tPostEconF())) {
  econSubcoolF = satDischargeF - tPostEconF();
} else {
  econSubcoolF = NAN;
}

// Decision logic
if (!isnan(subcoolF) && subcoolF >= targetSubcoolMin && subcoolF <= targetSubcoolMax) {
  // Liquid line alone meets target → economizer not needed
} else {
  // Use economizer value for optimization
  if (!isnan(econSubcoolF)) {
    if (econSubcoolF >= targetSubcoolMin && econSubcoolF <= targetSubcoolMax) {
      broadcast("Economizer achieving subcooling target");
    } else {
      broadcast("Economizer unable to meet subcooling target");
    }
  }
}
