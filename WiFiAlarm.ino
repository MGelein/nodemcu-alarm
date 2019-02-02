#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

/**
   WIFI STUFF
*/
//The wifi multi AP object
ESP8266WiFiMulti WiFiMulti;
//The wifi client that will handle the transfer
WiFiClient client;
//The HTTP get protocol object
HTTPClient http;
//Amount of minutes between checks
const byte ONLINE_CHECK_INTERVAL = 15;

/**
   Mode stuff
*/
//Only displays the current time
const byte DISPLAY_MODE = 0;
//Editing the hour mode
const byte HOUR_MODE = 1;
//Editing the minute mode
const byte MINUTE_MODE = 2;
//The current clock mode
byte clockMode = DISPLAY_MODE;


//The pin for the alarm LED
const int pinLED = D0;

/**
   Time keeping variables
*/
//First time on booting, check the time twice during the first minute
bool firstBoot = true;
//Amount of hours that have passed
byte hours = 0;
//Alarm hours
byte alarmHours = 0;
//Amount of minutes that have passed
byte minutes = 0;
//Alarm minutes
byte alarmMinutes = 0;
//Amount of seconds that have passed
byte seconds = 0;
//Variable to store the amount of time passed since last timestamp
int timepassed = 0;
//The point in time our last second update happened
unsigned long timestamp = 0;
//If the alarm is currently on
bool alarmOn = false;
//The value of the alarmRotation
int alarmVal = 0;
//Set the max amount for the alarm movement, increase to require more movement to SET alarm
#define ALARM_ON 2
//Set the max amount for the alarm movenennt, increase to require more movent to UNSET alarm (during sounding it is already x2)
#define ALARM_OFF -6
//If the alarm is sounding, this is true, if not, it's not
bool alarmSounding = false;
//The amount of snoozetime left. Gets set to 5 if its ht when the alarm sounds
byte snoozeTime = 0;
//The amount of times we have snoozed, gets reset if we turn off the alarm
byte timesSnoozed = 0;
//If the menu timeout reaches the menu timeout threshold, it resets to display mode
int menuTimeout = 0;
//100 updates / second, means 10 seconds for 1000 updates
#define MENU_TIMEOUT 1000

/**
 * Blinking stuff
 */
 //If we're hiding the info right now
bool isBlink = false;
//The amount of time we've spent on a period
byte blinkTime = 0;

/**
   Initial starting point of the code
*/
void setup() {
  //Start a Serial communication
  Serial.begin(115200);
  //Set pinmode for alarm led
  pinMode(pinLED, OUTPUT);
  //Initialize the screen
  initScreen();
  //Init wifi
  initWifi();
  //Init encoder stuff  
  initEncoder();
  //Read EEProm to see if stuff was saved
  initEEPROM();
}

