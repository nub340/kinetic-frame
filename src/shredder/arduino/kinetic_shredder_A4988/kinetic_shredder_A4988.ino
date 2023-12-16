#include <Arduino.h>
#include <IRremote.hpp>
#include <EEPROM.h>
#include "BasicStepperDriver.h"

#define EEPROM_ADDRESS 0

// Define stepper motor connections and steps per revolution:
#define BUZZ_PIN 2
#define SLEEP_PIN 3
#define STEP_PIN 4
#define DIR_PIN 5
#define IR_PIN 6
#define EN_PIN 7

// Define IR remote command codes
#define IR_BUTTON_OK 28
#define IR_BUTTON_UP 24
#define IR_BUTTON_DOWN 82
#define IR_BUTTON_LEFT 8
#define IR_BUTTON_RIGHT 90
#define IR_BUTTON_ASTERISK 22
#define IR_BUTTON_POUND 13
#define IR_BUTTON_NO1 69
#define IR_BUTTON_NO2 70
#define IR_BUTTON_NO3 71

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
#define STEPS_PER_REV 200
#define MAX_CURRENT 1000
#define MICROSTEPS  1 // Set the microstepping mode (1, 2, 4, 8, 16)
#define SPEED_SLOW 10000
#define SPEED_MEDIUM 5500
#define SPEED_FAST 3000
#define SPEED_DELAY SPEED_MEDIUM / MICROSTEPS
#define TOTAL_REVS 3
#define TOTAL_STEPS (STEPS_PER_REV * TOTAL_REVS) * MICROSTEPS
#define TRIM_STEPS 20 
#define MOTOR_ACCEL 2000
#define MOTOR_DECEL 1000


//BasicStepperDriver stepper(STEPS_PER_REV, DIR_PIN, STEP_PIN, SLEEP_PIN);
//A4988 stepper(STEPS_PER_REV, DIR_PIN, STEP_PIN, SLEEP_PIN, 8, 9, 10);

IRrecv irrecv(IR_PIN);

long _lastBuzzerMillis = 0;
long _irrecvPausedAtMillis = 0;
int _buzzerState = LOW;
bool _isShredded = false;
bool _isMuted = false;
int _location = 0;
int _stopLocation = TOTAL_STEPS;
int _speed = SPEED_MEDIUM;

void setup() {
  Serial.begin(115200);
  while (!Serial); //wait until ready

  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); // Enable driver (LOW means Enable, usually)
  
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  //stepper.begin(current_rpm, MICROSTEPS);

  _isShredded = getShreddedState();
  _isMuted = getMutedState();
  _location = getLocationState();
  _stopLocation = getStopLocationState();
  _speed = getSpeedState();

  Serial.println("Running...");
  writeStateToSerial();
}

void loop() {

  // Returns 0 if no data ready, 1 if data ready.
  if (IrReceiver.decode()) {
    int command = IrReceiver.decodedIRData.command;
    handleIRRemoteCommand(command);
    irrecv.resume();

    writeStateToSerial();
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
      resetLocationState();
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

      case IR_BUTTON_NO1:
        setSpeedState(SPEED_SLOW);
        break;
  
      case IR_BUTTON_NO2:
        setSpeedState(SPEED_MEDIUM);
        break;
  
      case IR_BUTTON_NO3:
        setSpeedState(SPEED_FAST);
        break;
  }
}

/**
 * runMainSequence
 * 
 * Executes the main sequence normally, or in reverse, 
 * depending on the current shredded state.
 */
void runMainSequence() {
  Serial.println("runMainSequence");

  stopIRReceiver();

  // If already in the shredded state, run the main sequence in reverse
  bool isSequenceReversed = _isShredded;
  
  if(isSequenceReversed){
    stepReverse(_location, false);
    setShreddedState(false);
  } else {
    stepForward(_stopLocation, true);
    setShreddedState(true);
  }

  Serial.println("Waiting 5 seconds...");
  delay(5000);

  if(isSequenceReversed){
    stepForward(_stopLocation, true);
    setShreddedState(true);
  } else {
    stepReverse(_location, false);
    setShreddedState(false);
  }
  
  Serial.println("Waiting 3 seconds...");
  delay(3000);
  Serial.println("Running...");

  startIRReceiver();
}

