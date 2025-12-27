/*
  EEV_SBT0811.ino  (with status output)
  Drives a 5‑wire unipolar EEV stepper via ZC‑A0591 / ULN2003 driver.
  Arduino Mega pins:
      D4→IN1, D5→IN2, D6→IN3, D7→IN4
*/

const uint8_t PIN_IN1 = 4;
const uint8_t PIN_IN2 = 5;
const uint8_t PIN_IN3 = 6;
const uint8_t PIN_IN4 = 7;

volatile unsigned int stepDelayMs = 1;
const unsigned long RUN_TIME_MS = 10000;
const unsigned int COIL_REST_MS = 200;

const uint8_t STEP_SEQ[4][4] = {
  {1,0,0,0},
  {0,1,0,0},
  {0,0,1,0},
  {0,0,0,1}
};

volatile int indexPos = 0;
bool direction = true;

void coilWrite(uint8_t a, uint8_t b, uint8_t c, uint8_t d){
  digitalWrite(PIN_IN1,a);
  digitalWrite(PIN_IN2,b);
  digitalWrite(PIN_IN3,c);
  digitalWrite(PIN_IN4,d);
}

void coilsOff(){
  coilWrite(0,0,0,0);
}

void stepForward(){
  indexPos = (indexPos + 1) & 0x03;
  const uint8_t* s = STEP_SEQ[indexPos];
  coilWrite(s[0],s[1],s[2],s[3]);
}

void stepReverse(){
  indexPos = (indexPos - 1) & 0x03;
  const uint8_t* s = STEP_SEQ[indexPos];
  coilWrite(s[0],s[1],s[2],s[3]);
}

void printStatus(){
  Serial.print("Status: ");
  Serial.print(direction ? "OPENING" : "CLOSING");
  Serial.print(" | IN1="); Serial.print(digitalRead(PIN_IN1));
  Serial.print(" IN2=");   Serial.print(digitalRead(PIN_IN2));
  Serial.print(" IN3=");   Serial.print(digitalRead(PIN_IN3));
  Serial.print(" IN4=");   Serial.println(digitalRead(PIN_IN4));
}

void timedRun(void (*stepFn)()){
  unsigned long start = millis();
  while(millis() - start < RUN_TIME_MS){
    stepFn();
    printStatus();
    delay(stepDelayMs);
  }
  coilsOff();
  delay(COIL_REST_MS);
}

void setup(){
  Serial.begin(115200);
  pinMode(PIN_IN1,OUTPUT);
  pinMode(PIN_IN2,OUTPUT);
  pinMode(PIN_IN3,OUTPUT);
  pinMode(PIN_IN4,OUTPUT);
  coilsOff();
  delay(250);
}

void loop(){
  direction = true;
  timedRun(stepForward);

  direction = false;
  timedRun(stepReverse);
}
