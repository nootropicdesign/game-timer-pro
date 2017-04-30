#include <Wire.h>

#include "Hardware.h"
#include "Config.h"
#include "Display.h"

static const uint8_t numbers[] = {
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F, // 9
  0x77, // A
  0x7C, // b
  0x39, // C
  0x5E, // d
  0x79, // E
  0x71, // F
};





// characters that cannot be represented will be blank
static const uint8_t chars[] = {
  0x00, // space
  0x00, //
  0x22, // "
  0x00, // 
  0x00, // 
  0x00, // 
  0x00, // 
  0x02, // '
  0x00, // 
  0x00, // 
  0x00, // 
  0x00, // 
  0x00, // 
  0x40, // -
  0x00, // 
  0x00, // 
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F, // 9
  0x00, // 
  0x00, // 
  0x00, // 
  0x48, // =
  0x00, // 
  0x00, // 
  0x00, // 
  0x77, // A
  0x00, // 
  0x39, // C
  0x00, // 
  0x79, // E
  0x71, // F
  0x3D, // G
  0x76, // H
  0x06, // I
  0x0E, // J
  0x00, // 
  0x38, // L
  0x00, // 
  0x37, // N
  0x3F, // O
  0x73, // P
  0x00, // 
  0x00, // 
  0x6D, // S
  0x00, // 
  0x3E, // U
  0x00, // 
  0x00, // 
  0x00, // 
  0x00, // 
  0x00, // 
  0x00, //
  0x00, // 
  0x00, //
  0x00, // 
  0x08, // _
  0x00, // 
  0x5F, // a
  0x7C, // b
  0x58, // c
  0x5E, // d
  0x00, // 
  0x00, // 
  0x00, // 
  0x74, // h
  0x10, // i
  0x00, // 
  0x00, // 
  0x30, // l
  0x00, // 
  0x54, // n
  0x5C, // o
  0x00, // 
  0x00, // 
  0x50, // r
  0x00, // 
  0x78, // t
  0x1C, // u
  0x00, // 
  0x00, // 
  0x00, // 
  0x6E, // y
  0x00, // 
  0x00, // 
  0x06, // |
  0x00, // 
  0x00, // 
  0x00  // 
};


Display::Display(void) {
}


void Display::setBlinkRate(uint8_t rate) {
  if (rate > HT16K33_BLINK_HALFHZ) {
    rate = HT16K33_BLINK_OFF;
  } 
  Wire.beginTransmission(I2CADDR);
  Wire.write(HT16K33_DISPLAY_CMD | HT16K33_DISPLAY_ON | (rate << 1)); 
  Wire.endTransmission();
}

// valid range 0-15.
void Display::setBrightness(uint8_t b) {
  if (b > 15) b = 15;
  Wire.beginTransmission(I2CADDR);
  Wire.write(HT16K33_DISPLAY_CMD | HT16K33_DISPLAY_ON);
  Wire.endTransmission();
  Wire.beginTransmission(I2CADDR);
  Wire.write(HT16K33_BRIGHTNESS_CMD| (b));
  Wire.endTransmission();
}

void Display::setOn() {
  Wire.beginTransmission(I2CADDR);
  Wire.write(HT16K33_DISPLAY_CMD | HT16K33_DISPLAY_ON);
  Wire.endTransmission();
}

void Display::setOff() {
  Wire.beginTransmission(I2CADDR);
  Wire.write(HT16K33_DISPLAY_CMD | HT16K33_DISPLAY_OFF);
  Wire.endTransmission();
}

void Display::begin() {
  Wire.begin();

  Wire.beginTransmission(I2CADDR);
  Wire.write(HT16K33_SYSTEM_CMD | HT16K33_SYSTEM_OSC_ON); // turn on oscillator
  Wire.endTransmission();
  setBlinkRate(HT16K33_BLINK_OFF);
  setBrightness(MAX_BRIGHTNESS);
}


void Display::update(void) {
  Wire.beginTransmission(I2CADDR);
  Wire.write((uint8_t)0x00); // start at address $00

  //Wire.write(buffer[0] & 0xFF);    
  //Wire.write(buffer[0] >> 8);
  for (uint8_t i=0; i<8; i++) {
    Wire.write((buffer[i] & 0xFF) | colonMask);    
    Wire.write((buffer[i] >> 8));
  }
  Wire.endTransmission();
}


void Display::clear(void) {
  for (uint8_t i=DIGIT1; i<=DIGIT4; i++) {
    buffer[i] = 0;
  }
  update();
}

