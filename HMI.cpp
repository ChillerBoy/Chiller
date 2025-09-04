#include "HMI.h"
#include "WaterTempSensors.h"
#include "RefTempSensors.h"
#include "PressureSensors.h"
#include "RefPressureSensors.h"
#include "FlowSensors.h"
#include "DigitalInputs.h"
#include "DigitalOutputs.h"
#include "AnalogOutputs.h"

#include "register_map.h"   // <-- for HMIReg::BASE_ALARMS / BASE_WARNINGS

// ================== INTERNAL STORAGE ==================
static const int MAX_EVENTS = 50;
static HMIEvent events[MAX_EVENTS];
static int eventCount = 0;

// Auto-clear delay for warnings
static const uint32_t WARNING_CLEAR_DELAY = 15000; // 15 sec

// ================== FRAME ENCODER ==================
static void sendFrameRawU16(uint16_t addr, uint16_t u16) {
    uint8_t frame[8];
    frame[0] = 0xA5; frame[1] = 0x5A;
    frame[2] = 0x05;         // Length
    frame[3] = 0x82;         // Write register
    frame[4] = addr >> 8;
    frame[5] = addr & 0xFF;
    frame[6] = u16 >> 8;
    frame[7] = u16 & 0xFF;
    Serial.write(frame, 8);
}

// ================== HELPERS ==================
static void addEvent(uint16_t code, bool isAlarm, const char* msg) {
    if (eventCount < MAX_EVENTS) {
        HMIEvent& e = events[eventCount++];
        e.code = code;
        e.isAlarm = isAlarm;
        strncpy(e.message, msg, sizeof(e.message)-1);
        e.message[sizeof(e.message)-1] = '\0';
        e.timestamp = millis();
        e.active = true;
    }
}

// ================== API ==================
void HMI_begin(unsigned long baud) {
    Serial.begin(baud);
    eventCount = 0;
}

// Scaled write (compatible with your existing HMI_sendToHMI uses)
void HMI_sendToHMI(uint16_t address, float value) {
    // value is in engineering units; device expects int16 = value*10
    int16_t valx10 = (int16_t)(value * 10.0f);
    sendFrameRawU16(address, (uint16_t)valx10);
}

// NEW: exact raw writes (no scaling)
void HMI_writeRawU16(uint16_t address, uint16_t value) {
    sendFrameRawU16(address, value);
}
void HMI_writeRawI16(uint16_t address, int16_t value) {
    sendFrameRawU16(address, (uint16_t)value);
}

void HMI_sendEvent(uint16_t code, bool isAlarm) {
    // Default message text for common codes (kept from your version)
    const char* msg = "Unknown Event";
    switch(code) {
        case 100: msg = "Critical Safety Trip"; break;
        case 101: msg = "AHU Flow Loss"; break;
        case 102: msg = "Evap Flow Loss"; break;
        case 103: msg = "Tower Flow Loss"; break;
        case 104: msg = "High Discharge Pressure"; break;
        case 105: msg = "Low Suction Pressure"; break;
        case 111: msg = "EEV Driver Shutdown"; break;
        case 200: msg = "Discharge Pressure High Warn"; break;
        case 201: msg = "Superheat Drift Warn"; break;
        case 202: msg = "Subcooling Drift Warn"; break;
    }

    addEvent(code, isAlarm, msg);

    // Push to the correct base block with NO scaling
    if (isAlarm) {
        HMI_writeRawU16(HMIReg::BASE_ALARMS, code);
    } else {
        HMI_writeRawU16(HMIReg::BASE_WARNINGS, code);
    }
}

void HMI_update() {
    // Auto-clear warnings after timeout (kept)
    for (int i=0; i<eventCount; i++) {
        HMIEvent& e = events[i];
        if (!e.isAlarm && e.active) {
            if (millis() - e.timestamp > WARNING_CLEAR_DELAY) {
                e.active = false;
                // Optional: send cleared warning code to a "cleared" register
                // Using 0x2001 from your original; feel free to map this into register_map.h too.
                HMI_writeRawU16(0x2001, e.code);
            }
        }
    }
}

// ================== BULK SENSOR UPDATE ==================
void HMI_sendAll(
  WaterTempSensors& wts,
  RefTempSensors& rts,
  PressureSensors& wps,
  RefPressureSensors& rps,
  FlowSensors& fs,
  DigitalInputs& di,
  DigitalOutputs& dout,
  AnalogOutputs& ao
) {
    // Temperatures
    for (int i=0; i<wts.getCount(); i++) {
        HMI_sendToHMI(0x3000+i, wts.getTempC(i));
    }
    for (int i=0; i<rts.getCount(); i++) {
        HMI_sendToHMI(0x3100+i, rts.getTempC(i));
    }

    // Pressures
    for (int i=0; i<wps.getCount(); i++) {
        HMI_sendToHMI(0x3200+i, wps.getPressure(i));
    }
    for (int i=0; i<rps.getCount(); i++) {
        HMI_sendToHMI(0x3300+i, rps.getPressure(i));
    }

    // Flows
    for (int i=0; i<fs.getCount(); i++) {
        HMI_sendToHMI(0x3400+i, fs.getGPM(i));
    }

    // Digital I/O
    for (int i=0; i<di.getCount(); i++) {
        // exact 0/1
        HMI_writeRawU16(0x3500+i, di.getState(i) ? 1 : 0);
    }
    for (int i=0; i<dout.getCount(); i++) {
        HMI_writeRawU16(0x3600+i, dout.getState(i) ? 1 : 0);
    }

    // Analog outputs (percentages)
    for (int i=0; i<ao.getCount(); i++) {
        HMI_sendToHMI(0x3700+i, ao.getPercent(i));
    }
}
