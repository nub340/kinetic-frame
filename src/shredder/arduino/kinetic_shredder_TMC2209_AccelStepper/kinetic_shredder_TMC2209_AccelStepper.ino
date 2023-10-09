#include <IRremote.hpp>
#include <EEPROM.h>
#include <AccelStepper.h>
#include <TMCStepper.h>
#include <SoftwareSerial.h>

#define EEPROM_ADDRESS 0
#define DRIVER_ADDRESS 0b00 // TMC2209 Driver address according to MS1 and MS2

#define BUZZER_PIN 2
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

#define IR_BUTTON_OK 28
#define IR_BUTTON_UP 24
#define IR_BUTTON_DOWN 82
#define IR_BUTTON_LEFT 8
#define IR_BUTTON_RIGHT 90
#define IR_BUTTON_ASTERISK 22
#define IR_BUTTON_POUND 13

#define STEPS_PER_REV 200
#define SPEED_DELAY 500
#define MAX_CURRENT 1000
#define R_SENSE 0.11f
#define MICROSTEPS  8 // Set the microstepping mode (1, 2, 4, 8, 16)
#define TOTAL_REVS 2.6
#define TOTAL_STEPS (STEPS_PER_REV * TOTAL_REVS) * MICROSTEPS
#define TRIM_STEPS 20 

SoftwareSerial SoftSerial(RX_PIN, TX_PIN);
TMC2209Stepper driver(&SoftSerial, R_SENSE, DRIVER_ADDRESS);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
IRrecv irrecv(IR_PIN);

long _lastBuzzerMillis = 0;
long _irrecvPausedAtMillis = 0;
int _buzzerState = LOW;
bool _isShredded = false;
bool _isMuted = false;
int _location = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial); //wait until ready
  SoftSerial.begin(115200);
  driver.beginSerial(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);      // LOW means enable

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  driver.begin();                 
  driver.toff(4);                  // Enables driver in software
  driver.rms_current(MAX_CURRENT); 
  driver.microsteps(MICROSTEPS);
  driver.en_spreadCycle(false);
  driver.pwm_autoscale(true); // Needed for stealthChop

  stepper.setEnablePin(EN_PIN);
  stepper.setMaxSpeed(1000);       // Set the maximum speed in steps per second
  stepper.setSpeed(600);
  stepper.setAcceleration(2000);  // Set the acceleration in steps per second^2
//  stepper.setPinsInverted(false, false, true);
//  stepper.enableOutputs();
  
  Serial.println("running...");
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

void toggleMute() {
  Serial.println("Toggling mute");
  setMutedState(!_isMuted);
  stopIRReceiver();
  tone(BUZZER_PIN, 3000);
  delay(500);
  noTone(BUZZER_PIN);
  startIRReceiver();
}


void resetLocationState() {
  _location = 0;
}

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
void stepForward(int steps, bool playBuzzer) {
  Serial.print("stepForward(");
  Serial.print(steps);
  Serial.println(")");
  delay(1);
  
  stepper.move(steps);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    if (playBuzzer) {
      handleBuzzer();
    }
  }
  setLocationState(_location+steps);
  noTone(BUZZER_PIN);
}

void stepReverse(int steps, bool playBuzzer) {
  Serial.print("stepReverse(");
  Serial.print(steps);
  Serial.println(")");
  delay(1);

  stepper.move(0-steps);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    if (playBuzzer) {
      handleBuzzer();
    }
  }
  setLocationState(_location-steps);
  noTone(BUZZER_PIN);
}

void stopIRReceiver() {
  // capture millis when reciever was stopped.
  _irrecvPausedAtMillis = millis();
  IrReceiver.stop();
}

void startIRReceiver() {
  // Calculate the elapsed millis since the IRReceiver was stopped. 
  // Start IR receiver again and pass the elapsed time to avoid bug with internal clock.
  // This took me weeks to figure out. The remote would only work once then a reboot was needed.
  int elapsedMicros = (millis() - _irrecvPausedAtMillis) * 1000;
  IrReceiver.start(elapsedMicros);
}

void handleBuzzer() {
  if(_isMuted) {
    _buzzerState = LOW;
    noTone(BUZZER_PIN);
  } else {
    unsigned long currentMillis = millis();
    if (currentMillis - _lastBuzzerMillis > 250) {
      _lastBuzzerMillis = currentMillis;
      if (_buzzerState == LOW) {
        _buzzerState = HIGH;
        tone(BUZZER_PIN, 3000);
      }
      else {
        _buzzerState = LOW;
        noTone(BUZZER_PIN);
      }
    }
  }
}

void writeStateToSerial() {
  Serial.print("Muted: ");
  Serial.print(_isMuted);
  Serial.print(", Shredded: ");
  Serial.print(_isShredded);
  Serial.print(", Location: ");
  Serial.print(_location);
  Serial.println("");
}

void setShreddedState(bool shredded) {
  _isShredded = shredded;
  EEPROM.put(EEPROM_ADDRESS, shredded);
}

bool getShreddedState() {
  bool shredded;
  EEPROM.get(EEPROM_ADDRESS, shredded);
  _isShredded = shredded;
  return shredded;
}

void setMutedState(bool isMuted) {
  _isMuted = isMuted;
  EEPROM.put(EEPROM_ADDRESS+1, isMuted);
}

bool getMutedState() {
  bool isMuted;
  EEPROM.get(EEPROM_ADDRESS+1, isMuted);
  _isMuted = isMuted;
  return isMuted;
}

void setLocationState(int location) {
  _location = location;
  EEPROM.put(EEPROM_ADDRESS+2, location);
}

int getLocationState() {
  int location;
  EEPROM.get(EEPROM_ADDRESS+2, location);
  _location = location;
  return location;
}
