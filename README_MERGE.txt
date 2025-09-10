Chiller Trainer — Full Bundle
--------------------------------
Drop all .h/.cpp into your project folder as tabs. Use ChillerCode_updated.ino as your main sketch (or copy bits into your existing one).

Key points:
- ECWT condenser fan PI controller via CondenserFanControl.*
- HMI register map centralized in register_map.h (fan block 0x1300..0x1303)
- EEPROM-backed setpoints in Settings.*
- Consistent labels via SensorLabels.h (replace free-typed strings)
- Optional helpers: StartupSequencer, HMI_events, Utils, ModbusMap_ACH580 (for later)
