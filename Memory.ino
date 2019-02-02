/**
   This file contains all methods containing any EEPROM stuff
*/

//The location in EEPROM of the alarmHours
const int alarmHoursAdress = 0;
//The location in EEPROM of the alarmMinutes
const int alarmMinutesAdress = 1;
//The location in EEPROM of the alarmStatus (on/off)
const int alarmStatusAdress = 2;

/**
  Starts the read from the EEPROM and checks if any settings
  we're left last time
*/
void initEEPROM() {
  //First start reading EEPROM
  EEPROM.begin(512);
  //Now read alarmHours
  byte aHours = EEPROM.read(alarmHoursAdress);
  //And alarmMinutes
  byte aMins = EEPROM.read(alarmMinutesAdress);
  //And finally alarm status
  byte aStatus = EEPROM.read(alarmStatusAdress);

  //And parse this shit
  alarmHours = constrain(aHours, 0, 23);
  alarmMinutes = constrain(aMins, 0, 59);
  alarmOn = aStatus == 1;

  //Turn the led on/off depending on alarm status
  digitalWrite(pinLED, (alarmOn ? HIGH : LOW));
}

/**
   Saves the alarmHours to EEPROM
*/
void saveAlarmHours() {
  //Writes the value to the right adress, only if it has changed
  if (EEPROM.read(alarmHoursAdress) != alarmHours) {
    EEPROM.put(alarmHoursAdress, alarmHours);
  }
  //And commit the change
  EEPROM.commit();
}

/**
   Saves the alarmMinutes to EEPROM
*/
void saveAlarmMinutes() {
  //Writes the value to the right adress, only if it has changed
  if (EEPROM.read(alarmMinutesAdress) != alarmMinutes) {
    EEPROM.put(alarmMinutesAdress, alarmMinutes);
  }
  //And commit the change
  EEPROM.commit();
}

/**
   Saves the alarmStatus to EEPROM
*/
void saveAlarmStatus() {
  //Writes the value to the right adress, only if it has changed
  if ((EEPROM.read(alarmStatusAdress) == 1) != alarmOn) {
    EEPROM.put(alarmStatusAdress, alarmOn);
  }
  //And commit the change
  EEPROM.commit();
}