/**
   Runs the code repeatedly, as often as possible
*/
void loop() {
  //Update the blink timer (for the blinking numbers)
  updateBlink();

  //Handle time keeping as often as possible
  checkTime();

  //Switch modes on a button press
  if (checkEncoderButton()) {
    //Reset the timeout timer for the menu
    menuTimeout = 0;
    //If the alarm is sounding and we have no snooze, hit it to snooze
    if (alarmSounding && snoozeTime == 0) {
      //Increment the amount of times snoozed
      timesSnoozed ++;
      //Give us 5 more minutes, the first time
      if (timesSnoozed == 1) snoozeTime = 5;
      else if (timesSnoozed == 2) snoozeTime = 4;
      else if (timesSnoozed == 3) snoozeTime = 3;
      else if (timesSnoozed == 4) snoozeTime = 2;
      else if (timesSnoozed == 5) snoozeTime = 1;
      //If timesSnoozed is more, nothing happens anymore

      //If we have snoozed, also reset the alarm
      if (snoozeTime > 0) resetAlarmBuzzer();
    } else {
      //Depending on the clockMode we have before changing, save to EEPROM
      if (clockMode == HOUR_MODE) saveAlarmHours();
      else if (clockMode == MINUTE_MODE) saveAlarmMinutes();
      //Circle the modes with a modulus
      clockMode = (clockMode + 1) % 3;
      //Also reset the blinking for now
      if (clockMode == DISPLAY_MODE) displayTime(false, false);
      //Stop blinking for a bit
      blinkTime = 0;
      //Reset the snooze
      snoozeTime = 0;
    }
  }

  //Check the menuTimeout
  if (clockMode != DISPLAY_MODE) {
    menuTimeout ++;
    //After timeout, reset
    if (menuTimeout > MENU_TIMEOUT) {
      //Save the values
      if (clockMode == HOUR_MODE) saveAlarmHours();
      else if (clockMode == MINUTE_MODE) saveAlarmMinutes();
      //Reset to display mode
      clockMode = DISPLAY_MODE;
      //And reset the timeout
      menuTimeout = 0;
      //Return early, so the loop runs again ASAP
      return;
    }
  }

  //Check how to handle the encoder
  if (hasEncoderChanged()) {
    //In hour mode, change hours
    if (clockMode == HOUR_MODE) {
      //Add the amount to the hours
      alarmHours += readEncoder();
      //And make sure it loops back properly, >200 only happens on overflow
      if (alarmHours > 200) alarmHours = 23;
      else if (alarmHours > 23) alarmHours = 0;
    } else if (clockMode == MINUTE_MODE) {
      //Add the amount to the mintes
      alarmMinutes += readEncoder();
      //And make sure it loops back properly, >200 only happens on overflow
      if (alarmMinutes > 200) alarmMinutes = 59;
      else if (alarmMinutes > 59) alarmMinutes = 0;
    } else if (clockMode == DISPLAY_MODE) {
      //Now the rotary encoder turns of/on the alarm
      alarmVal += readEncoder();
      if (alarmVal > ALARM_ON) {
        alarmVal = 0;
        alarmOn = true;
        resetAlarmBuzzer();
        saveAlarmStatus();
      }
      else if (alarmVal < (alarmSounding ? ALARM_OFF * 3 : ALARM_OFF)) { //During alarm it's three times as hard to turn it off.
        alarmVal = 0;
        alarmOn = false;
        timesSnoozed = 0;
        snoozeTime = 0;
        alarmSounding = false;
        saveAlarmStatus();
      }

      //Turn the led on/off depending on alarm status
      digitalWrite(pinLED, (alarmOn ? HIGH : LOW));
    }
  }

  //Now handle the modes appropriately
  if (clockMode == HOUR_MODE) {
    //Editing the hour of the alarm
    displayTime(isBlink, false);
  } else if (clockMode == MINUTE_MODE) {
    //Editing the minute of the alarm
    displayTime(false, isBlink);
  }

  //If there is an alarm, the sounding will act as a delay, limiting fps
  if (snoozeTime == 0 && alarmSounding) checkScreeching();
  //Wait 10ms, run at approximately 100fps
  else delay(10);
}

/**
   Sets up a network connection
*/
void initWifi() {
  //Prepare the serial port, show pretty start animation
  byte delayLength = 100;
  for (uint8_t t = 4; t > 0; t--) {
    //Frame 1
    hours = 80;
    minutes = 0;
    displayTime(false, false);
    delay(delayLength);
    //Frame 3
    hours = 8;
    displayTime(false, false);
    delay(delayLength);
    //Frame 4
    minutes = 80;
    hours = 0;
    displayTime(false, false);
    delay(delayLength);
    //Frame 5
    minutes = 8;
    displayTime(false, false);
    delay(delayLength);
    //Frame 6
    minutes = 80;
    hours = 0;
    displayTime(false, false);
    delay(delayLength);
    hours = 8;
    minutes = 0;
    displayTime(false, false);
    delay(delayLength);
  }
  //Reset to no time (AAAA)
  hours = minutes = seconds = 11;
  displayTime(false, false);
  //Access the wifi
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("SSID", "PASS");
  //get the time data from the online api
  getTime();
}

