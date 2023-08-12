#include <IRremote.hpp>
#include <EEPROM.h>
#include <TMCStepper.h>         // TMCstepper - https://github.com/teemuatlut/TMCStepper
#include <SoftwareSerial.h>     // Software serial for the UART to TMC2209 - https://www.arduino.cc/en/Reference/softwareSerial
#include <Streaming.h>          // For serial debugging output - https://www.arduino.cc/reference/en/libraries/streaming/

#define EEPROM_ADDRESS 0  // For persisting state between power cycling

#define BUZZER_PIN  2 // Buzzer
#define TX_PIN      3 // SoftwareSerial transmit pin, connect to RX of TMC2209 Driver
#define RX_PIN      4 // SoftwareSerial receive pin, connect to TX of TMC2209 Driver
#define DIR_PIN     5 // Direction
#define IR_PIN      6 // IR Receiver Signal
#define EN_PIN      7 // Enable

#define DRIVER_ADDRESS 0b00   // TMC2209 Driver address according to MS1 and MS2

#define IR_BUTTON_OK        28
#define IR_BUTTON_UP        24
#define IR_BUTTON_DOWN      82
#define IR_BUTTON_ASTERISK  22
#define IR_BUTTON_POUND     13

#define STEPS_PER_REV   200
#define SPEED_DELAY     5000
#define TOTAL_REVS      2.6
#define TOTAL_STEPS     (STEPS_PER_REV * TOTAL_REVS)

SoftwareSerial SoftSerial(RX_PIN, TX_PIN);
TMC2209Stepper TMCdriver(&SoftSerial, R_SENSE, DRIVER_ADDRESS);   // Create TMC driver

IRrecv irrecv(IR_PIN);
long lastBuzzerMillis = 0;
int buzzerState = LOW;
bool isShredded = false;
bool isMuted = false;
int location = 0;

void setup() {
  Serial.begin(9600);
  SoftSerial.begin(115200);
  TMCdriver.beginSerial(115200);      // Initialize UART
  
  while (!Serial || !SoftwareSerial); //wait until ready

  isShredded  = getShreddedState();
  isMuted     = getMutedState();
  location    = getLocationState();
  
  pinMode(EN_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  digitalWrite(EN_PIN, LOW); // Enable driver (LOW means Enable)
  digitalWrite(DIR_PIN, HIGH);

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  TMCdriver.begin();                                                                                                                                                                                                                                                                                                                            // UART: Init SW UART (if selected) with default 115200 baudrate
  TMCdriver.toff(5);                 // Enables driver in software
  TMCdriver.rms_current(500);        // Set motor RMS current
  TMCdriver.microsteps(256);         // Set microsteps

  TMCdriver.en_spreadCycle(false);
  TMCdriver.pwm_autoscale(true);     // Needed for stealthChop

  Serial.println("Running...");
  if (location > 0){
    Serial.println("Recovering...");
    stepReverse(location, false);
  } else if (location < 0) {
    Serial.println("Recovering...");
    stepForward(-location, false);
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
    Serial.println(command);
  }
  switch (command) {
    case IR_BUTTON_UP:
      stepReverse(5, false);
      break;

    case IR_BUTTON_DOWN:
      stepForward(5, false);
      break;

    case IR_BUTTON_OK:
      runMainSequence();
      break;

    case IR_BUTTON_ASTERISK:
      togglePictureState();
      break;

    case IR_BUTTON_POUND:
      toggleMute();
      break;
  }
}

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

void toggleMute() {
  setMutedState(!isMuted);
  int startMillis = millis();
  IrReceiver.stop();
  tone(BUZZER_PIN, 3000);
  delay(500);
  noTone(BUZZER_PIN);
  int elapsedMicros = (millis() - startMillis) * 1000;
  IrReceiver.start(elapsedMicros);
}

void togglePictureState() {
  if (isShredded) {
    enterNormalState();
  } else {
    enterShreddedState();
  }
}

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
    setLocationState(location+1);
  }
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
    setLocationState(location-1);
  }
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
