#ifndef KEYPAD_H
#define KEYPAD_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>

//#define ADAFRUIT
#define GENERIC

#define KEY_0 0
#define KEY_1 1
#define KEY_2 2
#define KEY_3 3
#define KEY_4 4
#define KEY_5 5
#define KEY_6 6
#define KEY_7 7
#define KEY_8 8
#define KEY_9 9
#define KEY_STAR 10
#define KEY_POUND 11

class Keypad {
 public:
  Keypad(void);
  int getKey();
  void clear();
    
 private:
  void readKeypad();
  uint8_t lastKeyMem[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t keyMem[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
};

extern Keypad keypad;

#endif // KEYPAD_H