/**
  Retrieves the time from an online api
 **/
void getTime() {
  //Wait for a connection
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.println("Waiting for wifi...");
    delay(1000);
  }
  //When we finally have a connection, continue
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    //Get the data from the world time api

    if (http.begin(client, "http://worldtimeapi.org/api/ip.txt")) {
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error, so if it is >0 we can continue
      if (httpCode > 0) {
        //If we find a file, or the file has been moved, echo the payload
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          parsePayload(http.getString());
        }
      } else {
        //Log the error string
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
}

/**
   Parses the provided payload from the time server
*/
void parsePayload(String data) {
  //Example date: "datetime: 2019-01-26T23:49:58.329505+01:00"
  //Isolate only the dateline from the output
  String dateLine = data.substring(data.indexOf("\n"), data.indexOf("\n", data.indexOf("\n") + 1));
  dateLine = dateLine.substring(dateLine.indexOf("T") + 1, dateLine.indexOf("."));
  //Now, split by colons to find the hours, minutes and seconds
  int firstColon = dateLine.indexOf(":");
  int secondColon = dateLine.indexOf(":", firstColon + 1);
  //Parse the dateLine string
  hours = dateLine.substring(0, firstColon).toInt();
  minutes = dateLine.substring(firstColon + 1, secondColon).toInt();
  seconds = dateLine.substring(secondColon + 1, dateLine.length()).toInt();
  //Print the result to the serial line
  Serial.println(String(hours) + ":" + String(minutes) + ":" + String(seconds));
}

/**
   Checks the passage of time
*/
void checkTime() {
  //Calculate how much time has passed
  timepassed = millis() - timestamp;

  //Only happens in case of unsigned long overflow
  if (timepassed < 0) {
    //Reset timestamp if the millis() has overflown, not ideal, but will at least fix for now
    timestamp = millis();
  }

  //If the timepassed is over a second, increase the timestamp by 1000
  if (timepassed >= 1000) {
    timestamp += 1000;
    seconds += 1;
    //Loop back time, and overflow into next unit of time
    if (seconds == 60) {
      seconds = 0;
      minutes++;
      //If we add a minute, also remove snoozeTime, if appropriate
      if (snoozeTime > 0) snoozeTime --;
      //Only during the boot procedure get time again
      if (firstBoot && ((minutes + 1) % ONLINE_CHECK_INTERVAL) != 0) {
        //Set the flag to false again
        firstBoot = false;
        getTime();
      }
    }
    //Sync every 15 minutes
    if ((minutes + 1) % ONLINE_CHECK_INTERVAL == 0 && seconds == 0) getTime();
    //Then continue time keeping
    if (minutes == 60) {
      minutes = 0;
      hours++;
    }
    if (hours == 24) {
      hours = 0;
    }
    //Finally display time every second
    if (clockMode == DISPLAY_MODE) displayTime(false, false);
    //And toggle the point
    toggleColon();

    //Log the current time
    Serial.print(String(hours) + ":" + String(minutes) + ":" + String(seconds));
    if (alarmOn) {
      Serial.print(" and alarm on " + String(alarmHours) + ":" + String(alarmMinutes));
      if (snoozeTime > 0) Serial.println("+" + String(snoozeTime));
      else Serial.println("");
    } else {
      //Finish the line
      Serial.println("");
    }

    //Now check the alarm
    if (clockMode == DISPLAY_MODE && hours == alarmHours && minutes == alarmMinutes) {
      if (alarmOn && !alarmSounding) {
        //Start sounding the alarm and ignore all other variables
        alarmSounding = true;
        snoozeTime = 0;
        timesSnoozed = 0;
        //Also switch  back to main display mode, because we need to do that
        clockMode = DISPLAY_MODE;
      }
    }
  }
}
