#ifndef INPUT_H
#define INPUT_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

// indexes into buttonPins array
#define BUTTON_LEFT 0
#define BUTTON_RIGHT 1
#define BUTTON_SELECT 2
#define BUTTON_DET 3

extern boolean buttonPressed(byte button);
extern boolean buttonPressedNew(byte button);
extern boolean buttonHeld(byte button, int duration);
extern unsigned long getLastPress(byte button);
extern int inputCode();
extern int inputWireOrder();
extern long inputTime(long t);



#endif
