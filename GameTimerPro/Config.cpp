#include <EEPROM.h>

#include "Hardware.h"
#include "Config.h"
#include "Input.h"
#include "Display.h"
#include "Sound.h"

Config::Config(void) {
  nValues[DISP] = 2;
  keyString[DISP] = "dISP";
  valueString[DISP] = new char* [2]{"ctdn", "cloc"};

  nValues[CLOCK24HR] = 2;
  keyString[CLOCK24HR] = "24hr";
  valueString[CLOCK24HR] = new char* [2]{"On", "OFF"};

  nValues[ALARM_TIME] = 0;
  keyString[ALARM_TIME] = "AL t";

  nValues[ALARM_MODE] = 3;
  keyString[ALARM_MODE] = "AL";
  valueString[ALARM_MODE] = new char* [3]{"OFF", "bEEP", "dEF"};

  nValues[TICK] = 2;
  keyString[TICK] = "tic";
  valueString[TICK] = new char* [2]{"On", "OFF"};

  nValues[DET_WIRE] = 6;
  keyString[DET_WIRE] = "dEt";
  valueString[DET_WIRE] = new char* [6]{"nonE", "rAnd", "1   ", " 2  ", "  3 ", "   4"};

  nValues[DEFUSE_WIRE] = 7;
  keyString[DEFUSE_WIRE] = "dEF";
  valueString[DEFUSE_WIRE] = new char* [7]{"codE", "ordr", "1   ", " 2  ", "  3 ", "   4", "rAnd"};

  nValues[SPEEDUP_WIRE] = 6;
  keyString[SPEEDUP_WIRE] = "2 SP";
  valueString[SPEEDUP_WIRE] = new char* [6]{"nonE", "rAnd", "1   ", " 2  ", "  3 ", "   4"};

  nValues[PAUSE_WIRE] = 6;
  keyString[PAUSE_WIRE] = "PAUS";
  valueString[PAUSE_WIRE] = new char* [6]{"nonE", "rAnd", "1   ", " 2  ", "  3 ", "   4"};

  nValues[DET_TRIGGER_SEC] = 0;
  keyString[DET_TRIGGER_SEC] = "tdEt";

  nValues[DEFUSE_TRIGGER_SEC] = 0;
  keyString[DEFUSE_TRIGGER_SEC] = "tdEF";

  nValues[SUCCESS_CODE] = 0;
  keyString[SUCCESS_CODE] = "Succ";

  nValues[BRIGHTNESS] = 16;
  keyString[BRIGHTNESS] = "brit";
  valueString[BRIGHTNESS] = new char* [16]{"   1", "   2", "   3", "   4", "   5", "   6", "   7", "   8", "   9", "  10", "  11", "  12", "  13", "  14", "  15", "  16"};

  nValues[SOUND] = 3;
  keyString[SOUND] = "Snd ";
  valueString[SOUND] = new char* [3]{"Hi", "Lo", "OFF"};

  nValues[LOW_POWER] = 2;
  keyString[LOW_POWER] = "Lo P";
  valueString[LOW_POWER] = new char* [2]{"OFF", "On"};

}

void Config::setDefaults() {
  set(DISP, DISP_COUNTDOWN);
  set(ALARM_TIME, 0);
  set(CLOCK24HR, CLOCK24HR_OFF);
  set(ALARM_MODE, ALARM_MODE_OFF);
  set(TICK, TICK_ON);
  set(DET_WIRE, DET_WIRE_RAND);
  set(DEFUSE_WIRE, DEFUSE_WIRE_RAND);
  set(SPEEDUP_WIRE, SPEEDUP_WIRE_NONE);
  set(PAUSE_WIRE, PAUSE_WIRE_NONE);
  set(DET_TRIGGER_SEC, 5);
  set(DEFUSE_TRIGGER_SEC, 5);
  set(SUCCESS_CODE, UNSET);
  set(BRIGHTNESS, MAX_BRIGHTNESS);
  set(SOUND, SOUND_HIGH);
  set(LOW_POWER, LOW_POWER_OFF);
  set(COUNTDOWN_DURATION, 10);
  set(DEFUSE_CODE, UNSET);
  set(DEFUSE_ORDER, UNSET);
}

