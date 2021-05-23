#include "7_segment_clock_timer.hpp"

#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "ctime"

extern "C" {
  #include "hardware/rtc.h"
  #include "hardware/pwm.h"

  #include "pico/util/datetime.h"
  #include "PicoTM1637.h"
}

#define CLK_PIN 7
#define DIO_PIN 6
#define BTN_A 4
#define BTN_B 1
#define BTN_X 2
#define BTN_Y 3
#define AUD_PIN 0

const int BUTTON_DEBOUNCE_PAUSE = 300;

int globalBrightness = 0;
bool showingMenu = false;
int showTimerAlarmCompleteCountDown = 0;
bool showTimer = false;
bool timerRunning = false;

alarm_id_t alarmId = -1;
uint64_t timerLastChecked = 0;
uint64_t timerLeft = 0;
uint64_t timerDuration = 0;

void resetRTC(int hour, int min) {
  datetime_t t = {
    .year  = 2020,
    .month = 2,
    .day   = 27,
    .dotw  = 6, // 0 is Sunday
    .hour  = (int8_t)hour,
    .min   = (int8_t)min,
    .sec   = 0
  };

  // Start the RTC
  rtc_set_datetime(&t);  
}

void setAudio() {
  pwm_config tone_pwm_cfg = pwm_get_default_config();

  pwm_config_set_clkdiv(&tone_pwm_cfg, 255);
  pwm_init(pwm_gpio_to_slice_num(AUD_PIN), &tone_pwm_cfg, true);
  gpio_set_function(AUD_PIN, GPIO_FUNC_PWM);  
}

void set_tone(uint16_t frequency, float duty) {
  // output a square wave, so 50% duty cycle
  if(AUD_PIN != -1) {
    uint16_t pwm_wrap = 490196 / frequency;
    pwm_set_wrap(AUD_PIN, pwm_wrap);
    pwm_set_gpio_level(AUD_PIN, pwm_wrap * duty);
  }
}

void drawMenu(int menuItem, int pos) {
  char* menu = (char *)"MENU";
  char* setTimer = (char *)"SttI";
  char* setBrightness = (char *)"Stbr";

  char* setClock = (char *)"STCL";

  if (menuItem == 0) {
    switch (pos) {
        case 0:
          TM1637_display_word(menu, true);
          break;
        case 1:
          TM1637_display_word(setTimer, true);
          break;
        case 2:
          TM1637_display_word(setBrightness, true);
          break;   
        case 3:
          TM1637_display_word(setClock, true);
          break;           
      }
  }
}

int64_t timerAlarmCallback(alarm_id_t id, void *user_data) {
  char* alarm = (char *)"ALrM";
  alarmId = -1;
  timerRunning = false;
  timerLeft = 0;
  timerLastChecked = 0;

  //Show the alarm text
  TM1637_display_word(alarm, true);

  //Audio
  set_tone(500, 0.5);

  showTimerAlarmCompleteCountDown = 5;
  //Need this to stay on screen for 5 seconds when in clock or timer mode but cannot sleep here!
  //Note. If was in the menu mode then this will stay until the user presses any button.
  //Cannot sleep here! It locks up.

  return 0;
}

bool isPressed(int button) {
  return !gpio_get(button);
}

void cancelTimerAlarm() {
  if (alarmId != -1) {
    cancel_alarm(alarmId);
    alarmId = -1;    
  }
}

void setTimerAlarm() {
  //Cancel any previous alarm
  cancelTimerAlarm();

  if (timerLeft != 0 && timerRunning) {
    uint64_t timeNow = time_us_64();    

    timerLeft = timerLeft-(timeNow-timerLastChecked);    
    timerLastChecked = timeNow;
    alarmId = add_alarm_in_us(timerLeft, timerAlarmCallback, NULL, false);
  }
}

void setBacklight(int brightness) {
  TM1637_set_brightness(brightness);
}

void drawTimeItem(int hour, int min, int pos) {
    TM1637_display_both(hour, min, true);
}

void drawBrightnessItem(int brightness) {  
  TM1637_display_right(brightness, true);
}

