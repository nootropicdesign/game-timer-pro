
#include <Wire.h>

#include "Hardware.h"
#include "Config.h"
#include "Display.h"
#include "IR.h"
#include "Input.h"
#include "Keypad.h"
#include "Sound.h"
#include "LowPower.h"

byte leds[] = {LED1, LED2, LED3, LED4};
int volume = 10;

Config config = Config();
Display display = Display();
Keypad keypad = Keypad();
IR ir = IR();
boolean countdownRunning = false;
boolean doubleSpeed = false;
volatile int countdownSeconds;
volatile long currentTime = 0;
long alarmTime;
volatile boolean halfSecond; // for accurate timekeeping if timer running double speed
volatile boolean alarmMatch = false;
boolean snoozeActivated = false;
volatile boolean updateDisplayFlag = false;
byte holdDelay = 150;
int detWire;
int defuseWire;
int speedupWire;
int pauseWire;

void setup() {
  Serial.begin(115200);
  display.begin();
  ir.begin();

  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(DET_TRIGGER_PIN, OUTPUT);
  pinMode(DEFUSE_TRIGGER_PIN, OUTPUT);
  pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DET_PIN, INPUT_PULLUP);
  pinMode(WIRE1_PIN, INPUT_PULLUP);
  pinMode(WIRE2_PIN, INPUT_PULLUP);
  pinMode(WIRE3_PIN, INPUT_PULLUP);
  pinMode(WIRE4_PIN, INPUT_PULLUP);

  digitalWrite(SPEAKER_PIN, LOW);
  digitalWrite(DET_TRIGGER_PIN, LOW);
  digitalWrite(DEFUSE_TRIGGER_PIN, LOW);

  if (!((buttonPressed(BUTTON_LEFT)) && (buttonPressed(BUTTON_RIGHT)))) {
    config.load();
    if (!config.valid()) {
      config.reset();
      beep(2000, 20, 10);
      delay(50);
      beep(2000, 20, 10);
      delay(50);
      beep(2000, 20, 10);
    }
  } else {
    // Factory reset
    config.reset();
    beep(2000, 20, 10);
    delay(50);
    beep(2000, 20, 10);
    while ((buttonPressed(BUTTON_LEFT)) || (buttonPressed(BUTTON_RIGHT)));
  }

  if (buttonPressed(BUTTON_SELECT) || (!config.getEEPROMValid())) {
    test();
    config.reset();
  }

  display.setBrightness(config.get(BRIGHTNESS));
  if (config.get(SOUND) == SOUND_LOW) {
    pinMode(SPEAKER_PIN, INPUT);
    digitalWrite(SPEAKER_PIN, LOW);
  }
  countdownSeconds = config.get(COUNTDOWN_DURATION);
  alarmTime = config.get(ALARM_TIME) * 60L; // convert minutes to seconds

  // init timer1
  // set prescaler to 1024
  TIMSK1 &= ~(1 << TOIE1);
  TCCR1A = 0;
  TCCR1B = (1 << CS12) | (1 << CS10);
  TIMSK1 |= (1 << TOIE1);
  // With prescalar of 1024, TCNT1 increments 15,625 times per second
  // 65535 - 15625 = 49910
  TCNT1 = TIMER1_SECOND_START;

  display.setDisplayColon(true);
  display.printCountdown(countdownSeconds);
}

void testButton(byte b) {
  if (buttonPressedNew(b)) {
    for(byte i=0;i<4;i++) {
      display.setLED(leds[i], HIGH);
    }
    while(buttonPressed(b));
    for(byte i=0;i<4;i++) {
      display.setLED(leds[i], LOW);
    }
  }
}

void test() {
  display.clear();
  beep(2000, 20, 10);
  for(byte i=0;i<3;i++) {
    for(byte j=0;j<4;j++) {
      display.setLED(leds[j], HIGH);
      delay(75);
      display.setLED(leds[j], LOW);
    }
  }

  int i=0;
  while (i++ <= 9999) {
    display.printNum(i, true);
    if (buttonPressedNew(BUTTON_DET)) {
      display.clear();
      while (buttonPressed(BUTTON_DET));
      return;
    }
    testButton(BUTTON_LEFT);
    testButton(BUTTON_SELECT);
    testButton(BUTTON_RIGHT);
  }

}