void Config::configure() {
  uint8_t key = 0;
  while (true) {
    if (buttonPressedNew(BUTTON_DET)) {
      break;
    }
    if (buttonPressedNew(BUTTON_LEFT)) {
      if (key == 0) {
        key = N_MENU-1;
      } else {
        key--;
      }
    }
    if (buttonPressedNew(BUTTON_RIGHT)) {
      key = (key + 1) % N_MENU;
    }

    if (buttonPressedNew(BUTTON_SELECT)) {
      // Setting a value
      int value = get(key);
      display.setLED(LED1, HIGH);

      // Special case for code input
      if (key == SUCCESS_CODE) {
        if (value != UNSET) {
          display.printNum(value, true); // show current value
          delay(1500);
        }
        int code = inputCode();
        if ((code != UNSET) || ((buttonPressed(BUTTON_DET)) && (buttonPressed(BUTTON_SELECT)))) {
          // If a value was returned or if SELECT was held when exiting with DET to erase value (e.g. SUCCESS code)
          values[key] = code;
          confirmSetting();
        }
        display.setLED(LED1, LOW);
        continue;
      }

      // Special case for time input
      if (key == ALARM_TIME) {
        long t = inputTime(values[key] * 60L);
        display.setDisplayColon(false);
        if (t != -1) {
          values[key] = t / 60;
          confirmSetting();
        }
        display.setLED(LED1, LOW);
        continue;
      }

      while (true) {
        if (buttonPressedNew(BUTTON_DET)) {
          break; // escape
        }
        if (buttonPressedNew(BUTTON_SELECT)) {

          if ((key == DEFUSE_WIRE) && (value == DEFUSE_WIRE_CODE)) {
            int currentCode = get(DEFUSE_CODE);
            if (currentCode != UNSET) {
              display.printNum(currentCode, true); // show current code
              delay(1500);
            }
            int code = inputCode();
            if (code != UNSET) {
              set(DEFUSE_CODE, code);
            }
          }

          if ((key == DEFUSE_WIRE) && (value == DEFUSE_WIRE_ORDER)) {
            int currentOrder = get(DEFUSE_ORDER);
            if (currentOrder != UNSET) {
              display.printNum(currentOrder, true); // show current wire order
              delay(1500);
            }
            int order = inputWireOrder();
            if (order != UNSET) {
              set(DEFUSE_ORDER, order);
            }
          }

          if (key == SOUND) {
            setSoundLevel(value);
          }

          values[key] = value; // new value set
          confirmSetting();
          break;
        }
        // decrease value
        if ((buttonPressedNew(BUTTON_LEFT)) || ((buttonHeld(BUTTON_LEFT, 150)) && (nValues[key] == 0))) {
          if (nValues[key] > 0) { // if this is an enumerated value
            if (value == 0) {
              value = nValues[key]-1;
            } else {
              value--;
            }
            if ((key >= DET_WIRE) && (key <= PAUSE_WIRE)) {
              while ((value >= WIRE1) && (value <= WIRE4) && (wireConflict(key, value))) {
                // while setting a wire number and there is a conflict, skip this value
                value--;
              }
            }
          } else {
            if (value > 0) value--;
          }
          if (key == SOUND) {
            demoSound(value);
          }
        }
        // increase value
        if ((buttonPressedNew(BUTTON_RIGHT)) || ((buttonHeld(BUTTON_RIGHT, 150)) && (nValues[key] == 0))) {
          if (nValues[key] > 0) { // if this is an enumerated value
            value = (value + 1) % nValues[key];
            if ((key >= DET_WIRE) && (key <= PAUSE_WIRE)) {
              while ((value >= WIRE1) && (value <= WIRE4) && (wireConflict(key, value))) {
                // while setting a wire number and there is a conflict, skip this value
                value = (value + 1) % nValues[key];
              }
            }
          } else {
            if (value < 9999) value++;
          }
          if (key == SOUND) {
            demoSound(value);
          }
        }

        if (nValues[key] > 0) {
          display.printString(getValueString(key, value));
        } else {
          display.printNum(value, false);
        }
        if (key == BRIGHTNESS) {
          display.setBrightness(value);
        }
        delay(20);
      } // while (true)
      display.setLED(LED1, LOW);
    }

    display.printString(getKeyString(key));
  	delay(20);
  }
  save();
  display.clear();
}

void Config::demoSound(int value) {
  if (value != SOUND_OFF) {
    int currentValue = values[SOUND];
    values[SOUND] = value; // temporarily
    setSoundLevel(value); // temporarily
    beep(2000, 20, 10);
    values[SOUND] = currentValue;
    setSoundLevel(currentValue); // reset hardware to current config
  }
}

