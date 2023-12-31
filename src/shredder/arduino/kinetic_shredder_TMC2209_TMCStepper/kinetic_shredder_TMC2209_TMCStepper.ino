#include <IRremote.hpp>
#include <EEPROM.h>
#include <TMCStepper.h>
#include <SoftwareSerial.h>

#define EEPROM_ADDRESS 0
#define DRIVER_ADDRESS 0b00 // TMC2209 Driver address according to MS1 and MS2

// Define Arduino pins
#define BUZZ_PIN 2
#define CLK_PIN 3
#define STEP_PIN 4
#define DIR_PIN 5
#define IR_PIN 6
#define RX_PIN 7
#define TX_PIN 8
#define MS2_PIN 9
#define MS1_PIN 10
#define EN_PIN 11
#define AUX_PIN 12

// Define IR remote command codes
#define IR_BUTTON_OK 28
#define IR_BUTTON_UP 24
#define IR_BUTTON_DOWN 82
#define IR_BUTTON_LEFT 8
#define IR_BUTTON_RIGHT 90
#define IR_BUTTON_ASTERISK 22
#define IR_BUTTON_POUND 13

// Define motor parameters
#define STEPS_PER_REV 200
#define MAX_CURRENT 1000
#define R_SENSE 0.11f
#define MICROSTEPS  4 // Set the microstepping mode (1, 2, 4, 8, 16)
#define SPEED_DELAY 8000 / MICROSTEPS
#define TOTAL_REVS 2.6
#define TOTAL_STEPS (STEPS_PER_REV * TOTAL_REVS) * MICROSTEPS
#define TRIM_STEPS 20 

// Instantiate classes
IRrecv irrecv(IR_PIN);
SoftwareSerial SoftSerial(RX_PIN, TX_PIN);
TMC2209Stepper driver(&SoftSerial, R_SENSE, DRIVER_ADDRESS);

// Initialize global variables
long _lastBuzzerMillis = 0;
long _irrecvPausedAtMillis = 0;
int _buzzerState = LOW;
bool _isShredded = false;
bool _isMuted = false;
int _location = 0;

/**
 * Runs once per restart to setup the device and starting state
 */
void setup() {
  Serial.begin(115200);
  while (!Serial); //wait until ready
  SoftSerial.begin(115200);
  driver.beginSerial(115200);

  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);  // Enable driver (LOW means Enable)

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  driver.begin();                 
  driver.rms_current(MAX_CURRENT); 
  driver.microsteps(MICROSTEPS);
  driver.toff(4);
  driver.en_spreadCycle(false);
  driver.pwm_autoscale(true); // Needed for stealthChop

  _isShredded = getShreddedState();
  _isMuted = getMutedState();
  _location = getLocationState();

  Serial.println("Running...");
  writeStateToSerial();
}

/**
 * The main loop.
 */
void loop() {

  // Returns 0 if no data ready, 1 if data ready.
  if (IrReceiver.decode()) {
    int command = IrReceiver.decodedIRData.command;
    handleIRRemoteCommand(command);
    irrecv.resume();

    writeStateToSerial();
  }
}

/**
 * handleIRRemoteCommand
 * 
 * Handles the IR remote commands
 */
void handleIRRemoteCommand(int command) {
  Serial.print("command: ");
  Serial.print(command);
  Serial.print(" - ");
  if (command == 0) {
    Serial.println("undefined");
  } else {
    switch (command) {
      case IR_BUTTON_UP:
        Serial.println("Trim up");
        stepReverse(TRIM_STEPS, false);
        break;
  
      case IR_BUTTON_DOWN:
        Serial.println("Trim down");
        stepForward(TRIM_STEPS, false);
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
    }
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
    stepReverse(TOTAL_STEPS, false);
    setShreddedState(false);
  } else {
    stepForward(TOTAL_STEPS, true);
    setShreddedState(true);
  }

  Serial.println("Waiting 5 seconds...");
  delay(5000);

  if(isSequenceReversed){
    stepForward(TOTAL_STEPS, true);
    setShreddedState(true);
  } else {
    stepReverse(TOTAL_STEPS, false);
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
  setLocationState(0);
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
  stepForward(TOTAL_STEPS, true);
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
  stepReverse(TOTAL_STEPS, false);
  setShreddedState(false);
  startIRReceiver();
}

/**
 * stepForward
 * 
 * Sets the stepper motor direction and calls stepAndBuzz
 */
void stepForward(int steps, bool playBuzzer) {
  Serial.print("stepForward(");
  Serial.print(steps);
  Serial.println(")");
  driver.shaft(false);
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
  driver.shaft(true);
  delay(1);

  stepAndBuzz(steps, playBuzzer);
}

/**
 * stepAndBuzz
 * 
 * Sends the commands to the stepper motor and piezo buzzer
 * according to the parameters passed
 */
void stepAndBuzz(int steps, bool playBuzzer) {
  for (int i = 0; i <= steps; i++) {
    if (playBuzzer) {
      handleBuzzer();
    }
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(SPEED_DELAY);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(SPEED_DELAY);
  }
  setLocationState(_location-steps);
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