void updateDisplay() {
    if (config.get(DISP) == DISP_COUNTDOWN) {
      display.printCountdown(countdownSeconds);
    }
    if (config.get(DISP) == DISP_CLOCK) {
      display.printTime(currentTime);
    }
}

void loop() {
  if (config.get(DISP) == DISP_COUNTDOWN) {
    getTimerInput();
  }
  if (config.get(DISP) == DISP_CLOCK) {
    getClockInput();
  }
  if (updateDisplayFlag) {
    updateDisplayFlag = false;
    updateDisplay();
  }
  if (alarmMatch) {
    getAlarmInput();
  }
  delay(10);
}

void getTimerInput() {
  unsigned long irCommand = ir.getIRCommand();

  if ((buttonPressedNew(BUTTON_LEFT)) || (buttonHeld(BUTTON_LEFT, 150)) || (irCommand == IR_LEFT) || (irCommand == IR_LEFT2)) {
    if ((buttonPressed(BUTTON_LEFT)) && (millis() - getLastPress(BUTTON_LEFT) > 2000)) {
      while (buttonPressed(BUTTON_LEFT)) {
        if (countdownSeconds > 1) countdownSeconds--;
        display.printCountdown(countdownSeconds);
        delay(5);
      }
    } else {
      if (countdownSeconds > 1) countdownSeconds--;
      updateDisplayFlag = true;
    }
  }
  if (buttonPressed(BUTTON_SELECT)) {
    display.setDisplayColon(false);
    configure();
    updateDisplayFlag = true;
  }
  if ((buttonPressedNew(BUTTON_RIGHT)) || (buttonHeld(BUTTON_RIGHT, 150)) || (irCommand == IR_RIGHT) || (irCommand == IR_RIGHT2)) {
    if ((buttonPressed(BUTTON_RIGHT)) && (millis() - getLastPress(BUTTON_RIGHT) > 2000)) {
      while (buttonPressed(BUTTON_RIGHT)) {
        if (countdownSeconds < 5999) countdownSeconds++;
        display.printCountdown(countdownSeconds);
        delay(5);
      }
    } else {
      if (countdownSeconds < 5999) countdownSeconds++;
      updateDisplayFlag = true;
    }
  }
  checkDetButton(irCommand);
}

void getClockInput() {
  unsigned long irCommand = ir.getIRCommand();

  if (buttonPressed(BUTTON_SELECT)) {
    display.setDisplayColon(false);
    display.setLED(LED2, LOW);
    configure();
    updateDisplayFlag = true;
  }

  if (buttonPressedNew(BUTTON_LEFT) || buttonHeld(BUTTON_LEFT, holdDelay)) {
    if (millis() - getLastPress(BUTTON_LEFT) > 2000) {
      // button held for more than 2 seconds
      holdDelay = 10;
    } else {
      holdDelay = 150;
    }
    currentTime = (currentTime - 60) - (currentTime % 60); // round down to minute
    if (currentTime < 0) {
      currentTime = SECONDS_PER_DAY-60;
    }
    if ((currentTime >= SECONDS_AT_NOON) && (config.get(CLOCK24HR) == CLOCK24HR_OFF)) {
      display.setLED(LED2, HIGH);
    } else {
      display.setLED(LED2, LOW);
    }
    if (currentTime == alarmTime) {
      alarmMatch = true;
    }
    updateDisplayFlag = true;
  }
  if (buttonPressedNew(BUTTON_RIGHT) || buttonHeld(BUTTON_RIGHT, holdDelay)) {
    if (millis() - getLastPress(BUTTON_RIGHT) > 2000) {
      // button held for more than 2 seconds
      holdDelay = 10;
    } else {
      holdDelay = 150;
    }
    currentTime = (currentTime + 60) - (currentTime % 60); // round down to minute
    if (currentTime >= SECONDS_PER_DAY) {
      currentTime = 0;
    }
    if ((currentTime >= SECONDS_AT_NOON) && (config.get(CLOCK24HR) == CLOCK24HR_OFF)) {
      display.setLED(LED2, HIGH);
    } else {
      display.setLED(LED2, LOW);
    }
    if (currentTime == alarmTime) {
      alarmMatch = true;
    }
    updateDisplayFlag = true;
  }
  if ((!buttonPressed(BUTTON_LEFT)) && (!buttonPressed(BUTTON_RIGHT))) {
    display.setLED(LED2, LOW);
  }

  checkDetButton(irCommand);
}