boolean Config::wireConflict(uint8_t key, int value) {
  switch (key) {
    case DET_WIRE:
    return ((get(DEFUSE_WIRE) == value) || (get(SPEEDUP_WIRE) == value) || (get(PAUSE_WIRE) == value));
    case DEFUSE_WIRE:
    return ((get(DET_WIRE) == value) || (get(SPEEDUP_WIRE) == value) || (get(PAUSE_WIRE) == value));
    case SPEEDUP_WIRE:
    return ((get(DET_WIRE) == value) || (get(DEFUSE_WIRE) == value) || (get(PAUSE_WIRE) == value));
    case PAUSE_WIRE:
    return ((get(DET_WIRE) == value) || (get(DEFUSE_WIRE) == value) || (get(SPEEDUP_WIRE) == value));
  }
}

void Config::confirmSetting() {
  for(byte i=0;i<3;i++) {
    display.setLED(LED1, LOW);
    delay(50);
    display.setLED(LED1, HIGH);
    delay(50);
  }
}

char * Config::getKeyString(uint8_t key) {
	return keyString[key];
}

char * Config::getValueString(uint8_t key, int value) {
	return valueString[key][value];
}

int Config::get(uint8_t key) {
  return values[key];
}

void Config::set(uint8_t key, int value) {
	values[key] = value;
}

boolean Config::getEEPROMValid() {
  // determine if the EEPROM has ever been written by this firmware
  // so we can determine if the values can be trusted
  unsigned int magic = EEPROM.read(0);
  magic = magic << 8;
  magic |= EEPROM.read(1);
  return (magic == EEPROM_MAGIC_NUMBER);
}

void Config::setEEPROMValid() {
  EEPROM.write(0, EEPROM_MAGIC_NUMBER >> 8);
  EEPROM.write(1, (EEPROM_MAGIC_NUMBER & 0xFF));
}

void Config::setEEPROMInvalid() {
  EEPROM.write(0, 0);
  EEPROM.write(1, 0);
}

void Config::load() {
  int addr;
  int value;
  if (getEEPROMValid()) {
    // Load menu configuration items
    for(uint8_t i=0;i<N_CONFIG;i++) {
      addr = CONFIG_START_ADDR + (i*2);
      value = EEPROM.read(addr);
      value = value << 8;
      value |= EEPROM.read(addr+1);
      values[i] = value;
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(values[i]);
    }
  } else {
    setDefaults();
  }
}

boolean Config::valid() {
  int v;
  // check enumerated values
  for(uint8_t i=0;i<N_MENU;i++) {
    if (nValues[i] > 0) {
      if (values[i] > nValues[i]) {
        Serial.print("valid() - invalid value for [");
        Serial.print(i);
        Serial.print("]: ");
        Serial.println(v);
        return false;
      }
    }
  }
  v = values[ALARM_TIME];
  if ((v < UNSET) || (v >= (SECONDS_PER_DAY / 60))) {
    Serial.print("valid() - invalid value for ALARM_TIME: ");
    Serial.println(v);
    return false;
  }

  v = values[DET_TRIGGER_SEC];
  if ((v < 0) || (v > 9999)) {
    Serial.print("valid() - invalid value for DET_TRIGGER_SEC: ");
    Serial.println(v);
    return false;
  }

  v = values[DEFUSE_TRIGGER_SEC];
  if ((v < 0) || (v > 9999)) {
    Serial.print("valid() - invalid value for DEFUSE_TRIGGER_SEC: ");
    Serial.println(v);
    return false;
  }

  v = values[SUCCESS_CODE];
  if ((v < UNSET) || (v > 9999)) {
    Serial.print("valid() - invalid value for SUCCESS_CODE: ");
    Serial.println(v);
    return false;
  }

  v = values[DEFUSE_CODE];
  if ((v < UNSET) || (v > 9999)) {
    Serial.print("valid() - invalid value for DEFUSE_CODE: ");
    Serial.println(v);
    return false;
  }

  v = values[DEFUSE_ORDER];
  if ((v < UNSET) || (v > 4321)) { // not robust check
    Serial.print("valid() - invalid value for DEFUSE_ORDER: ");
    Serial.println(v);
    return false;
  }

  v = values[COUNTDOWN_DURATION];
  if ((v <= 0) || (v > 5999)) {
    Serial.print("valid() - invalid value for COUNTDOWN_DURATION: ");
    Serial.println(v);
    return false;
  }

  return true;
}

void Config::save() {
  int addr;
  int value;
  setEEPROMValid();
  for(uint8_t i=0;i<N_CONFIG;i++) {
    addr = CONFIG_START_ADDR + (i*2);
    value = values[i];
    EEPROM.write(addr, value >> 8);
    EEPROM.write(addr+1, value & 0xFF);
  }
}

void Config::reset() {
  setDefaults();
  save();
}













