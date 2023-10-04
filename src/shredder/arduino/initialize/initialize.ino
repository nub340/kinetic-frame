#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_ADDRESS 0

void setup() {
  Serial.begin(115200);
  while (!Serial); //wait until ready

  setShreddedState(false);
  setMutedState(false);
  setLocationState(0);
  //setRPMState(50);

  writeStateToSerial();
}

void loop() {
}

void writeStateToSerial() {
  bool isShredded = getShreddedState();
  bool isMuted = getMutedState();
  bool location = getLocationState();
//  int current_rpm = getRPMState();
  
  Serial.print("Muted: ");
  Serial.print(isMuted);
  Serial.print(", Shredded: ");
  Serial.print(isShredded);
  Serial.print(", Location: ");
  Serial.print(location);
//  Serial.print(", RPM: ");
//  Serial.print(current_rpm);
  Serial.println("");
}

void setShreddedState(bool newShreddedState) {
  EEPROM.put(EEPROM_ADDRESS, newShreddedState);
}

bool getShreddedState() {
  bool shredded;
  EEPROM.get(EEPROM_ADDRESS, shredded);
  return shredded;
}

void setMutedState(bool newMutedState) {
  EEPROM.put(EEPROM_ADDRESS+1, newMutedState);
}

bool getMutedState() {
  bool isMuted;
  EEPROM.get(EEPROM_ADDRESS+1, isMuted);
  return isMuted;
}

void setLocationState(int newLocationState) {
  EEPROM.put(EEPROM_ADDRESS+2, newLocationState);
}

int getLocationState() {
  int location;
  EEPROM.get(EEPROM_ADDRESS+2, location);
  return location;
}

void setRPMState(int rpm) {
  EEPROM.put(EEPROM_ADDRESS+2+sizeof(int), rpm);
}

int getRPMState() {
  int rpm;
  EEPROM.get(EEPROM_ADDRESS+2+sizeof(int), rpm);
  return rpm;
}
