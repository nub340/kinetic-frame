#include <Arduino.h>
#include <IRremote.hpp>
#include <EEPROM.h>
#include "BasicStepperDriver.h"

#define EEPROM_ADDRESS 0

// Define stepper motor connections and steps per revolution:
#define BUZZER_PIN 2
#define SLEEP_PIN 3
#define STEP_PIN 4
#define DIR_PIN 5
#define IR_PIN 6
#define EN_PIN 7

#define IR_BUTTON_1 69
#define IR_BUTTON_2 70
#define IR_BUTTON_3 71
#define IR_BUTTON_4 68
#define IR_BUTTON_5 64
#define IR_BUTTON_6 67
#define IR_BUTTON_7 7
#define IR_BUTTON_8 21
#define IR_BUTTON_9 9
#define IR_BUTTON_0 25
#define IR_BUTTON_OK 28
#define IR_BUTTON_UP 24
#define IR_BUTTON_DOWN 82
#define IR_BUTTON_LEFT 8
#define IR_BUTTON_RIGHT 90
#define IR_BUTTON_ASTERISK 22
#define IR_BUTTON_POUND 13

#define STEPS_PER_REV 200
#define RPM_1 10
#define RPM_2 20
#define RPM_3 30
#define RPM_4 40
#define RPM_5 50
#define RPM_6 60
#define RPM_7 70
#define RPM_8 80
#define RPM_9 90
#define RPM_0 100

// 1=full step, 2=half step etc.
#define MICROSTEPS 1
#define SPEED_DELAY_NORMAL 5000
#define SPEED_DELAY_SLOW 10000
#define MOTOR_ACCEL 2000
#define MOTOR_DECEL 1000
#define TOTAL_REVS 2.6
#define TOTAL_STEPS (STEPS_PER_REV * TOTAL_REVS)

BasicStepperDriver stepper(STEPS_PER_REV, DIR_PIN, STEP_PIN, SLEEP_PIN);
//A4988 stepper(STEPS_PER_REV, DIR_PIN, STEP_PIN, SLEEP_PIN, 8, 9, 10);

IRrecv irrecv(IR_PIN);
long lastBuzzerMillis = 0;
int buzzerState = LOW;
bool isShredded = false;
bool isMuted = false;
int location = 0;
int current_rpm = RPM_1;

void setup() {
  Serial.begin(9600);
  while (!Serial); //wait until ready

  stepper.begin(current_rpm, MICROSTEPS);

  isShredded = getShreddedState();
  isMuted = getMutedState();
  location = getLocationState();
  current_rpm = getRPMState();

  pinMode(BUZZER_PIN, OUTPUT);

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  Serial.println("Running...");
  if (location > 0){
    Serial.println("Recovering...");
    stepReverse(location, false);
  } else if (location < 0) {
    Serial.println("Recovering...");
    stepForward(abs(location), false);
  }

  Serial.print("Muted: ");
  Serial.print(isMuted);
  Serial.print(", Shredded: ");
  Serial.print(isShredded);
  Serial.print(", Location: ");
  Serial.println(location);
}

void loop() {

  // Returns 0 if no data ready, 1 if data ready.
  if (IrReceiver.decode()) {
    int command = IrReceiver.decodedIRData.command;
    handleIRRemoteCommand(command);
    irrecv.resume();

    Serial.print("Muted: ");
    Serial.print(isMuted);
    Serial.print(", Shredded: ");
    Serial.print(isShredded);
    Serial.print(", Location: ");
    Serial.println(location);
  }
}

void handleIRRemoteCommand(int command) {
  if (command != 0) {
    Serial.print("command: ");
    Serial.print(command);
    Serial.print(" - ");
  }
  switch (command) {
    case IR_BUTTON_UP:
      Serial.println("Trim up");
      stepReverse(10, false);
      break;

    case IR_BUTTON_DOWN:
      Serial.println("Trim down");
      stepForward(10, false);
      break;

    case IR_BUTTON_OK:
      Serial.println("Main sequence");
      runMainSequence();
      break;

    case IR_BUTTON_ASTERISK:
      Serial.println("Reset location");
      resetHomeLocation();
      break;

    case IR_BUTTON_LEFT:
      enterShreddedState();
      break;

    case IR_BUTTON_RIGHT:
      enterNormalState();
      break;

    case IR_BUTTON_POUND:
      toggleMute();
      break;

    case IR_BUTTON_1:
      setRPMState(RPM_1);
      break;

    case IR_BUTTON_2:
      setRPMState(RPM_2);
      break;

    case IR_BUTTON_3:
      setRPMState(RPM_3);
      break;

    case IR_BUTTON_4:
      setRPMState(RPM_4);
      break;    

    case IR_BUTTON_5:
      setRPMState(RPM_5);
      break; 

    case IR_BUTTON_6:
      setRPMState(RPM_6);
      break; 

    case IR_BUTTON_7:
      setRPMState(RPM_7);
      break; 

    case IR_BUTTON_8:
      setRPMState(RPM_8);
      break; 

    case IR_BUTTON_9:
      setRPMState(RPM_9);
      break; 

    case IR_BUTTON_0:
      setRPMState(RPM_0);
      break; 
  }
}

