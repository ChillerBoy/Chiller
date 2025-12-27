/* Enhanced DRV8825 controller for Sporlan SER-AA EEV */

const int STEP_PIN=9;
const int DIR_PIN=10;
const int EN_PIN=-1;

const unsigned int FULL_STROKE_STEPS=2500;
const unsigned int INIT_OVERDRIVE_STEPS=3000;
const unsigned long STEPS_PER_SEC=200;
const unsigned long PHASE_MS=15000;
const unsigned int STEP_PULSE_US=3;

unsigned long stepPeriodUs;
unsigned long nextStepUs;
unsigned long lastPhaseMs;
bool dirOpen=false;
unsigned long stepsThisPhase=0;

void pulseStep(){
  digitalWrite(STEP_PIN,HIGH);
  delayMicroseconds(STEP_PULSE_US);
  digitalWrite(STEP_PIN,LOW);
}

void setup(){
  pinMode(STEP_PIN,OUTPUT);
  pinMode(DIR_PIN,OUTPUT);
  if(EN_PIN>=0){ pinMode(EN_PIN,OUTPUT); digitalWrite(EN_PIN,LOW); }

  Serial.begin(115200);
  while(!Serial){}

  stepPeriodUs=1000000UL/STEPS_PER_SEC;
  nextStepUs=micros();

  Serial.println(F("Init: Over-drive closing 3000 steps"));
  digitalWrite(DIR_PIN,LOW);
  for(unsigned long i=0;i<INIT_OVERDRIVE_STEPS;i++){
    pulseStep();
    delayMicroseconds(stepPeriodUs);
  }
  Serial.println(F("Init complete. Starting cycles."));

  dirOpen=true;
  digitalWrite(DIR_PIN,HIGH);
  lastPhaseMs=millis();
  stepsThisPhase=0;
}

void loop(){
  unsigned long nowMs=millis();
  unsigned long nowUs=micros();

  if(nowMs-lastPhaseMs>=PHASE_MS){
    Serial.print(F("Phase done: "));
    Serial.print(dirOpen?F("OPEN "):F("CLOSE "));
    Serial.print(F("Steps=")); Serial.println(stepsThisPhase);

    dirOpen=!dirOpen;
    digitalWrite(DIR_PIN,dirOpen?HIGH:LOW);
    lastPhaseMs=nowMs;
    stepsThisPhase=0;
  }

  if((long)(nowUs-nextStepUs)>=0){
    pulseStep();
    stepsThisPhase++;
    nextStepUs+=stepPeriodUs;
  }
}
