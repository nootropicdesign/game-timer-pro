#include "Hardware.h"
#include "Config.h"
#include "Input.h"
#include "Display.h"
#include "Keypad.h"

byte buttonPins[4] = {BUTTON_LEFT_PIN, BUTTON_RIGHT_PIN, BUTTON_SELECT_PIN, BUTTON_DET_PIN};
byte buttonState[4] = {HIGH, HIGH, HIGH, HIGH};
unsigned long buttonChange[4] = {0L, 0L, 0L, 0L};
unsigned long buttonLastPress[4] = {0L, 0L, 0L, 0L};

// return true if the button is pressed.
boolean buttonPressed(byte button) {
  if (digitalRead(buttonPins[button]) == LOW) {
    // the button is currently pressed
    if (buttonState[button] == HIGH) {
      // if the button was not pressed before, update the state.
      buttonChange[button] = millis();
      buttonLastPress[button] = millis();
      buttonState[button] = LOW;
    }
    return true;
  } else {
    // The button is currently not pressed
    if (buttonState[button] == LOW) {
      // if the button was pressed before, update the state.
      buttonChange[button] = millis();
      buttonState[button] = HIGH;
    }
    return false;
  }
}

// return true if the button is pressed and it is a new press (not held)
boolean buttonPressedNew(byte button) {
  if (digitalRead(buttonPins[button]) == LOW) {
    // The button is currently pressed
    if (buttonState[button] == HIGH) {
      // This is a new press.
      buttonChange[button] = millis();
      buttonLastPress[button] = millis();
      buttonState[button] = LOW;
      return true;
    }
    // This is not a new press.
    return false;
  } else {
    // The button is currently not pressed
    if (buttonState[button] == LOW) {
      buttonChange[button] = millis();
      buttonState[button] = HIGH;
    }
    return false;
  }
}

// return true if the button is pressed and has been held for at least n milliseconds
boolean buttonHeld(byte button, int n) {
  if (digitalRead(buttonPins[button]) == LOW) {
    // the button is currently pressed
    if (buttonState[button] == HIGH) {
      // if the button was not pressed before, update the state and return false.
      buttonChange[button] = millis();
      buttonLastPress[button] = millis();
      buttonState[button] = LOW;
      return false;
    }
    if ((millis() - buttonChange[button]) >= n) {
      // the button has been pressed for over n milliseconds.
      // update the state change time even though the state hasn't changed.
      // we update the state change time so we can start the counting over
      buttonChange[button] = millis();
      return true;
    }
    // The button is being held, but has not been held for longer than n milliseconds.
    return false;
  } else {
    // The button is currently not pressed
    if (buttonState[button] == LOW) {
      // if the button was pressed before, update the state.
      buttonChange[button] = millis();
      buttonState[button] = HIGH;
    }
    return false;
  }
}

unsigned long getLastPress(byte button) {
  return buttonLastPress[button];
}

long inputTime(long t) {
  // t expressed as seconds of the day
  byte d = 150;
  display.printTime(t);
  while (true) {
    if (buttonPressedNew(BUTTON_DET)) {
      // escape without changing
      display.setLED(LED2, LOW);
      return -1;
    }
    if (buttonPressedNew(BUTTON_SELECT)) {
      display.setLED(LED2, LOW);
      return t;
    }

    if (buttonPressedNew(BUTTON_LEFT) || buttonHeld(BUTTON_LEFT, d)) {
      if (millis() - buttonLastPress[BUTTON_LEFT] > 2000) {
        // button held for more than 2 seconds
        d = 10;
      } else {
        d = 150;
      }
      t -= 60;
      if (t < 0) {
        t = SECONDS_PER_DAY-60;
      }
      if ((t >= SECONDS_AT_NOON) && (config.get(CLOCK24HR) == CLOCK24HR_OFF)) {
        display.setLED(LED2, HIGH);
      } else {
        display.setLED(LED2, LOW);
      }
      display.printTime(t);
    }
    if (buttonPressedNew(BUTTON_RIGHT) || buttonHeld(BUTTON_RIGHT, d)) {
      if (millis() - buttonLastPress[BUTTON_RIGHT] > 2000) {
        // button held for more than 2 seconds
        d = 10;
      } else {
        d = 150;
      }
      t += 60;
      if (t >= SECONDS_PER_DAY) {
        t = 0;
      }
      if ((t >= SECONDS_AT_NOON) && (config.get(CLOCK24HR) == CLOCK24HR_OFF)) {
        display.setLED(LED2, HIGH);
      } else {
        display.setLED(LED2, LOW);
      }
      display.printTime(t);
    }
    if ((!buttonPressed(BUTTON_LEFT)) && (!buttonPressed(BUTTON_RIGHT))) {
      display.setLED(LED2, LOW);
    }
    delay(20);
  }

}