/**
 * toggleMute
 * 
 * Toggles the mute feature on/off
 */
void toggleMute() {
  Serial.println("Toggling mute");
  setMutedState(!_isMuted);
  stopIRReceiver();
  tone(BUZZ_PIN, 3000);
  delay(500);
  noTone(BUZZ_PIN);
  startIRReceiver();
}

/**
 * resetLocationState
 * 
 * Resets the location state to zero
 */
void resetLocationState() {
  if(_isShredded) {
    _stopLocation = _location <= 0 ? TOTAL_STEPS : _location;
    setLocationState(_location);
  }
  else {
    _location = 0;
    setLocationState(0);
  }
}

void beepOnce() {
  int startMillis = millis();
  //must stop the IRReceiver as the tone/notone functions need to use the same timer
  IrReceiver.stop(); 
  tone(BUZZ_PIN, 3000);
  delay(500);
  noTone(BUZZ_PIN);
  int elapsedMicros = (millis() - startMillis) * 1000;
  //keep track of how much time has elapsed and use it to restart the IRReceiver
  IrReceiver.start(elapsedMicros); 
}

/**
 * enterShreddedState
 * 
 * Puts the device into the "shredded" state
 */
void enterShreddedState() {
  if (_isShredded) {
    Serial.println("Already shredded");
    return;
  }
  Serial.println("Shredding...");
  stopIRReceiver();
  stepForward(_stopLocation - _location, true);
  setShreddedState(true);
  startIRReceiver();
}

/**
 * enterNormalState
 * 
 * Puts the device into the "normal" state
 */
void enterNormalState() {
  if (!_isShredded) {
    Serial.println("Already restored");
    return;
  }
  Serial.println("Restoring...");
  stopIRReceiver();
  stepReverse(_location, false);
  setShreddedState(false);
  startIRReceiver();
}

//void rotateDegrees(int deg, bool playBuzzer) {
//  stepper.enable();
//  stepper.move(deg);
//  stepper.disable();
//}

/**
 * stepForward
 * 
 * Sets the stepper motor direction and calls stepAndBuzz
 */
void stepForward(int steps, bool playBuzzer) {
  Serial.print("stepForward(");
  Serial.print(steps);
  Serial.println(")");
  //driver.shaft(false);
  digitalWrite(DIR_PIN, HIGH);
  delay(1);

  stepAndBuzz(steps, playBuzzer);
}

/**
 * stepReverse
 * 
 * Sets the stepper motor direction and calls stepAndBuzz
 */
void stepReverse(int steps, bool playBuzzer) {
  Serial.print("stepReverse(");
  Serial.print(steps);
  Serial.println(")");
  //driver.shaft(true);
  digitalWrite(DIR_PIN, LOW);
  delay(1);

  stepAndBuzz(-steps, playBuzzer);
}

/**
 * stepAndBuzz
 * 
 * Sends the commands to the stepper motor and piezo buzzer
 * according to the parameters passed
 */
void stepAndBuzz(int steps, bool playBuzzer) {
  int speedDelay = _speed / MICROSTEPS;
  for (int i = 0; i <= abs(steps); i++) {
    if (playBuzzer) {
      handleBuzzer();
    }
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(speedDelay);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(speedDelay);
  }
  setLocationState(_location+steps);
  noTone(BUZZ_PIN);
}

/**
 * stopIRReceiver
 * 
 * Captures millis when reciever was stopped.
 */
void stopIRReceiver() {
  _irrecvPausedAtMillis = millis();
  IrReceiver.stop();
}