bool showTimerItem() {
  int pos = 1;
  int hour = 0;
  int min = 0;

  drawTimeItem(hour, min, pos);  
  sleep_ms(BUTTON_DEBOUNCE_PAUSE);

  int count = 0;

  while (true) {
    sleep_ms(10);

    //Flash the correct timer item
    if (count == 100) {
      count = 0;
      drawTimeItem(hour, min, pos);       
    }

    if (count == 50) {
      TM1637_display_dash_and_number(hour, min, pos, true);                 
    }

    count++;

    if(isPressed(BTN_A)) {
      pos--;
      if (pos < 1) {
        sleep_ms(200);    
        return false;
      }  

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_B)) {
      pos++;
      if (pos > 2) {
        //Set timer
        if (hour == 0 && min == 0) {
          //no timer set
          timerLeft = 0;
          timerLastChecked = 0;
          timerRunning = false;
          timerDuration = 0;
          cancelTimerAlarm();
          
          return true;
        }

        timerLeft = (min*60*1000000ULL) + (hour*3600*1000000ULL);
        timerLastChecked = time_us_64(); 
        timerRunning = true;
        timerDuration = (hour*3600) + (min*60);
        setTimerAlarm();
        sleep_ms(200);
        return true;
      }

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_X)) {
      if (pos == 1) {
        hour++;
        if (hour > 23) {
          hour = 0;
        }
      }
      else {
        min++;
        if (min > 59) {
          min = 0;
        }
      }

      drawTimeItem( hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_Y)) {
      if (pos == 1) {
        hour--;
        if (hour < 0) {
          hour = 23;
        }
      }
      else {
        min--;
        if (min < 0) {
          min = 59;
        }
      }

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }
  }  

  return true;
}

bool showBrightnessItem() {
  int brightness = globalBrightness;
  TM1637_clear();
  drawBrightnessItem(brightness);
  sleep_ms(BUTTON_DEBOUNCE_PAUSE);
  
  while (true) {
    if(isPressed(BTN_A)) {
      setBacklight(globalBrightness);
      drawBrightnessItem(brightness);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
      return false;
    }

    if(isPressed(BTN_B)) {
      //Set the brightness
      globalBrightness = brightness;
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
      return true;
    }

    if(isPressed(BTN_X)) {
      brightness += 1;
      if (brightness > 7) {
        brightness = 0;
      }      
      setBacklight(brightness);
      drawBrightnessItem(brightness);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_Y)) {
      brightness -= 1;
      if (brightness < 0) {
        brightness = 7;
      }      
      setBacklight(brightness);
      drawBrightnessItem(brightness);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }
  }   
}

bool showTimeItem() {
  datetime_t t;

  rtc_get_datetime(&t);

  int pos = 1;
  int hour = t.hour;
  int min = t.min;

  drawTimeItem(hour, min, pos); 
  sleep_ms(BUTTON_DEBOUNCE_PAUSE);

  int count = 0;
  
  while (true) {
    sleep_ms(10);

    //Flash the correct time item
    if (count == 100) {
      count = 0;
      drawTimeItem(hour, min, pos);       
    }

    if (count == 50) {
      TM1637_display_dash_and_number(hour, min, pos, true);                 
    }

    count++;

    if(isPressed(BTN_A)) {
      pos--;
      if (pos < 1) {
        sleep_ms(200);    
        return false;
      }  

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_B)) {
      pos++;
      if (pos > 2) {
        //Set time
        resetRTC(hour, min);

        char* menu = (char *)"SEt*";
        TM1637_display_word(menu, true);

        sleep_ms(1000);
        return true;
      }

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_X)) {
      if (pos == 1) {
        hour++;
        if (hour > 23) {
          hour = 0;
        }
      }
      else {
        min++;
        if (min > 59) {
          min = 0;
        }
      }

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_Y)) {
      if (pos == 1) {
        hour--;
        if (hour < 0) {
          hour = 23;
        }
      }
      else {
        min--;
        if (min < 0) {
          min = 59;
        }
      }

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);      
    }
  }  

  return true;
}

