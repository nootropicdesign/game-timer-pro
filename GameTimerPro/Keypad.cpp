#include "Hardware.h"
#include "Keypad.h"


Keypad::Keypad(void) {
}

void Keypad::readKeypad() {
  // Save the last memory
  for(uint8_t i=KEY_0;i<=KEY_POUND;i++) {
    lastKeyMem[i] = keyMem[i];
  }
  uint8_t keypadData[] = {0, 0, 0, 0, 0, 0};

  Wire.beginTransmission(I2CADDR);
  Wire.write((uint8_t)0x40); // start at address 0x40
  Wire.endTransmission();

  uint8_t av = Wire.requestFrom(I2CADDR, 6);
  uint8_t n = 0;
  while ((Wire.available()) && (n < av)) {
    keypadData[n++] = Wire.read();
  }

  // Adafruit keypad

#ifdef ADAFRUIT
  keyMem[KEY_STAR] = keypadData[1] & 0x01;
  keyMem[KEY_0] = keypadData[3] & 0x01;
  keyMem[KEY_POUND] = keypadData[5] & 0x01;

  keyMem[KEY_7] = keypadData[1] & 0x02;
  keyMem[KEY_8] = keypadData[3] & 0x02;
  keyMem[KEY_9] = keypadData[5] & 0x02;

  keyMem[KEY_4] = keypadData[1] & 0x04;
  keyMem[KEY_5] = keypadData[3] & 0x04;
  keyMem[KEY_6] = keypadData[5] & 0x04;

  keyMem[KEY_1] = keypadData[1] & 0x08;
  keyMem[KEY_2] = keypadData[3] & 0x08;
  keyMem[KEY_3] = keypadData[5] & 0x08;
#endif

  // Cheap membrane keypad or Jameco keypad with half twist in cable connection:

#ifdef GENERIC
  keyMem[KEY_STAR] = keypadData[5] & 0x08;
  keyMem[KEY_0] = keypadData[3] & 0x08;
  keyMem[KEY_POUND] = keypadData[1] & 0x08;

  keyMem[KEY_7] = keypadData[5] & 0x04;
  keyMem[KEY_8] = keypadData[3] & 0x04;
  keyMem[KEY_9] = keypadData[1] & 0x04;

  keyMem[KEY_4] = keypadData[5] & 0x02;
  keyMem[KEY_5] = keypadData[3] & 0x02;
  keyMem[KEY_6] = keypadData[1] & 0x02;

  keyMem[KEY_1] = keypadData[5] & 0x01;
  keyMem[KEY_2] = keypadData[3] & 0x01;
  keyMem[KEY_3] = keypadData[1] & 0x01;
#endif
}

int Keypad::getKey() {
  readKeypad();
  for(uint8_t i=KEY_0;i<=KEY_POUND;i++) {
    if ((keyMem[i]) && (!lastKeyMem[i])) {
      return i;
    }
  }
  return -1;
}

void Keypad::clear() {
  readKeypad();
}