void checkDetButton(unsigned long irCommand) {
  if ((buttonPressedNew(BUTTON_DET)) || (irCommand == IR_SELECT) || (irCommand == IR_SELECT2)) {
    for(byte i=0;i<4;i++) {
      display.setLED(leds[i], HIGH);
    }
    delay(30);
    for(byte i=0;i<4;i++) {
      display.setLED(leds[i], LOW);
    }
    if (config.get(LOW_POWER) == LOW_POWER_ON) {
      // disable display
      display.setOff();
      while (true) {
        // power down for 500ms then check if button is released
        LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
        if (!buttonPressed(BUTTON_DET)) {
          display.setOn();
          display.printCountdown(countdownSeconds);
          break;
        }
      }
    } else {
      display.printCountdown(countdownSeconds);
      while (buttonPressed(BUTTON_DET));
    }
    if (countdownSeconds != config.get(COUNTDOWN_DURATION)) {
      config.set(COUNTDOWN_DURATION, countdownSeconds);
      config.save();
    }
    countdown();
  }
}

void getAlarmInput() {
  alarmMatch = false;
  int alarmMode = config.get(ALARM_MODE);
  if (alarmMode != ALARM_MODE_OFF) {
    if (alarmMode == ALARM_MODE_BEEP) {
      snoozeActivated = ringAlarm();
      if (snoozeActivated) {
        alarmTime = (alarmTime + (9 * 60L)) % SECONDS_PER_DAY;
      } else {
        alarmTime = config.get(ALARM_TIME) * 60L;
      }
    }
    if (alarmMode == ALARM_MODE_DEFUSE) {
      for (byte i=0;i<3;i++) {
        beep(2450, 350);
        delay(150);
      }
      countdown();
    }
  }
}

boolean wireConflict(byte key, int value) {
  switch (key) {
    case DET_WIRE:
    return ((defuseWire == value) || (speedupWire == value) || (pauseWire == value));
    case DEFUSE_WIRE:
    return ((detWire == value) || (speedupWire == value) || (pauseWire == value));
    case SPEEDUP_WIRE:
    return ((detWire == value) || (defuseWire == value) || (pauseWire == value));
    case PAUSE_WIRE:
    return ((detWire == value) || (defuseWire == value) || (speedupWire == value));
  }
}

int getRandomWire(byte key) {
  int wire = random(WIRE1, WIRE4+1);
  while (wireConflict(key, wire)) {
    wire = random(WIRE1, WIRE4+1);
  }
  return wire;
}

boolean wireCut(int wire) {
  switch (wire) {
    case WIRE1:
    return (digitalRead(WIRE1_PIN) == HIGH);
    case WIRE2:
    return (digitalRead(WIRE2_PIN) == HIGH);
    case WIRE3:
    return (digitalRead(WIRE3_PIN) == HIGH);
    case WIRE4:
    return (digitalRead(WIRE4_PIN) == HIGH);
    default:
    return false;
  }
}

