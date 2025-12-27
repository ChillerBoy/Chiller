# ChillerDS18B20_JSON (Arduino Mega 2560)

- **Baud:** 115200
- **Format:** newline-delimited JSON (one object per line), keys match your Node-RED flow
- **Interval:** 1000 ms
- **OneWire pin:** 2 (change by defining `ONEWIRE_PIN` before `#include "DS18B20_Sensors.h"`)

## Files
- `ChillerDS18B20_JSON.ino` — main sketch
- `DS18B20_Sensors.h/.cpp` — your helper (unmodified)
- `TempSensorIDs.h` — enum IDs used by the helper

## Node-RED
Import your existing `flows.json`. Point the **Serial in** node to the Arduino port at 115200. Ensure the **`json`** node is set to parse `msg.payload` as JSON per line.