void showMenu() { 
  char buffer[50];
  int menuItem = 0;
  int pos = 0;

  drawMenu(menuItem, pos);
  sleep_ms(BUTTON_DEBOUNCE_PAUSE);

  while (true) {    
    if(isPressed(BTN_A)) {       
      showingMenu = false;
      showTimerAlarmCompleteCountDown = 0;   //Prevent pause of display of alarm when back in timer/clock mode             
      return;
    }

    if(isPressed(BTN_B)) {
      switch (pos) {
        case 0:
          //Still on menu item so do nothing
          break;
        case 1:
          if (showTimerItem()) {            
            showingMenu = false;  
            showTimerAlarmCompleteCountDown = 0;   //Prevent pause of display of alarm when back in timer/clock mode       
            return; 
          }

          drawMenu(menuItem, pos);
          sleep_ms(BUTTON_DEBOUNCE_PAUSE);        
          break;
        case 2:
          if (showBrightnessItem()) {            
            showingMenu = false;       
            showTimerAlarmCompleteCountDown = 0;   //Prevent pause of display of alarm when back in timer/clock mode  
            return; 
          }    

          drawMenu(menuItem, pos);
          sleep_ms(BUTTON_DEBOUNCE_PAUSE);      
          break;
        case 3:
          if (showTimeItem()) {            
            showingMenu = false;       
            showTimerAlarmCompleteCountDown = 0;   //Prevent pause of display of alarm when back in timer/clock mode  
            return; 
          }  

          drawMenu(menuItem, pos);
          sleep_ms(BUTTON_DEBOUNCE_PAUSE);        
          break;
      }
    }

    if(isPressed(BTN_X)) {
      pos--;
      if (pos < 0) {        
        pos = 3;
      }
      drawMenu(menuItem, pos);    
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_Y)) {
      pos++;
      if (pos > 3) {
        pos = 0;
      }
      drawMenu(menuItem, pos); 
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }
  }
}

void drawClockSimple() {
  char datetime_buf[256];
  char *datetime_str = &datetime_buf[0];
  datetime_t t;

  rtc_get_datetime(&t);

  //Show time 
  TM1637_display_both(t.hour, t.min, true);
}

void drawTimerSimple() {
  char datetime_buf[256];
  char *datetime_str = &datetime_buf[0];
  datetime_t t;

  rtc_get_datetime(&t);

  if (timerRunning) {
    //Determine proportion
    uint64_t timeNow = 0;

    //Calculate what is left
    timeNow = time_us_64();

    if (timerLeft <= (timeNow-timerLastChecked)) { //This goes very positive when negative if I just do <=0!
      //Stop
      timerLeft = 0;
      timerRunning = false;
      timerLastChecked = 0;
    }
    else {
      timerLeft = timerLeft-(timeNow-timerLastChecked);
      timerLastChecked = timeNow;
    }     
  }
  
   //Show timer
  uint64_t temp;
  uint8_t numHours = 0;
  uint8_t numMins = 0;
  uint8_t numSecs2 = 0;

  temp = timerLeft / 1000000ULL;
  numHours = temp / (3600);
  temp -= (numHours * (3600));
  numMins = temp / (60);
  temp -= (numMins * (60));
  numSecs2 = temp;
  
  //Show timer
  TM1637_display_both(numHours, numMins, true);
}

void waitUntilAtBeginningOfSecond() {
  //Tight loop to wait until RTC clock has changed the second
  datetime_t t;  
  rtc_get_datetime(&t);
  int sec = t.sec;

  while (true) {
    sleep_ms(10);
    rtc_get_datetime(&t);
    if (t.sec != sec) {
      return;
    }    
  }
}

void buttonPressed(uint gpio, uint32_t events) {
  static absolute_time_t lastTime = make_timeout_time_us(0);

  absolute_time_t absTime = get_absolute_time(); 

  if (absolute_time_diff_us(lastTime, absTime) < 300000) {   
    return;
  }

  lastTime = absTime;

  if (showingMenu) {
    //This is dealt with elsewhere in a tight loop not an interrupt
    return;
  }

  if (gpio == BTN_A)
  {
    setButtonInterrupts(false);
    
    showingMenu = true;  
    return;    
  }

  if (gpio == BTN_B)
  {
    uint64_t timeNow = 0;
    
    if (timerLeft == 0) {
      //Timer not set so cannot pause or un-pause
      return;
    }

    timeNow = time_us_64();

    if (timerRunning) {
      timerLeft = timerLeft-(timeNow-timerLastChecked);    
    }

    timerLastChecked = timeNow;
    timerRunning = !timerRunning;

    if(timerRunning) {
      setTimerAlarm();

      showTimerAlarmCompleteCountDown = 1;
      char* message = (char *)"Strt";
      TM1637_display_word(message, true);
    }
    else {
      cancelTimerAlarm();

      showTimerAlarmCompleteCountDown = 1;
      char* message = (char *)"StoP";
      TM1637_display_word(message, true);
    }
  } 

  if (gpio == BTN_X)
  {
    showTimer = false;     
  }

  if (gpio == BTN_Y)
  {
    showTimer = true;     
  }
}

