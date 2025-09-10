
#include <Arduino.h>
#include "io_pins.h"
#include "ds18b20_map.h"
#include "soo_config.h"
#include "alarms.h"
#include "signals_bridge.h"

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature dallas(&oneWire);

unsigned long lastTick = 0;

void setupPins(){
  pinMode(DI_ALARM_SILENCE, INPUT_PULLUP);
  pinMode(DI_ENABLE_OFF, INPUT_PULLUP);
  pinMode(DI_TOWER_FLOW_SWITCH, INPUT_PULLUP);
  pinMode(DI_AHU_FLOW_SWITCH, INPUT_PULLUP);
  pinMode(DI_EVAP_FLOW_SWITCH, INPUT_PULLUP);
  pinMode(DI_RIGHT_FAN_CONTACTOR_PROOF, INPUT_PULLUP);
  pinMode(DI_AHU_PUMP_CONTACTOR_PROOF, INPUT_PULLUP);
  pinMode(DI_EVAP_PUMP_CONTACTOR_PROOF, INPUT_PULLUP);
  pinMode(DI_LOWER_FAN_CONTACTOR_PROOF, INPUT_PULLUP);
  pinMode(DI_LEFT_FAN_CONTACTOR_PROOF, INPUT_PULLUP);
  pinMode(DI_CTOWER_PUMP_CONTACTOR_PROOF, INPUT_PULLUP);

  pinMode(DO_AHU_PUMP, OUTPUT);
  pinMode(DO_EVAP_PUMP, OUTPUT);
  pinMode(DO_SAND_FILTER_PUMP, OUTPUT);
  pinMode(DO_CTOWER_PUMP, OUTPUT);
  pinMode(DO_LOWER_FAN_CONTACTOR, OUTPUT);
  pinMode(DO_LEFT_FAN_CONTACTOR, OUTPUT);
  pinMode(DO_RIGHT_FAN_CONTACTOR, OUTPUT);
  pinMode(DO_CTOWER_FAN_ENABLE, OUTPUT);
  pinMode(DO_RED_LIGHT, OUTPUT);
  pinMode(DO_ORANGE_LIGHT, OUTPUT);
  pinMode(DO_GREEN_LIGHT, OUTPUT);
  pinMode(DO_ALARM_BELL, OUTPUT);

  digitalWrite(DO_AHU_PUMP, LOW);
  digitalWrite(DO_EVAP_PUMP, LOW);
  digitalWrite(DO_SAND_FILTER_PUMP, LOW);
  digitalWrite(DO_CTOWER_PUMP, LOW);
  digitalWrite(DO_LOWER_FAN_CONTACTOR, LOW);
  digitalWrite(DO_LEFT_FAN_CONTACTOR, LOW);
  digitalWrite(DO_RIGHT_FAN_CONTACTOR, LOW);
  digitalWrite(DO_CTOWER_FAN_ENABLE, LOW);
  digitalWrite(DO_RED_LIGHT, LOW);
  digitalWrite(DO_ORANGE_LIGHT, LOW);
  digitalWrite(DO_GREEN_LIGHT, LOW);
  digitalWrite(DO_ALARM_BELL, LOW);
}

static float analog_to_psi(int raw, float psi_fullscale=500.0f){
  return (raw * (psi_fullscale/1023.0f));
}

void setup() {
  Serial.begin(115200);
  setupPins();
  dallas.begin();
  signals_init();
  alarms_init();
  Serial.println(F("ChillerCode booted."));
}

void read_all_sensors(){
  // Example scaling — replace with your specific sensor ranges
  float p_suct = analog_to_psi(analogRead(AI_SUCTION_PRESS));
  float p_dis  = analog_to_psi(analogRead(AI_COMP_DISCH_PRESS));
  (void)p_suct; (void)p_dis;

  dallas.requestTemperatures();
  // Example: you'd look up each address by name when needed.
}

void loop() {
  unsigned long now = millis();
  if (now - lastTick >= 200){
    lastTick = now;
    read_all_sensors();
    alarms_tick(now);
    // TODO: implement state machine per SOO
  }
}
