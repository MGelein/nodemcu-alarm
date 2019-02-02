/**
 * This file contains all the methods for the Piezo buzzer to make sound
 */
 
//Starting frequency for the first sound
#define FREQ1 80
//Starting frequency for the second sound
#define FREQ2 1000
//Starting delay for the first beep
#define BEEP_DELAY 1000
//The frequency the buzzer first makes
int freq1 = FREQ1;
//The frequency the buzzer then makes
int freq2 = FREQ2;
//The amount of initial silence between beeps
int beepDelay = BEEP_DELAY;
//Amount of Hz to increase every beep
byte freqStep = 50;
//Amount of time to have less delay every beep
byte timeStep = 50;
//The pin for the buzzer
const int pinBuzzer = D7;


/**
   Restores the buzzer variables to their default values
*/
void resetAlarmBuzzer() {
  freq1 = FREQ1;
  freq2 = FREQ2;
  beepDelay = BEEP_DELAY;
}

/**
   Makes the screeching sound when the alarm is
*/
void checkScreeching() {
  //Take a look at the current time
  unsigned long now = millis();
  //See if we are beeping
  if (now - beepStart < 200) {
    tone(pinBuzzer, freq1, 5);//850
    delay(5);
    tone(pinBuzzer, freq2, 5);//1800
    delay(5);
  } else if (now > beepStart) {
    beepStart = now + beepDelay;
    if (freq1 < 880) {
      freq1 += freqStep;
      freq2 += freqStep;
      beepDelay -= timeStep;
    }
  }
}
