#include <AggModeEnum.h>

extern currAggMode;
extern alarmMode;
extern alarmActivated;

extern alarmThresholdYTD;
extern alarmThresholdMTD;
extern alarmThresholdTDY;

const int ALARM_BUTTON_PIN = 13;

void IRAM_ATTR handleAlarmButtonPress() {
  
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