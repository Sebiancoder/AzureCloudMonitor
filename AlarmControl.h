#include <AggModeEnum.h>

extern aggMode currAggMode;
extern bool alarmMode;
extern bool alarmActivated;

extern float alarmThresholdYTD;
extern float alarmThresholdMTD;
extern float alarmThresholdTDY;

const int ALARM_BUTTON_PIN = 14;
const int ALARM_LIGHT_PIN = 26;
const int ALARM_SOUND_PIN = 25;

//switch bounce prevention timer
unsigned long lastAlarmButtonToggle = 0;

void IRAM_ATTR handleAlarmButtonPress() {
  
  unsigned long pressTime = millis();

  if ((pressTime - lastAlarmButtonToggle) < 1000) {

    return;

  }

  lastAlarmButtonToggle = pressTime;
  
  //if this button is pressed when the alarm is activated, turn off the alarm and reset the corresponding threshold
  if (alarmActivated) {

    alarmActivated = false;

    switch (currAggMode) {

      case YEAR_TO_DATE:

        alarmThresholdYTD = 0;
        break;

      case MONTH_TO_DATE:

        alarmThresholdMTD = 0;
        break;

      case TODAY:

        alarmThresholdTDY = 0;
        break;

    }

  } else {

    //now we handle the case when the alarm is not activated, and the user just wants to toggle alarm mode
    alarmMode = !alarmMode;

  }

}