void countdown() {
  boolean isDefused = false;
  detWire = config.get(DET_WIRE);
  defuseWire = config.get(DEFUSE_WIRE);
  speedupWire = config.get(SPEEDUP_WIRE);
  pauseWire = config.get(PAUSE_WIRE);
  boolean pauseDone = false;
  int fractionalSecond;
  byte ledCurrentState = HIGH;
  int key;
  boolean enteringCode = false;
  int toggleCount = 0;
  uint8_t currentDigit = 1;
  uint8_t digits[4]; // left to right
  int currentValue = -1;
  int defuseCode = config.get(DEFUSE_CODE);
  unsigned long irCommand;

  // variables for order-based defuse
  byte wirePos = 0;
  byte nextWire;
  boolean wireCutState[4] = {false, false, false, false};
  byte wiresCut = 0;
  byte wires[4] = {WIRE1, WIRE2, WIRE3, WIRE4};
  byte sequence[4] = {0, 0, 0, 0};


  doubleSpeed = false;
  // Assign wires
  if (detWire == DET_WIRE_RAND) {
    detWire = getRandomWire(DET_WIRE);
  }
  if (defuseWire == DEFUSE_WIRE_RAND) {
    defuseWire = getRandomWire(DEFUSE_WIRE);
  }
  if (speedupWire == SPEEDUP_WIRE_RAND) {
    speedupWire = getRandomWire(SPEEDUP_WIRE);
  }
  if (pauseWire == PAUSE_WIRE_RAND) {
    pauseWire = getRandomWire(PAUSE_WIRE);
  }

  if (defuseWire == DEFUSE_WIRE_ORDER) {
      int order = config.get(DEFUSE_ORDER);
      if (order != UNSET) {
        sequence[0] = (order / 1000);
        sequence[1] = ((order % 1000) / 100);
        sequence[2] = ((order % 100) / 10);
        sequence[3] = ((order % 10));
      }
      wirePos = 0;
      nextWire = sequence[0];
  }

  // Keep track of how far we are into the current
  // second so we can correct later.
  fractionalSecond = TCNT1 - TIMER1_SECOND_START;
  // Reset back to the last second boundary so we can start the countdown
  // immediately and so that the first second isn't truncated
  TCNT1 = TIMER1_SECOND_START;


  beep(3800, 30);
  display.setLED(LED1, ledCurrentState);
  keypad.clear();
  countdownRunning = true;
  while ((countdownSeconds > 0) && (!isDefused)) {
    for(int i=0;i<10;i++) {
      if (updateDisplayFlag) {
        updateDisplayFlag = false;
        if (!enteringCode) {
          display.printCountdown(countdownSeconds);
        }
      }

      if ((detWire != DET_WIRE_NONE) && (defuseWire != DEFUSE_WIRE_ORDER)) {
        if (wireCut(detWire)) {
          countdownRunning = false;
          countdownSeconds = 0;
          break;
        }
      }
      if ((defuseWire != DEFUSE_WIRE_CODE) && (defuseWire != DEFUSE_WIRE_ORDER)) {
        if (wireCut(defuseWire)) {
          isDefused = true;
          break;
        }
      }
      if ((pauseWire != PAUSE_WIRE_NONE) && (defuseWire != DEFUSE_WIRE_ORDER)) {
        if ((wireCut(pauseWire)) && (!pauseDone)) {
          pauseDone = true;
          countdownRunning = false;
          beep(4500, 80);
          display.setLED(LED1, LOW);
          delay(2000);
          countdownRunning = true;
        }
      }
      if ((speedupWire != SPEEDUP_WIRE_NONE) && (defuseWire != DEFUSE_WIRE_ORDER)) {
        if (wireCut(speedupWire)) {
          doubleSpeed = true;
        }
      }
      if (defuseWire == DEFUSE_WIRE_CODE) {
        key = keypad.getKey();
        if (key > -1) {
          if ((!enteringCode) && (key <= 9)) {
            display.clear();
            display.setDisplayColon(false);
            display.printString("____");
            enteringCode = true;
            currentDigit = 1;
            currentValue = -1;
          }

          if (key <= 9) {
            display.setDigitNum(currentDigit, key);
            digits[currentDigit-1] = key;
            currentDigit++;
            currentValue = -1;
            toggleCount = 0;
          }

          if ((key == KEY_STAR) || (key == KEY_POUND)) {
            enteringCode = false;
            display.clear();
            display.printCountdown(countdownSeconds);
          }

          if (currentDigit > 4) {
            beep(3800, 30);
            int enteredCode = (digits[0] * 1000) + (digits[1] * 100) + (digits[2] * 10) + digits[3];
            countdownRunning = false;
            delay(500);
            if (enteredCode == defuseCode) {
              isDefused = true;
            } else {
              countdownSeconds = 0; // wrong code
            }
          }
        }

        // if no current value yet, toggle an underscore
        if ((currentValue < 0) && (enteringCode)) {
          toggleCount++;
          if ((toggleCount % 10) == 0) {
            if ((toggleCount % 20) == 0) {
              display.setDigitChar(currentDigit, '_');
            } else {
              display.setDigitChar(currentDigit, ' ');
            }
          }
        }
      } // if defuse code

      if (defuseWire == DEFUSE_WIRE_ORDER) {
        for(byte w=0;w<4;w++) {
          if (countdownSeconds == 0) break;
          if ((!wireCutState[w]) && (wireCut(wires[w]))) {
            wiresCut++;
            wireCutState[w] = true;
            Serial.print("wire ");
            Serial.print(w+1);
            Serial.print(" cut...");
            if ((w+1) != nextWire) {
              // wrong wire cut!
              Serial.print("expected wire ");
              Serial.println(nextWire);
              countdownRunning = false;
              countdownSeconds = 0;
              break;
            } else {
              Serial.println("correct");
              // correct wire!
              if (wiresCut == 4) {
                isDefused = true;
                break;
              }
              wirePos++;
              nextWire = sequence[wirePos];
            }
          }
        } // for all wires
      } // if defuse wire order

      irCommand = ir.getIRCommand();
      if ((irCommand == IR_UP) || (irCommand == IR_UP2)) {
        doubleSpeed = true;
      }
      if ((irCommand == IR_DOWN) || (irCommand == IR_DOWN2)) {
        doubleSpeed = false;
      }

      // pause
      if ((irCommand == IR_A) || (irCommand == IR_A2)) {
        countdownRunning = false;
        beep(4500, 80);
        display.setLED(LED1, LOW);
        delay(20);
        while (true) {
          delay(20);
          irCommand = ir.getIRCommand();
          if ((irCommand == IR_A) || (irCommand == IR_A2)) break;
        }
        countdownRunning = true;
      }

      // defuse
      if ((irCommand == IR_B) || (irCommand == IR_B2)) {
        isDefused = true;
        break;
      }

      // detonate
      if ((irCommand == IR_C) || (irCommand == IR_C2)) {
        countdownRunning = false;
        countdownSeconds = 0;
        break;
      }

      delay(20);
    }
    // toggle LED
    ledCurrentState = (ledCurrentState == HIGH ? LOW : HIGH);
    display.setLED(LED1, ledCurrentState);

  }
  display.setDisplayColon(true);
  display.printCountdown(countdownSeconds);
  countdownRunning = false;
  doubleSpeed = false;
  if (isDefused) {
    defused();
  } else {
    detonate();
  }
  updateDisplay();
  // Now to keep the time accurate, add back in the fractional
  // second that we took off when we started the countdown sequence.
  // Wait until we can add it back to TCNT1 without overflowing.
  while (TCNT1 >= (65535 - fractionalSecond));
  TCNT1 += fractionalSecond;
  updateDisplayFlag = true;
}


