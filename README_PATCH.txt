Chiller_Updated — DS18B20 Full Patch (2025-09-04)
==============================================
This patch mirrors your repo filenames and wires **all temperature reads** to a unified, fixed-address DS18B20 module.

Patched/Added files:
- src/TempSensorIDs.h
- src/DS18B20_Sensors.h
- src/DS18B20_Sensors.cpp
- WaterTempSensors.h / WaterTempSensors.cpp (now delegate to unified bus)
- RefTempSensors.h / RefTempSensors.cpp (now delegate to unified bus)
- SensorLabels.h (labels indexed by TempSensorID)
- register_map.h (stub helpers to bind Modbus/HMI to IDs)
- ChillerCode.ino (skeleton showing integration)

How to apply:
1) Copy these files over your repo (keep a backup of originals).
2) Ensure Arduino libs **OneWire** and **DallasTemperature** are installed.
3) If your OneWire pin is NOT 2, add `#define ONEWIRE_PIN <pin>` before including DS18B20 headers.
4) Build & upload. Check Serial Monitor for the live table.

Notes:
- If your existing `register_map.h` has a different structure, copy the helper ideas from this file into yours instead of replacing wholesale.
- Same for `HMI.cpp` / `HMI.h`: they typically read labels/values — with this patch, use TempSensorID indices consistently.
