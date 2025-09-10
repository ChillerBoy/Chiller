
#pragma once
#include <Arduino.h>

class EEV {
public:
  EEV(const char* name, uint8_t stepPin, uint8_t dirPin, uint8_t enPin, int maxSteps);
  void begin();
  void home();
  void openPercent(float pct);
  void stepTo(int targetSteps);
  void stepMotor(bool dir, int steps);
  void updatePID(float setpoint, float processValue);
  void forceClose();
  void shutdown();
  void startup();
  float getPercentOpen();
  void setPID(float kp, float ki, float kd);

private:
  const char* name;
  uint8_t stepPin, dirPin, enPin;
  int maxSteps;
  int currentSteps;
  struct {
    float kp, ki, kd;
    float integral;
    float lastError;
  } pid;
};