void detonate() {
  unsigned long irCommand;
  unsigned long triggerStart;
  unsigned long triggerStop;
  int n = 50;
  for (int i = 0; i < 8; i++) {
    beep(5000, 50, 10);
    delay(25);
  }

  triggerStart = millis();
  triggerStop = triggerStart + (config.get(DET_TRIGGER_SEC) * 1000UL);
  if (triggerStop > triggerStart) {
    digitalWrite(DET_TRIGGER_PIN, HIGH);
  }
  display.setDisplayColon(false);
  for(int i=0;i<n;i++) {
    if (millis() >= triggerStop) {
      digitalWrite(DET_TRIGGER_PIN, LOW);
    }
    display.setDigitRaw(1, random(255));
    display.setDigitRaw(2, random(255));
    display.setDigitRaw(3, random(255));
    display.setDigitRaw(4, random(255));
    display.setDigitRaw(0, random(255) << 8);
    display.update();
    for (int j = 0; j < 5; j++) {
      if (millis() >= triggerStop) {
        digitalWrite(DET_TRIGGER_PIN, LOW);
      }
      beep(random(100, 300), 10, 10);
    }
  }
  display.clear();
  for(byte i=0;i<4;i++) {
    display.setLED(leds[i], LOW);
  }
  display.setDisplayColon(true);
  display.printCountdown(countdownSeconds);

  delay(250);
  display.setLED(LED2, HIGH);
  // reset countdown
  while (true) {
    if (millis() >= triggerStop) {
      digitalWrite(DET_TRIGGER_PIN, LOW);
    }
    // wait for button press or IR select command
    irCommand = ir.getIRCommand();
    if ((irCommand == IR_SELECT) || (irCommand == IR_SELECT2)) break;
    if (buttonPressed(BUTTON_DET)) break;
    delay(20);
  }
  delay(20);
  while (true) {
    if (millis() >= triggerStop) {
      digitalWrite(DET_TRIGGER_PIN, LOW);
    }
    // wait for button release
    if ((irCommand == IR_SELECT) || (irCommand == IR_SELECT2)) break; // remote was used
    if (!buttonPressed(BUTTON_DET)) break;
    delay(20);
  }
  display.setLED(LED2, LOW);
  digitalWrite(DET_TRIGGER_PIN, LOW);
  countdownSeconds = config.get(COUNTDOWN_DURATION);
}

