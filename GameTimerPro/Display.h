#ifndef DISPLAY_H
#define DISPLAY_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>

#define HT16K33_SYSTEM_CMD 0x20
#define HT16K33_SYSTEM_OSC_ON 0x01
#define HT16K33_DISPLAY_CMD 0x80
#define HT16K33_DISPLAY_OFF 0x00
#define HT16K33_DISPLAY_ON 0x01
#define HT16K33_BLINK_OFF 0
#define HT16K33_BLINK_2HZ  1
#define HT16K33_BLINK_1HZ  2
#define HT16K33_BLINK_HALFHZ  3
#define HT16K33_INT_CMD 0xA0
#define HT16K33_INT_ACTIVE_LOW 0x01
#define HT16K33_INT_ACTIVE_HIGH 0x03
#define HT16K33_BRIGHTNESS_CMD 0xE0
#define MAX_BRIGHTNESS 15

class Display {
 public:
  Display(void);
  uint16_t buffer[8]; 
  void begin();
  void init(uint8_t a);

  void setOn();
  void setOff();
  void setBrightness(uint8_t b);
  void setBlinkRate(uint8_t b);
  void update(void);
  void clear(void);
  void setDigitNum(uint8_t d, uint8_t n);  // set a particular digit to a number
  void setDigitChar(uint8_t d, uint8_t c); // set a particular digit to a char
  void printNumShift(uint16_t n); // set rightmost digit to number, scrolling current display to the left
  void printCharShift(uint8_t c); // set rightmost digit to char, scrolling current display to the left
  void printNum(uint16_t n, boolean leadingZeros = false); // print number to the display.
  void printString(char *s); // print 4-character string to display
  void printStringScroll(char *s, uint16_t delay); // print a string that scrolls from right to left with specified delay
  void printTime(long t); // t expressed as seconds of the day
  void printCountdown(int t); // t expressed as seconds
  void setDisplayColon(boolean state);
  void setDigitRaw(uint8_t d, uint16_t bitmask);
  void setLED(uint8_t led, uint8_t val);
  void fadeIn(int d);
  void fadeOut(int d);
    
 private:
  uint8_t readFlag();
  void error(uint16_t n);
  void error(char *s);
  void shiftLeft();
  uint8_t colonMask = 0;
  uint8_t ledMask = 0;
};

extern Display display;

#endif // DISPLAY_H

