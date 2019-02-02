/**
   This file contains any methods and variables used for the rotary encoder
*/
//The left, interrupt, pin
const int pinA = D2;
//The right (non interrupt) pin
const int pinB = D3;
//The pin for the pushbutton of the encoder
const int pinButton = D1;
//The direction of the turn (1= none, 2 = right, 0 = left)
volatile byte encoderDir = 1;
//Amount of updates to ignore any new presses in.
#define BUTTON_DEBOUNCE 50
//How many updates of irresponsiveness are left untill we trigger presses again
byte buttonTimeout = 0;

/**
   Handles the initialization of the encoder stuff
*/
void initEncoder() {
  //Set pinmodes
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  pinMode(pinButton, INPUT_PULLUP);
  //Attach an interrupt to the first pin
  attachInterrupt(digitalPinToInterrupt(pinA), encoderChange, RISING);
}

/**
   Returns if the rotary encoder has moved since last reading
*/
bool hasEncoderChanged() {
  return encoderDir != 1;
}

/**
   Returns the amount of change the encoder has had since its last reading
*/
int readEncoder() {
  //Backup the current direction for this function
  int temp = encoderDir;
  //Now reset the direction to none
  encoderDir = 1;
  //And return using the reference we still have
  return temp - 1;
}

/**
   Returns true if a button press was detected, returns false if nothing was detected. Only detects
   the first rising edge, then nothing for a bit.
*/
bool checkEncoderButton() {
  if (digitalRead(pinButton) == LOW && buttonTimeout == 0) {
    //Set the timeout to the mentioned value
    buttonTimeout = BUTTON_DEBOUNCE;
    //This counts as a press
    return true;
  } else if (digitalRead(pinButton) == HIGH) {
    //Remove a little bit from the timeout untill it's gone
    if (buttonTimeout > 0) buttonTimeout--;
  }
  //If we didn't match any condition, return false
  return false;
}

/**
   Interrupt used for the rotary encoder
*/
void encoderChange() {
  //If they are the same, that is one rotaiton one way
  if ((digitalRead(pinA) == LOW) != (digitalRead(pinB) == LOW)) {
    encoderDir = 0;
  } else {
    encoderDir = 2;
  }
}
