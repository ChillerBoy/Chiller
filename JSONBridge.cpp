/*
  JSONBridge.cpp (v2)
*/
#include "JSONBridge.h"

namespace {
  Stream*  g_out      = &Serial;
  uint32_t g_interval = 1000;
  uint8_t  g_digits   = 1;
  uint32_t g_lastMs   = 0;
  JSONBridge::Frame g_frame;

  inline void printKV_float(Print& out, const __FlashStringHelper* k, float v, bool& first) {
    if (isnan(v)) return;
    if (!first) out.write(',');
    first = false;
    out.write('\"'); out.print(k); out.write('\"'); out.write(':');
    out.print(v, g_digits);
  }

  inline void printKV_bool(Print& out, const __FlashStringHelper* k, bool v, bool& first) {
    if (!first) out.write(',');
    first = false;
    out.write('\"'); out.print(k); out.write('\"'); out.write(':');
    out.print(v ? F("true") : F("false"));
  }

  inline void printKV_str(Print& out, const __FlashStringHelper* k, const String& s, bool& first) {
    if (s.length() == 0) return;
    if (!first) out.write(',');
    first = false;
    out.write('\"'); out.print(k); out.write('\"'); out.write(':');
    out.write('\"'); out.print(s); out.write('\"');
  }

  void emit(Print& out) {
    out.write('{');
    // Status
    out.print(F("\"Status\":{"));
    bool first = true;
    printKV_bool (out, F("Unit"), g_frame.unitEnabled, first);
    printKV_str  (out, F("Step"), g_frame.step, first);
    out.write('}');

    // CHW
    out.write(',');
    out.print(F("\"CHW\":{"));
    first = true;
    printKV_float(out, F("Supply"), g_frame.chwSupplyF, first);
    printKV_float(out, F("SP"),     g_frame.chwSPF,     first);
    out.write('}');

    // Temps
    out.write(',');
    out.print(F("\"Temp\":{"));
    first = true;
    printKV_float(out, F("Evap_EWT"),  g_frame.evapEWT_F,  first);
    printKV_float(out, F("Evap_LWT"),  g_frame.evapLWT_F,  first);
    printKV_float(out, F("AHU_Supply"),g_frame.ahuSupplyF, first);
    printKV_float(out, F("AHU_Return"),g_frame.ahuReturnF, first);
    out.write('}');

    // Pressures
    out.write(',');
    out.print(F("\"Press\":{"));
    first = true;
    printKV_float(out, F("Evap_Suction"), g_frame.evapSucPsi, first);
    printKV_float(out, F("Evap_Discharge"), g_frame.evapDisPsi, first);
    printKV_float(out, F("AHU_Suction"), g_frame.ahuSucPsi, first);
    printKV_float(out, F("AHU_Discharge"), g_frame.ahuDisPsi, first);
    out.write('}');

    // Flows
    out.write(',');
    out.print(F("\"Flow\":{"));
    first = true;
    printKV_float(out, F("Evap_GPM"), g_frame.evapGpm, first);
    printKV_float(out, F("AHU_GPM"),  g_frame.ahuGpm,  first);
    out.write('}');

    // Proofs
    out.write(',');
    out.print(F("\"Proof\":{"));
    first = true;
    printKV_bool(out, F("Evap_Flow"), g_frame.evapFlowProven, first);
    printKV_bool(out, F("AHU_Flow"),  g_frame.ahuFlowProven,  first);
    out.write('}');

    // Warn/Alarm
    out.write(',');
    out.print(F("\"Flags\":{"));
    first = true;
    printKV_bool(out, F("Warning"), g_frame.warningActive, first);
    printKV_bool(out, F("Alarm"),   g_frame.alarmActive,   first);
    out.write('}');

    // Commands
    out.write(',');
    out.print(F("\"Cmd\":{"));
    first = true;
    printKV_bool  (out, F("Evap_Pump"), g_frame.evapPumpOn, first);
    printKV_bool  (out, F("AHU_Pump"),  g_frame.ahuPumpOn,  first);
    printKV_float (out, F("Evap_Pct"),  g_frame.evCmdPct,   first);
    printKV_float (out, F("AHU_Pct"),   g_frame.ahCmdPct,   first);
    out.write('}');

    out.write('}');
    out.println();
  }
}

namespace JSONBridge {

  void begin(Stream* out) {
    if (out) g_out = out;
  }

  void setInterval(uint32_t ms) {
    g_interval = ms;
  }

  void setFloatDigits(uint8_t digits) {
    g_digits = digits;
  }

  void tick() {
    uint32_t now = millis();
    if (now - g_lastMs >= g_interval) {
      if (g_out) emit(*g_out);
      g_lastMs = now;
    }
  }

  void publishNow() {
    if (g_out) emit(*g_out);
    g_lastMs = millis();
  }

  // ----- Setters -----
  void setUnit(bool enabled, const char* step) {
    g_frame.unitEnabled = enabled;
    g_frame.step = step ? String(step) : String();
  }

  void setCHW(float supplyF, float setpointF) {
    g_frame.chwSupplyF = supplyF;
    g_frame.chwSPF     = setpointF;
  }

  void setTemps4(float a, float b, float c, float d) {
    g_frame.evapEWT_F  = a;
    g_frame.evapLWT_F  = b;
    g_frame.ahuSupplyF = c;
    g_frame.ahuReturnF = d;
  }

  void setTempsAll(const float* temps, size_t n) {
    if (!temps || n == 0) return;
    // Map to the first four as a convenience
    if (n > 0) g_frame.evapEWT_F  = temps[0];
    if (n > 1) g_frame.evapLWT_F  = temps[1];
    if (n > 2) g_frame.ahuSupplyF = temps[2];
    if (n > 3) g_frame.ahuReturnF = temps[3];
    // Also emit a generic Temp.Ti list for extras
    if (g_out && n > 4) {
      // Emit an additional compact line for overflow temps T5..Tn
      *g_out << F("{\"Temp\":{");
      for (size_t i = 4; i < n; ++i) {
        if (i > 4) g_out->write(',');
        g_out->write('\"'); g_out->print('T'); g_out->print(i+1); g_out->write('\"'); g_out->write(':');
        g_out->print(temps[i], g_digits);
      }
      *g_out << F("}}") << "\r\n";
    }
  }

  void setPress(float es, float ed, float as, float ad) {
    g_frame.evapSucPsi = es;
    g_frame.evapDisPsi = ed;
    g_frame.ahuSucPsi  = as;
    g_frame.ahuDisPsi  = ad;
  }

  void setFlow(float eg, float ag) {
    g_frame.evapGpm = eg;
    g_frame.ahuGpm  = ag;
  }

  void setProof(bool ep, bool ap) {
    g_frame.evapFlowProven = ep;
    g_frame.ahuFlowProven  = ap;
  }

  void setWarnAlarm(bool w, bool a) {
    g_frame.warningActive = w;
    g_frame.alarmActive   = a;
  }

  void setCommands(bool evOn, bool ahOn, int evPct, int ahPct) {
    g_frame.evapPumpOn = evOn;
    g_frame.ahuPumpOn  = ahOn;
    g_frame.evCmdPct   = constrain(evPct, 0, 100);
    g_frame.ahCmdPct   = constrain(ahPct, 0, 100);
  }

} // namespace JSONBridge