// Get button or keypad input
int inputCode() {
  int key;
  int toggleCount = 0;
  uint8_t currentDigit = 1;
  uint8_t digits[4]; // left to right
  int currentValue = UNSET;
  display.setDisplayColon(false);
  display.printString("____");
  while (currentDigit <= 4) {
    if (buttonPressedNew(BUTTON_DET)) {
      return UNSET;
    }
    // if no current value yet, toggle an underscore
    if (currentValue == UNSET) {
      toggleCount++;
      if ((toggleCount % 10) == 0) {
        if ((toggleCount % 20) == 0) {
          display.setDigitChar(currentDigit, '_');
        } else {
          display.setDigitChar(currentDigit, ' ');
        }
      }
    }

    if (buttonPressedNew(BUTTON_LEFT)) {
      if (currentValue == UNSET) {
        currentValue = 0;
      } else {
        if (currentValue == 0) {
          currentValue = 9;
        } else {
          currentValue--;
        }
      }
      display.setDigitNum(currentDigit, currentValue);
    }

    if (buttonPressedNew(BUTTON_RIGHT)) {
      if (currentValue == UNSET) {
        currentValue = 0;
      } else {
        if (currentValue == 9) {
          currentValue = 0;
        } else {
          currentValue++;
        }
      }
      display.setDigitNum(currentDigit, currentValue);
    }

    if (buttonPressedNew(BUTTON_SELECT)) {
      if (currentValue != UNSET) {
        digits[currentDigit-1] = currentValue;
        currentDigit++;
        currentValue = UNSET;
        toggleCount = 0;
      }
    }

    key = keypad.getKey();
    if ((key > -1) && (key <= 9)) {
      display.setDigitNum(currentDigit, key);
      digits[currentDigit-1] = key;
      currentDigit++;
      currentValue = -1;
      toggleCount = 0;
    }
    delay(20);
  }
  return (digits[0] * 1000) + (digits[1] * 100) + (digits[2] * 10) + digits[3];
}

boolean wireConflict(uint8_t index, int value, uint8_t *values) {
  boolean retVal = false;
  switch (index) {
    case 1:
    retVal = (values[1] == value) || (values[2] == value) || (values[3] == value);
    break;
    case 2:
    retVal = (values[0] == value) || (values[2] == value) || (values[3] == value);
    break;
    case 3:
    retVal = (values[0] == value) || (values[1] == value) || (values[3] == value);
    break;
    case 4:
    retVal = (values[0] == value) || (values[1] == value) || (values[2] == value);
    break;
  }
  return retVal;
}


// Get button or keypad input
int inputWireOrder() {
  int key;
  int toggleCount = 0;
  uint8_t currentDigit = 1;
  uint8_t digits[4]; // left to right
  int currentValue = UNSET;
  display.setDisplayColon(false);
  display.printString("____");
  while (currentDigit <= 4) {
    if (buttonPressedNew(BUTTON_DET)) {
      return UNSET;
    }
    // if no current value yet, toggle an underscore
    if (currentValue == UNSET) {
      toggleCount++;
      if ((toggleCount % 10) == 0) {
        if ((toggleCount % 20) == 0) {
          display.setDigitChar(currentDigit, '_');
        } else {
          display.setDigitChar(currentDigit, ' ');
        }
      }
    }

    if (buttonPressedNew(BUTTON_LEFT)) {
      do {
        if (currentValue == UNSET) {
          currentValue = 1;
        } else {
          if (currentValue == 1) {
            currentValue = 4;
          } else {
            currentValue--;
          }
        }
      } while ((currentValue == UNSET) || (wireConflict(currentDigit, currentValue, digits)));
      display.setDigitNum(currentDigit, currentValue);
    }

    if (buttonPressedNew(BUTTON_RIGHT)) {
      do {
        if (currentValue == UNSET) {
          currentValue = 1;
        } else {
          if (currentValue == 4) {
            currentValue = 1;
          } else {
            currentValue++;
          }
        }
      } while ((currentValue == UNSET) || (wireConflict(currentDigit, currentValue, digits)));
      display.setDigitNum(currentDigit, currentValue);
    }

    if (buttonPressedNew(BUTTON_SELECT)) {
      if (currentValue != UNSET) {
        digits[currentDigit-1] = currentValue;
        currentDigit++;
        currentValue = UNSET;
        toggleCount = 0;
      }
    }

    key = keypad.getKey();
    if ((key > -1) && (key <= 4) && (!wireConflict(currentDigit, key, digits))) {
      display.setDigitNum(currentDigit, key);
      digits[currentDigit-1] = key;
      currentDigit++;
      currentValue = UNSET;
      toggleCount = 0;
    }
    delay(20);
  }
  return (digits[0] * 1000) + (digits[1] * 100) + (digits[2] * 10) + digits[3];
}


