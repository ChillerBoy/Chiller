
#pragma once
// Auto-generated alarms interface  2025-09-01
#include <stdint.h>

enum class AlarmType : uint8_t { WARNING=0, ALARM=1 };
enum class Priority : uint8_t { Low=0, Medium=1, High=2, Critical=3 };

struct AlarmDef {
  const char* code;         // e.g., "HIGH_DISCHARGE_PRESSURE"
  const char* name;         // human readable
  AlarmType type;           // warning or alarm
  Priority priority;        // default severity
  bool latched;             // alarms latch until ACK+clear
  bool autoClear;           // warnings auto-clear when condition ends
  uint16_t debounce_s;      // time condition must persist to trigger
  uint16_t clear_s;         // time condition must clear to auto-reset (warnings)
  uint16_t min_on_s;        // inhibit re-fire inside this window
  const char* source;       // signal name
  const char* op;           // "gt","lt","eq","band"
  float threshold;          // threshold value; band uses system setpoints
};

struct ActiveAlarm {
  const AlarmDef* def;
  uint32_t first_ts_ms;
  uint32_t last_ts_ms;
  bool acknowledged;
  bool active;
};

// Registry (defined in alarms.cpp)
extern const AlarmDef ALARM_TABLE[];
extern const uint16_t ALARM_TABLE_SIZE;

// API
void alarms_init();
void alarms_tick(uint32_t now_ms);              // evaluate alarms, manage timers
void alarms_ack_all();                           // acknowledge latched alarms
bool alarms_any_active();
bool alarms_any_trip();                          // any ALARM (not warning) active
uint16_t alarms_active_count();
const ActiveAlarm* alarms_get(uint16_t i);

// Hooks (to be implemented in your codebase)
float read_signal(const char* name);             // e.g., "DischargePressure"
bool  read_flag(const char* name);               // e.g., "VFDFault"
void  on_alarm_trip(const AlarmDef* def);        // e.g., stop compressor
void  on_warning(const AlarmDef* def);           // e.g., HMI banner
void  log_event(const char* code, const char* msg);
