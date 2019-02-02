#include <TM1637.h>;
/**
   This file holds all the necessary variables and methods to interface with the SevenSegment display
*/
//The character for a clear (nothing on the screen);
const int8_t CLEAR = 0x7f;
//The screen instance
TM1637 screen(D6, D5);
//The numbers on the screen
int8_t displayNumbers[] = {0, 0, 0, 0};
//Boolean to see if the colon is on
bool colonOn = true;
//Half the period of a blink
byte blinkPeriod = 15;


/**
   Initializes the display
*/
void initScreen() {
  //Set brightness to default
  screen.set();
  //Initialize the screen
  screen.init();
  //Show the initial time
  displayTime(false, false);
}

/**
 * Updates the blinking timer, used in editing alarm hours and minutes
 */
void updateBlink(){
  //Check the blinktimer
  blinkTime ++;
  isBlink = blinkTime > blinkPeriod;
  if (blinkTime > blinkPeriod * 2) blinkTime = 0;
}

/**
   Toggles the colon on the display
*/
void toggleColon() {
  //Toggle the value
  colonOn = !colonOn;
  //Set the point in the screen
  screen.point(colonOn ? POINT_ON : POINT_OFF);
}

/**
   Show the current time HH:MM on the 4 digit display
*/
void displayTime(bool hideHours, bool hideMinutes) {
  //Check what time or alarm to display
  byte h = (clockMode == DISPLAY_MODE) ? hours : alarmHours;
  byte m = (clockMode == DISPLAY_MODE) ? minutes : alarmMinutes;

  //Calculate the numbers that go in each place
  displayNumbers[0] = hideHours ? CLEAR : h / 10;
  displayNumbers[1] = hideHours ? CLEAR : h % 10;
  displayNumbers[2] = hideMinutes ? CLEAR : m / 10;
  displayNumbers[3] = hideMinutes ? CLEAR : m % 10;

  //Now finally show the number on the screen
  screen.display(displayNumbers);
}
