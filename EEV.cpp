#include "EEV.h"

// ================== CONSTRUCTOR ==================
EEV::EEV(const char* name, uint8_t stepPin, uint8_t dirPin, uint8_t enPin, int maxSteps)
: name(name), stepPin(stepPin), dirPin(dirPin), enPin(enPin), maxSteps(maxSteps), currentSteps(0) {
    pid.kp = 1.0; pid.ki = 0.01; pid.kd = 0.1;
    pid.integral = 0; pid.lastError = 0;
}

// ================== SETUP ==================
void EEV::begin() {
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(enPin, OUTPUT);
    digitalWrite(enPin, LOW); // enable driver
    home();
}

// ================== HOMING ==================
void EEV::home() {
    // drive closed for extra steps to guarantee zero
    stepMotor(false, maxSteps + 200);
    currentSteps = 0;
}

// ================== OPEN BY PERCENT ==================
void EEV::openPercent(float pct) {
    pct = constrain(pct, 0, 100);
    int targetSteps = (int)((pct/100.0) * maxSteps);
    stepTo(targetSteps);
}

// ================== STEP TO POSITION ==================
void EEV::stepTo(int targetSteps) {
    targetSteps = constrain(targetSteps, 0, maxSteps);
    int delta = targetSteps - currentSteps;

    if (delta == 0) return;

    bool dir = (delta > 0);
    stepMotor(dir, abs(delta));
    currentSteps = targetSteps;
}

// ================== LOW-LEVEL STEPPING ==================
void EEV::stepMotor(bool dir, int steps) {
    digitalWrite(dirPin, dir ? HIGH : LOW);
    for (int i=0; i<steps; i++) {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(500);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(500);
    }
}

// ================== PID CONTROL ==================
void EEV::updatePID(float setpoint, float processValue) {
    float error = setpoint - processValue;
    pid.integral += error;
    float derivative = error - pid.lastError;
    pid.lastError = error;

    float output = pid.kp * error + pid.ki * pid.integral + pid.kd * derivative;

    float pct = getPercentOpen() + output;
    openPercent(pct);
}

// ================== EMERGENCY FULL CLOSE ==================
void EEV::forceClose() {
    stepMotor(false, maxSteps + 200);
    currentSteps = 0;
}

// ================== NORMAL SHUTDOWN ==================
void EEV::shutdown() {
    forceClose();
}

// ================== STARTUP ==================
void EEV::startup() {
    home();
}

// ================== GET PERCENT OPEN ==================
float EEV::getPercentOpen() {
    return (100.0 * currentSteps) / maxSteps;
}

// ================== SET PID GAINS ==================
void EEV::setPID(float kp, float ki, float kd) {
    pid.kp = kp;
    pid.ki = ki;
    pid.kd = kd;
}
