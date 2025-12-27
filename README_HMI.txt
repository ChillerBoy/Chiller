Chiller Tabbed HMI (PySide6)
=======================

1) Install:
   pip install -r requirements.txt

2) Edit SERIAL_PORT in main_hmi.py (top of file):
   - Windows: COM3, COM4, etc.
   - Raspberry Pi / Linux: /dev/ttyACM0 or /dev/ttyUSB0

3) Run:
   python main_hmi.py

The HMI expects newline-delimited JSON from the Arduino.
It can also send simple serial text commands:
- RESET
- SP LWT 44.0
- SP DB 2.0
- SIM ON / SIM OFF
- SIM FAULT <name>
