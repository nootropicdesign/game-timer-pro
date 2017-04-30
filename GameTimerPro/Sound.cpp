#include "Hardware.h"
#include "Sound.h"
#include "Config.h"
#include "Input.h"

void setSoundLevel(int value) {
  if (value == SOUND_LOW) {
    pinMode(SPEAKER_PIN, INPUT);
    digitalWrite(SPEAKER_PIN, LOW);
  }
  if (value == SOUND_HIGH) {
    pinMode(SPEAKER_PIN, OUTPUT);
  }
}

void tick(int vol) {
  boolean sound = (config.get(SOUND) != SOUND_OFF);
  uint8_t volume_div[] = { 255, 200, 150, 125, 100, 87, 50, 33, 22, 2 };
  int frequency = 1000000 / 2000;
  int duty = frequency / volume_div[vol-1];
  int loopCount = (2 * ((float)frequency / 1000.0));
  for (int i = 0; i < loopCount; i++) {
    if (sound) PORTD |= (1 << 2);
    delayMicroseconds(duty);
    if (sound) PORTD &= ~(1 << 2);
    delayMicroseconds(frequency-duty);
  }
}

void beep(int frequency, int duration) {
  beep(frequency, duration, 10);
}

void beep(int frequency, int duration, int vol) {
  boolean sound = (config.get(SOUND) != SOUND_OFF);
  uint8_t volume_div[] = { 255, 200, 150, 125, 100, 87, 50, 33, 22, 2 };
  frequency = 1000000 / frequency;
  int duty = frequency / volume_div[vol-1];
  unsigned long start = millis();
  while ((millis() - start) < duration) {
    if (sound) PORTD |= (1 << 2);
    delayMicroseconds(duty);
    if (sound) PORTD &= ~(1 << 2);
    delayMicroseconds(frequency-duty);
  }
}

// returns true if snooze activated
boolean ringAlarm() {
  uint8_t vol = 10;
  uint8_t volume_div[] = { 255, 200, 150, 125, 100, 87, 50, 33, 22, 2 };
  uint16_t frequency = 1000000 / 2500;
  uint16_t duty = frequency / volume_div[vol-1];
  uint16_t duration = 350;  
  uint16_t pauseDuration = 150;

  while (true) {
    unsigned long start = millis();
    while ((millis() - start) < duration) {
      PORTD |= (1 << 2);
      if (digitalRead(BUTTON_DET_PIN) == LOW) {
        PORTD &= ~(1 << 2);
        while(buttonPressed(BUTTON_DET)); // wait for release
        return true;
      }
      delayMicroseconds(duty);
      PORTD &= ~(1 << 2);
      if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
        while(buttonPressed(BUTTON_SELECT)); // wait for release.
        return false;
      }
      delayMicroseconds(frequency-duty);
    }

    start = millis();
    while ((millis() - start) < pauseDuration) {
      if (digitalRead(BUTTON_DET_PIN) == LOW) {
        while(buttonPressed(BUTTON_DET)); // wait for release
        return true;
      }
      delayMicroseconds(duty);
      if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
        while(buttonPressed(BUTTON_SELECT)); // wait for release.
        return false;
      }
      delayMicroseconds(frequency-duty);
    }
  } // while (true)
}
