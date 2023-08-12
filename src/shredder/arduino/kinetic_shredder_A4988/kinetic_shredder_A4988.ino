#include <IRremote.hpp>
#include <EEPROM.h>

#define EEPROM_ADDRESS 0

// Define stepper motor connections and steps per revolution:
#define BUZZER_PIN 2
#define SLEEP_PIN 3
#define STEP_PIN 4
#define DIR_PIN 5
#define IR_PIN 6
#define EN_PIN 7

#define IR_BUTTON_OK 28
#define IR_BUTTON_UP 24
#define IR_BUTTON_DOWN 82
#define IR_BUTTON_LEFT 8
#define IR_BUTTON_RIGHT 90
#define IR_BUTTON_ASTERISK 22
#define IR_BUTTON_POUND 13

#define STEPS_PER_REV 200
#define SPEED_DELAY 5000
#define TOTAL_REVS 2.6
#define TOTAL_STEPS (STEPS_PER_REV * TOTAL_REVS)

IRrecv irrecv(IR_PIN);
long lastBuzzerMillis = 0;
int buzzerState = LOW;
bool isShredded = false;
bool isMuted = false;
int location = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial); //wait until ready

  isShredded = getShreddedState();
  isMuted = getMutedState();
  location = getLocationState();
  
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); // Enable driver (LOW means Enable)

  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, HIGH);

  pinMode(STEP_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);

  pinMode(SLEEP_PIN, OUTPUT);
  digitalWrite(SLEEP_PIN, LOW);

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
  
  // Keep track of how long the IR receiver has been stopped
  int startMillis = millis();
  IrReceiver.stop();

  // If already in the shredded state, run the main sequence in reverse
  bool isSequenceReversed = isShredded;
  
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

void resetLocationState() {
  location = 0;
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
  setShreddedState(false);
  int elapsedMicros = (millis() - startMillis) * 1000;
  IrReceiver.start(elapsedMicros);
  resetLocationState();
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
    delayMicroseconds(SPEED_DELAY);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(SPEED_DELAY);
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
    delayMicroseconds(SPEED_DELAY);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(SPEED_DELAY);
    
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
  EEPROM.write(EEPROM_ADDRESS, isShredded);
}

bool getShreddedState() {
  return EEPROM.read(EEPROM_ADDRESS);
}

void setMutedState(bool newMutedState) {
  isMuted = newMutedState;
  EEPROM.write(EEPROM_ADDRESS+1, isMuted);
}

bool getMutedState() {
  return EEPROM.read(EEPROM_ADDRESS+1);
}

void setLocationState(int newLocationState) {
  location = newLocationState;
  writeIntIntoEEPROM(EEPROM_ADDRESS+2, location);
}

int getLocationState() {
  return readIntFromEEPROM(EEPROM_ADDRESS+2);
}

void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address)
{
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}