void defused() {
  unsigned long irCommand;
  unsigned long triggerStart;
  unsigned long triggerStop;

  for(byte i=0;i<4;i++) {
    display.setLED(leds[i], HIGH);
  }
  beep(4500, 80);
  triggerStart = millis();
  triggerStop = triggerStart + (config.get(DEFUSE_TRIGGER_SEC) * 1000UL);
  if (triggerStop > triggerStart) {
    digitalWrite(DEFUSE_TRIGGER_PIN, HIGH);
  }
  delay(50);
  for(byte i=0;i<4;i++) {
    display.setLED(leds[i], LOW);
  }
  int successCode = config.get(SUCCESS_CODE);
  if (successCode != UNSET) {
    for(byte i=0;i<50;i++) {
      if (millis() >= triggerStop) {
        digitalWrite(DEFUSE_TRIGGER_PIN, LOW);
      }
      delay(20);
    }
    display.setDisplayColon(false);
    display.printNum(successCode, false);
    delay(250);
    display.clear();
    delay(50);
    display.printNum(successCode, false);
    delay(250);
    display.clear();
    delay(50);
    display.printNum(successCode, false);
  }

  delay(250);
  display.setLED(LED2, HIGH);
  while (true) {
    if (millis() >= triggerStop) {
      digitalWrite(DEFUSE_TRIGGER_PIN, LOW);
    }
    // wait for button press or IR select command
    irCommand = ir.getIRCommand();
    if ((irCommand == IR_SELECT) || (irCommand == IR_SELECT2)) break;
    if (buttonPressed(BUTTON_DET)) break;
    delay(20);
  }
  delay(20);
  while (true) {
    if (millis() >= triggerStop) {
      digitalWrite(DEFUSE_TRIGGER_PIN, LOW);
    }
    // wait for button release
    if ((irCommand == IR_SELECT) || (irCommand == IR_SELECT2)) break; // from last check
    if (!buttonPressed(BUTTON_DET)) break;
    delay(20);
  }
  display.setLED(LED2, LOW);
  digitalWrite(DEFUSE_TRIGGER_PIN, LOW);
  countdownSeconds = config.get(COUNTDOWN_DURATION);
}

void configure() {
  int currentAlarmTimeSetting = config.get(ALARM_TIME);
  int currentAlarmMode = config.get(ALARM_MODE);
  config.configure(); // get configuration from user
  // Update any variables based on configuration.
  if ((currentAlarmTimeSetting != config.get(ALARM_TIME)) || (currentAlarmMode != config.get(ALARM_MODE))) {
    Serial.println("alarm settings changed, deactivating snooze");
    snoozeActivated = false;
  }
  if (!snoozeActivated) {
    alarmTime = config.get(ALARM_TIME) * 60L;
  }
}


// Timer 1 interrupt.  This executes every second.
ISR(TIMER1_OVF_vect) {
  if (!doubleSpeed) {
    TCNT1 = TIMER1_SECOND_START;
    halfSecond = false;
  } else {
    TCNT1 = TIMER1_HALFSECOND_START;
    halfSecond = !halfSecond; // toggle this for accurate timekeeping during double speed countdown
  }
  if (countdownRunning) {
    if (config.get(TICK) == TICK_ON) {
      tick(10);
    }
    countdownSeconds--;
  }
  if (!halfSecond) {
    currentTime = (currentTime + 1) % SECONDS_PER_DAY;
  }
  if ((currentTime == alarmTime) && (!countdownRunning)) {
    alarmMatch = true;
  }
  updateDisplayFlag = true;
}
