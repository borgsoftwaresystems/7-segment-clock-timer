#ifndef _7_SEGMENT_CLOCK_TIMER_H_
#define _7_SEGMENT_CLOCK_TIMER_H_

extern void updateDisplay();
extern bool isPressedA();
extern bool isPressedB();
extern bool isPressedX();
extern bool isPressedY();
extern void setBacklight(int brightness);
extern void setLEDOn();
extern void setLEDOff();

extern bool showingMenu;
extern int globalBrightness;
extern bool forDisplay;

void showMenu();
void setButtonInterrupts(bool enabled);
void turnOnAlarmLED();

void setClockTime(int& hour, int &min);
void startRTC(int hour, int min);
void showTheClockRunning();

#endif