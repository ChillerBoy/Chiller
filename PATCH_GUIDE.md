PATCH GUIDE (Monolithic Arduino Code)
===================================

You requested that the main sketch contains ALL live logic, and other files are reference-only.
This package is structured that way.

Arduino sketch location:
  arduino/Chiller_Main_Code/Chiller_Main_Code.ino

Reference-only files (NOT compiled):
  arduino/reference/*

How to patch (recommended):
- Open Chiller_Main_Code.ino and use CTRL+F to jump to PATCH_POINT_* blocks.
- Line numbers below are for this package version; they may change if you edit the file.

Key patch locations (approx):
- Pin map (pumps/relays/flow/VFD IO): line 43
- Pressure sensor pin map + ranges:   line 74
- Flow meter K-factor + pins:         line 92
- Setpoints/thresholds/timers:        line 99
- VFD ramp/min/max defaults:          line 121

Important Arduino IDE note:
- Arduino compiles all .ino files in the same sketch folder.
- That is why all other .ino files were placed in arduino/reference so they won't compile.

If you later want to merge this into an existing sketch:
- Copy everything from Chiller_Main_Code.ino into your existing main file, OR
- Replace your main file with this one.


## HMI Manual Control & Calibration Commands (Serial)
The Python HMI now sends plain-text commands (one per line). Arduino accepts:

### Mode
- `MODE AUTO`
- `MODE SERVICE`  (enables manual overrides)

### Reset
- `RESET`

### Pumps / outputs (SERVICE mode)
- `PUMP EVAP ON|OFF`
- `PUMP COND ON|OFF`
- `PUMP CT ON|OFF`
- `ALARM_BELL ON|OFF`

### VFD / Compressor (SERVICE mode)
- `VFD RUN ON|OFF`
- `VFD SPEED <0-100>`  (percent)

### Cooling tower fan (SERVICE mode; JSON-only unless wired)
- `TOWER FAN <0-100>`

### EEV (SERVICE mode; simulated unless tied to driver)
- `EEV EVAP <0-100>`
- `EEV ECON <0-100>`

### Setpoints
- `SP LWT <degF>`
- `SP LWT_DB <degF>`

### Calibration offsets
- `CAL TEMP EVAP_LWT <degF>`
- `CAL TEMP EVAP_EWT <degF>`
- `CAL TEMP COND_LWT <degF>`
- `CAL TEMP COND_EWT <degF>`
- `CAL PRESS <PRESS_KEY> <psig>`  (PRESS_KEY matches the JSON key, e.g. `P_SUCTION`)
- `CAL FLOW AHU|EVAP|COND <gpm>`

Notes:
- Manual overrides are only applied when `STAT_ServiceMode=true`.
- Hard trips (flow loss, high head, low suction, VFD fault) still stop the unit.