void runMainSequence() {
  Serial.println("runMainSequence");
  
  // Keep track of how long the IR receiver has been stopped
  int startMillis = millis();
  IrReceiver.stop();

  // If already in the shredded state, run the main sequence in reverse
  bool isSequenceReversed = isShredded;
  
  if(isSequenceReversed){
    rotateDegrees(0-TOTAL_STEPS, false);
    //stepReverse(TOTAL_STEPS, false);
    setShreddedState(false);
  } else {
    rotateDegrees(TOTAL_STEPS, true);
    //stepForward(TOTAL_STEPS, true);
    setShreddedState(true);
  }

  Serial.println("Waiting 5 seconds...");
  delay(5000);

  if(isSequenceReversed){
    rotateDegrees(TOTAL_STEPS, true);
    //stepForward(TOTAL_STEPS, true);
    setShreddedState(true);
  } else {
    rotateDegrees(0-TOTAL_STEPS, false);
    //stepReverse(TOTAL_STEPS, false);
    setShreddedState(false);
  }
  
  Serial.println("Waiting 5 seconds...");
  delay(5000);
  Serial.println("Running...");

  // Calculate how long the timer has been stopped. 
  // Start IR receiver again and pass the elapsed time to avoid bug with internal clock.
  int elapsedMicros = (millis() - startMillis) * 1000;
  IrReceiver.start(elapsedMicros);
}

void toggleMute() {
  Serial.println("Toggling mute");
  setMutedState(!isMuted);
  beepOnce();
}

void beepOnce() {
  int startMillis = millis();
  //must stop the IRReceiver as the tone/notone functions need to use the same timer
  IrReceiver.stop(); 
  tone(BUZZER_PIN, 3000);
  delay(500);
  noTone(BUZZER_PIN);
  int elapsedMicros = (millis() - startMillis) * 1000;
  //keep track of how much time has elapsed and use it to restart the IRReceiver
  IrReceiver.start(elapsedMicros); 
}

void resetHomeLocation() {
  setLocationState(0);
  setShreddedState(false);
  beepOnce();
}

void setRpm(int rpm) {
  stepper.stop();
  stepper.begin(rpm, MICROSTEPS);
  setRPMState(rpm);
  beepOnce();
}

void enterShreddedState() {
  if (isShredded) {
    Serial.println("Already shredded");
    return;
  }
  Serial.println("Shredding...");
  int startMillis = millis();
  IrReceiver.stop();
  stepForward(TOTAL_STEPS, true);
  setShreddedState(true);
  int elapsedMicros = (millis() - startMillis) * 1000;
  IrReceiver.start(elapsedMicros);
}

void enterNormalState() {
  if (!isShredded) {
    Serial.println("Already restored");
    return;
  }
  Serial.println("Restoring...");
  int startMillis = millis();
  IrReceiver.stop();
  stepReverse(TOTAL_STEPS, false);
  int elapsedMicros = (millis() - startMillis) * 1000;
  IrReceiver.start(elapsedMicros);
  resetHomeLocation();
}

void rotateDegrees(int deg, bool playBuzzer) {
  stepper.enable();
  stepper.move(deg);
  stepper.disable();
}

void stepForward(int steps, bool playBuzzer) {
  Serial.print("stepForward(");
  Serial.print(steps);
  Serial.println(")");
  digitalWrite(SLEEP_PIN, HIGH);
  digitalWrite(DIR_PIN, HIGH);
  delay(1);

  for (int i = 0; i <= steps; i++) {
    if (playBuzzer) {
      handleBuzzer();
    }
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(SPEED_DELAY_SLOW);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(SPEED_DELAY_SLOW);
  }
  setLocationState(location+steps);
  digitalWrite(SLEEP_PIN, LOW);
  noTone(BUZZER_PIN);
}

void stepReverse(int steps, bool playBuzzer) {
  Serial.print("stepReverse(");
  Serial.print(steps);
  Serial.println(")");
  digitalWrite(SLEEP_PIN, HIGH);
  digitalWrite(DIR_PIN, LOW);
  delay(1);

  for (int i = 0; i <= steps; i++) {
    if (playBuzzer) {
      handleBuzzer();
    }
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(SPEED_DELAY_SLOW);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(SPEED_DELAY_SLOW);
    
  }
  setLocationState(location-steps);
  digitalWrite(SLEEP_PIN, LOW);
  noTone(BUZZER_PIN);
}

void handleBuzzer() {
  if(isMuted) {
    buzzerState = LOW;
    noTone(BUZZER_PIN);
  } else {
    unsigned long currentMillis = millis();
    if (currentMillis - lastBuzzerMillis > 250) {
      lastBuzzerMillis = currentMillis;
      if (buzzerState == LOW) {
        buzzerState = HIGH;
        tone(BUZZER_PIN, 3000);
      }
      else {
        buzzerState = LOW;
        noTone(BUZZER_PIN);
      }
    }
  }
}

void setShreddedState(bool newShreddedState) {
  isShredded = newShreddedState;
  EEPROM.put(EEPROM_ADDRESS, isShredded);
}

bool getShreddedState() {
  bool shredded;
  EEPROM.get(EEPROM_ADDRESS, shredded);
  return shredded;
}

void setMutedState(bool newMutedState) {
  isMuted = newMutedState;
  EEPROM.put(EEPROM_ADDRESS+1, isMuted);
}

bool getMutedState() {
  bool isMuted;
  EEPROM.get(EEPROM_ADDRESS+1, isMuted);
  return isMuted;
}

void setLocationState(int newLocationState) {
  location = newLocationState;
  EEPROM.put(EEPROM_ADDRESS+2, location);
}

int getLocationState() {
  int location;
  EEPROM.get(EEPROM_ADDRESS+2, location);
  return location;
}

void setRPMState(int rpm) {
  current_rpm = rpm;
  EEPROM.put(EEPROM_ADDRESS+2+sizeof(int), rpm);
}

int getRPMState() {
  int rpm;
  EEPROM.get(EEPROM_ADDRESS+2+sizeof(int), rpm);
  return rpm;
}
