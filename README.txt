Chiller Control Package (Baseline)
================================

This zip provides:
- A monolithic Arduino Mega 2560 sketch implementing the SOO + alarms + training mode (baseline)
- A tabbed PySide6 HMI that mirrors the system-by-system layout
- Documentation: unified SOO + patch guide
- All other previously shared .ino files are included as reference only.

Upload steps:
1) Arduino:
   - Open arduino/Chiller_Main_Code/Chiller_Main_Code.ino in Arduino IDE
   - Ensure DS18B20_Sensors.* and TempSensorIDs.h are in same folder (they are)
   - Select Board: Mega 2560, Baud 115200
   - Upload
2) Python HMI:
   - Go to python_hmi/
   - pip install -r requirements.txt
   - Edit SERIAL_PORT in main_hmi.py
   - python main_hmi.py

Tuning:
- Default setpoint: 44°F, deadband: 2°F
- Flow min GPM and pressure limits are placeholders and must be tuned.

