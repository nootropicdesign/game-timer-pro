#ifndef CONFIG_H
#define CONFIG_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#define N_CONFIG 18
#define N_MENU 15
#define UNSET -1

#define DISP 0
#define CLOCK24HR 1
#define ALARM_TIME 2
#define ALARM_MODE 3
#define TICK 4
#define DET_WIRE 5
#define DEFUSE_WIRE 6
#define SPEEDUP_WIRE 7
#define PAUSE_WIRE 8
#define DET_TRIGGER_SEC 9
#define DEFUSE_TRIGGER_SEC 10
#define SUCCESS_CODE 11
#define BRIGHTNESS 12
#define SOUND 13
#define LOW_POWER 14
// non-menu configuration items
#define COUNTDOWN_DURATION 15
#define DEFUSE_CODE 16
#define DEFUSE_ORDER 17

#define DISP_COUNTDOWN 0
#define DISP_CLOCK 1

#define CLOCK24HR_ON 0
#define CLOCK24HR_OFF 1

#define ALARM_MODE_OFF 0
#define ALARM_MODE_BEEP 1
#define ALARM_MODE_DEFUSE 2

#define TICK_ON 0
#define TICK_OFF 1

#define DET_WIRE_NONE 0
#define DET_WIRE_RAND 1

#define DEFUSE_WIRE_CODE 0
#define DEFUSE_WIRE_ORDER 1
#define DEFUSE_WIRE_RAND 6

#define SPEEDUP_WIRE_NONE 0
#define SPEEDUP_WIRE_RAND 1

#define PAUSE_WIRE_NONE 0
#define PAUSE_WIRE_RAND 1

#define WIRE1 2
#define WIRE2 3
#define WIRE3 4
#define WIRE4 5

#define SOUND_HIGH 0
#define SOUND_LOW 1
#define SOUND_OFF 2

#define LOW_POWER_OFF 0
#define LOW_POWER_ON 1


#define EEPROM_MAGIC_NUMBER 0xbad1
#define CONFIG_START_ADDR 2


class Config {
 public:
  Config(void);
  void setDefaults();
  int get(uint8_t key);
  void set(uint8_t key, int value);
  const char *getKeyString(uint8_t key);
  const char *getValueString(uint8_t key, int value);
  uint8_t nValues[N_MENU];
  void configure(void);
  void load(void);
  boolean valid(void);
  void save(void);
  void reset();
  boolean getEEPROMValid();

 private:
  int values[N_CONFIG];
  const char *keyString[N_MENU];
  const char **valueString[N_MENU];
  boolean wireConflict(uint8_t key, int value);
  void demoSound(int value);
  void confirmSetting();
  void setEEPROMValid();
  void setEEPROMInvalid();

};

extern Config config;

#endif // CONFIG_H
