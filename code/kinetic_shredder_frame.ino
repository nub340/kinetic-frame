#include <IRremote.hpp>
#include <EEPROM.h>

// Define where in memory to start storing state (i.e. offset). 
#define EEPROM_ADDRESS 0

// Define Arduino pins
#define BUZZER_PIN 2
#define SLEEP_PIN 3
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

// Define stepper motor parameters
#define STEPS_PER_REV 200
#define SPEED_DELAY 5000
#define TOTAL_REVS 2.6
#define TOTAL_STEPS (STEPS_PER_REV * TOTAL_REVS)

// Initialize globals
IRrecv irrecv(IR_PIN);
// Tracks how long the buzzer has been in the current state
long lastBuzzerMillis = 0;
// Tracks the state of the buzzer. LOW = OFF, HIGH = ON
int buzzerState = LOW;
// Tracks whether the frame is currently in the shredded state or not
bool isShredded = false;
// Tracks whether the mute function is enabled or not
bool isMuted = false;
// Tracks the position of the main canvas belt
int position = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial); //wait until ready

  // Restore state from EEPROM
  isShredded = getShreddedState();
  isMuted = getMutedState();
  position = getLocationState();
  
  // Setup pin modes
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); // Enable driver (LOW means Enable)

  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, HIGH);

  pinMode(STEP_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);

  pinMode(SLEEP_PIN, OUTPUT);
  digitalWrite(SLEEP_PIN, LOW);

  pinMode(BUZZER_PIN, OUTPUT);

  // Start listening for IR commands
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  // Output debug messages to serial monitor
  Serial.println("Running...");
  if (position > 0){
    Serial.println("Recovering...");
    stepReverse(position, false);
  } else if (position < 0) {
    Serial.println("Recovering...");
    stepForward(abs(position), false);
  }

  Serial.print("Muted: ");
  Serial.print(isMuted);
  Serial.print(", Shredded: ");
  Serial.print(isShredded);
  Serial.print(", Location: ");
  Serial.println(position);
}

/**
Main loop
*/
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
    Serial.print(", Position: ");
    Serial.println(position);
  }
}

/*
Handles IR remote commands
*/
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
      Serial.println("Reset position");
      resetPositionState();
      break;

    case IR_BUTTON_LEFT:
      if(!isShredded){
        Serial.println("Shredding");
        enterShreddedState();
      } else {
        Serial.println("Already shredded");
      }
      break;

    case IR_BUTTON_RIGHT:
      if(isShredded){
        Serial.println("Restoring");
        enterNormalState();
      } else {
        Serial.println("Already restored");
      }
      break;

    case IR_BUTTON_POUND:
      Serial.println("Toggle state");
      toggleMute();
      break;
  }
}

/*
Run main shred/un-shred sequence
*/
void runMainSequence() {
  Serial.println("runMainSequence");
  int startMillis = millis();
  IrReceiver.stop();
  stepForward(TOTAL_STEPS, true);
  setShreddedState(true);
  Serial.println("Waiting 5 seconds...");
  delay(5000);
  stepReverse(TOTAL_STEPS, false);
  setShreddedState(false);
  Serial.println("Waiting 5 seconds...");
  delay(5000);
  Serial.println("Running...");
  int elapsedMicros = (millis() - startMillis) * 1000;
  IrReceiver.start(elapsedMicros);
}

/*
Toggles the mute feature on/off
*/
void toggleMute() {
  setMutedState(!isMuted);
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

/*
Resets the position state to zero.
*/
void resetPositionState() {
  position = 0;
}

/*
Enter the shredded state if not already there
*/
void enterShreddedState() {
  if (isShredded) {
    return;
  }
  Serial.println("enterShreddedState");
  int startMillis = millis();
  IrReceiver.stop();
  stepForward(TOTAL_STEPS, true);
  setShreddedState(true);
  int elapsedMicros = (millis() - startMillis) * 1000;
  IrReceiver.start(elapsedMicros);
}

/*
Enter normal state if not already there
*/
void enterNormalState() {
  if (!isShredded) {
    return;
  }
  Serial.println("enterNormalState");
  int startMillis = millis();
  IrReceiver.stop();
  stepReverse(TOTAL_STEPS, false);
  setShreddedState(false);
  int elapsedMicros = (millis() - startMillis) * 1000;
  IrReceiver.start(elapsedMicros);
}

/*
Move the position of the canvas down a bit
*/
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
    delayMicroseconds(SPEED_DELAY);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(SPEED_DELAY);
  }
  setLocationState(position+steps);
  digitalWrite(SLEEP_PIN, LOW);
  noTone(BUZZER_PIN);
}

/*
Move the position of the canvas up a bit
*/
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
    delayMicroseconds(SPEED_DELAY);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(SPEED_DELAY);
    
  }
  setLocationState(position-steps);
  digitalWrite(SLEEP_PIN, LOW);
  noTone(BUZZER_PIN);
}

/*
Handles making the buzzer beep on and off at ther right pitch and tempo
*/
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

/*
Persist the shredded state to memory
*/
void setShreddedState(bool newShreddedState) {
  isShredded = newShreddedState;
  EEPROM.write(EEPROM_ADDRESS, isShredded);
}

/*
Reads the shredded state from memory
*/
bool getShreddedState() {
  return EEPROM.read(EEPROM_ADDRESS);
}

/*
Persist the mute state to memory
*/
void setMutedState(bool newMutedState) {
  isMuted = newMutedState;
  EEPROM.write(EEPROM_ADDRESS+1, isMuted);
}

/*
Read the mute state from memory
*/
bool getMutedState() {
  return EEPROM.read(EEPROM_ADDRESS+1);
}

/*
Persist the location state to memory
*/
void setLocationState(int newLocationState) {
  position = newLocationState;
  writeIntIntoEEPROM(EEPROM_ADDRESS+2, position);
}

/*
Read the location state from memory
*/
int getLocationState() {
  return readIntFromEEPROM(EEPROM_ADDRESS+2);
}

/*
Helper function for writing an integer value to memory
*/
void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

/**
Helper function for reading an integer value from memory
*/
int readIntFromEEPROM(int address)
{
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}
