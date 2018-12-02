#ifndef SOUND_H
#define SOUND_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

extern void tick(int vol);
extern void beep(int frequency, int duration);
extern void beep(int frequency, int duration, int vol);
extern boolean ringAlarm();
extern void setSoundLevel(int value);

#endif