/*
 * startIRReceiver
 * 
 * Calculates the elapsed millis since the IRReceiver was stopped.
 * Starts IR receiver again and passes the elapsed time to avoid an issue with the internal clock.
 * This took me weeks to figure out! The remote would only work once per restart...
*/
void startIRReceiver() {
  int elapsedMicros = (millis() - _irrecvPausedAtMillis) * 1000;
  IrReceiver.start(elapsedMicros);
}

/*
 * handleBuzzer
 * 
 * This function is called repeatedly while the motor is spinning. It's job is to handle the
 * piezo buzzer signal and is hard-coded to beep on/off every quarter second.
*/
void handleBuzzer() {
  if(_isMuted) {
    _buzzerState = LOW;
    noTone(BUZZ_PIN);
  } else {
    unsigned long currentMillis = millis();
    if (currentMillis - _lastBuzzerMillis > 250) {
      _lastBuzzerMillis = currentMillis;
      if (_buzzerState == LOW) {
        _buzzerState = HIGH;
        tone(BUZZ_PIN, 3000);
      }
      else {
        _buzzerState = LOW;
        noTone(BUZZ_PIN);
      }
    }
  }
}

/**
 * writeStateToSerial
 * 
 * Outputs the current state to the Serial monitor
 */
void writeStateToSerial() {
  Serial.print("Muted: ");
  Serial.print(_isMuted);
  Serial.print(", Shredded: ");
  Serial.print(_isShredded);
  Serial.print(", Location: ");
  Serial.print(_location);
  Serial.print(", Stop Location: ");
  Serial.print(_stopLocation);
  Serial.print(", Speed: ");
  Serial.print(SPEED_SLOW == _speed ? "SLOW" : SPEED_MEDIUM == _speed ? "MEDIUM" : "FAST");
  Serial.println("");
}

/**
 * setShreddedState
 * 
 * Sets the shredded state and persists to flash
 */
void setShreddedState(bool shredded) {
  _isShredded = shredded;
  EEPROM.put(EEPROM_ADDRESS, shredded);
}

/**
 * getShreddedState
 * 
 * Reads the shredded state from flash
 */
bool getShreddedState() {
  bool shredded;
  EEPROM.get(EEPROM_ADDRESS, shredded);
  return shredded;
}

/**
 * setMutedState
 * 
 * Sets the muted state
 */
void setMutedState(bool isMuted) {
  _isMuted = isMuted;
  EEPROM.put(EEPROM_ADDRESS+1, isMuted);
}

/**
 * getMutedState
 * 
 * Reads the muted state from flash
 */
bool getMutedState() {
  bool isMuted;
  EEPROM.get(EEPROM_ADDRESS+1, isMuted);
  return isMuted;
}

/**
 * setLocationState
 * 
 * Sets the location state and persists it to flash
 */
void setLocationState(int location) {
  _location = location;
  EEPROM.put(EEPROM_ADDRESS+2, location);
}

/**
 * getLocationState
 * 
 * Reads the location state from flash
 */
int getLocationState() {
  int location;
  EEPROM.get(EEPROM_ADDRESS+2, location);
  return location;
}

/**
 * setStopLocationState
 * 
 * Sets the stop location state and persists it to flash
 */
void setStopLocationState(int location) {
  _stopLocation = location;
  EEPROM.put(EEPROM_ADDRESS+3, location);
}

/**
 * getStopLocationState
 * 
 * Reads the stop location state from flash
 */
int getStopLocationState() {
  int stopLocation;
  EEPROM.get(EEPROM_ADDRESS+3, stopLocation);
  return stopLocation <= 0 ? TOTAL_STEPS : stopLocation;
}

/**
 * setStopLocationState
 * 
 * Sets the speed state and persists it to flash
 */
void setSpeedState(int speed) {
  _speed = speed;
  EEPROM.put(EEPROM_ADDRESS+4, speed);
}

/**
 * getStopLocationState
 * 
 * Reads the speed state from flash
 */
int getSpeedState() {
  int speed;
  EEPROM.get(EEPROM_ADDRESS+4, speed);
  return speed <= 0 ? SPEED_MEDIUM : _speed;
}