void setButtonInterrupts(bool enabled) {
  gpio_set_irq_enabled_with_callback(BTN_A, 4, enabled, buttonPressed);
  gpio_set_irq_enabled_with_callback(BTN_B, 4, enabled, buttonPressed);
  gpio_set_irq_enabled_with_callback(BTN_X, 4, enabled, buttonPressed);
  gpio_set_irq_enabled_with_callback(BTN_Y, 4, enabled, buttonPressed); 
}

void showTheClockRunning() {
  int resetInterrupts = true;  

  //sleep_until does not run as consistently (time wise) as rtc_set_alarm - quite often calls slighly ahead of time
  absolute_time_t absTime = get_absolute_time();  

  waitUntilAtBeginningOfSecond();
    
  for(;;) {
    if (showingMenu) {
      return;
    }

    if (!showingMenu) {    
      if (showTimerAlarmCompleteCountDown > 1) {
        //Do not want this interrupted so remove the interrupts
        resetInterrupts = false;
        setButtonInterrupts(false);        

        //Need to show this for 5 seconds
        showTimerAlarmCompleteCountDown--;        
      }
      else if (showTimerAlarmCompleteCountDown == 1) {
        showTimerAlarmCompleteCountDown--;  

        //Turn off audio
        set_tone(500, 0);

        resetInterrupts = true;  
      }
      else if (showTimer) {
          drawTimerSimple(); 
      }
      else {
          drawClockSimple();              
      }
    }      

    //Set up interrupts for buttons  
    if (resetInterrupts) {
      resetInterrupts = false;
      setButtonInterrupts(true);
    }

    absTime = delayed_by_ms(absTime, 1000);
    sleep_until(absTime);    
  }
}

void startRTC(int hour, int min) {
  rtc_init();
  resetRTC(hour, min);
}

void setClockTime(int& hour, int &min) {
  //This needs to be refactored out as there are another two functions very similar to this
  int pos = 1;  

  drawTimeItem(hour, min, pos);
  sleep_ms(BUTTON_DEBOUNCE_PAUSE);

  int count = 0;

  while (true) {
    sleep_ms(10);

    //Flash the correct timer item
    if (count == 100) {
      count = 0;
      drawTimeItem(hour, min, pos);       
    }

    if (count == 50) {
      TM1637_display_dash_and_number(hour, min, pos, true);                 
    }

    count++;

    if(isPressed(BTN_A)) {
      pos--;
      if (pos < 1) {
        pos = 1;
      }  

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_B)) {
      pos++;
      if (pos > 2) {
        char* menu = (char *)"SEt ";
        TM1637_display_word(menu, true);

        sleep_ms(1000);
        buttonPressed(0, 0); //All this does is set the static so that the first button press is not ignored
        return;
      }

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_X)) {
      if (pos == 1) {
        hour++;
        if (hour > 23) {
          hour = 0;
        }
      }
      else {
        min++;
        if (min > 59) {
          min = 0;
        }
      }

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }

    if(isPressed(BTN_Y)) {
      if (pos == 1) {
        hour--;
        if (hour < 0) {
          hour = 23;
        }
      }
      else {
        min--;
        if (min < 0) {
          min = 59;
        }
      }

      drawTimeItem(hour, min, pos);
      sleep_ms(BUTTON_DEBOUNCE_PAUSE);
    }
  }
}

int main() {
    stdio_init_all();

    gpio_set_function(BTN_A, GPIO_FUNC_SIO); gpio_set_dir(BTN_A, GPIO_IN); gpio_pull_up(BTN_A);
    gpio_set_function(BTN_B, GPIO_FUNC_SIO); gpio_set_dir(BTN_B, GPIO_IN); gpio_pull_up(BTN_B);
    gpio_set_function(BTN_X, GPIO_FUNC_SIO); gpio_set_dir(BTN_X, GPIO_IN); gpio_pull_up(BTN_X);
    gpio_set_function(BTN_Y, GPIO_FUNC_SIO); gpio_set_dir(BTN_Y, GPIO_IN); gpio_pull_up(BTN_Y);

    TM1637_init(CLK_PIN, DIO_PIN);  
    TM1637_clear(); 
    
    TM1637_set_brightness(globalBrightness);

    setAudio();

    int hour = 0;
    int min = 0;

    setClockTime(hour, min);

    startRTC(hour, min);  //Not as accurate when the next second shows but no problem with propagating beyond 60 seconds 

    setButtonInterrupts(true);

    while (true) {    
        if (showingMenu) {
            showMenu();
        }
        else {
            showTheClockRunning();
        }
    }
}