// d is 1-4
void Display::setDigitRaw(uint8_t d, uint16_t bitmask) {
  uint8_t index = DIGIT1 + (d-1);
  buffer[index] = bitmask;
}

void Display::setDisplayColon(boolean c) {
  if (c) {
    colonMask = (1 << 7);
  } else {
    colonMask = 0;
  }
}

void Display::setDigitNum(uint8_t d, uint8_t n) {
  setDigitRaw(d, numbers[n]);
  update();
}

void Display::setDigitChar(uint8_t d, uint8_t c) {
  if ((c < ' ') || (c > 127)) {
    printString("Err2");
    Serial.print("Error in setDigitChar: ");
    Serial.println(c);
    return;
  }
  setDigitRaw(d, chars[c - ' ']);
  update();
}

void Display::printNumShift(uint16_t n) {
  shiftLeft();
  setDigitNum(4, n);
}

void Display::printCharShift(uint8_t c) {
  shiftLeft();
  setDigitChar(4, c);
}

void Display::shiftLeft() {
  for(byte d=DIGIT1;d<DIGIT4;d++) {
    buffer[d] = buffer[d+1];
  }
}

void Display::printNum(uint16_t n, boolean leadingZeros) {
  uint8_t d;
  if (n > 9999) {
    printString("Err1");
    Serial.print("Error in printNum: ");
    Serial.println(n);
    return;
  }

  d = n % 10;
  setDigitRaw(4, numbers[d]);
  d = (n / 10) % 10;
  if (leadingZeros || (n >= 100) || (d > 0)) {
    setDigitRaw(3, numbers[d]);
  } else {
    setDigitRaw(3, 0x0);
  }
  d = (n / 100) % 10;
  if (leadingZeros || (n >= 1000) || (d > 0)) {
    setDigitRaw(2, numbers[d]);
  } else {
    setDigitRaw(2, 0x0);
  }
  d = (n / 1000) % 10;
  if (leadingZeros || (d > 0)) {
    setDigitRaw(1, numbers[d]);
  } else {
    setDigitRaw(1, 0x0);
  }
  update();
}

// print a time t expressed as seconds of the day
void Display::printTime(long t) {
  setDisplayColon(true);
  boolean leadingZero = true;    
  byte hours = t / 3600;
  if (config.get(CLOCK24HR) == CLOCK24HR_OFF) {
    leadingZero = false;
    hours = hours % 12;
    if (hours == 0) {
      hours = 12;
    }
  }
  byte minutes = (t % 3600) / 60;
  uint16_t displayTime = (hours*100) + minutes;
  printNum(displayTime, leadingZero);
}

void Display::printCountdown(int t) {
  setDisplayColon(true);
  printNum(((t / 60)*100) + (t % 60), true);
}

void Display::printString(char *s) {
  uint8_t len = strlen(s);
  if (len > 4) {
    printString("Err3");
  Serial.print("Error in printString: ");
  Serial.println(s);
    return;
  }
  for(uint8_t l=0;l<len;l++) {
    setDigitRaw(l+1, chars[s[l] - ' ']);
  }
  for(uint8_t l=len;l<=4;l++) {
    setDigitRaw(l+1, chars[' ']);
  }
  update();
}

void Display::printStringScroll(char *s, uint16_t d) {
  shiftLeft();
  uint8_t len = strlen(s);
  for(uint8_t i=0;i<len;i++) {
    printCharShift(s[i]);
    if (i < (len-1)) {
      delay(d);
    }
  }
}

void Display::setLED(uint8_t led, uint8_t val) {
  if (val == HIGH) {
    ledMask |= led;
  } else {
    ledMask &= ~led;
  }
  setDigitRaw(0, ledMask << 8);
  update();
}

uint8_t Display::readFlag() {
  Wire.beginTransmission(I2CADDR);
  Wire.write((uint8_t)0x60);
  Wire.endTransmission();
  Wire.requestFrom(I2CADDR, 1);
  uint8_t flag = Wire.read();
  return flag;
}

void Display::fadeIn(int d) {
  uint8_t max = config.get(BRIGHTNESS);
  for(int i=0;i<=max;i++) {
    setBrightness(i);
    delay(d);
  }
}

void Display::fadeOut(int d) {
  uint8_t max = config.get(BRIGHTNESS);
  for(int i=max;i>=0;i--) {
    setBrightness(i);
    delay(d);
  }
}


