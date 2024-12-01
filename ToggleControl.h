#include <AggModeEnum.h>

extern aggMode currAggMode;
extern int loopCounter;

const int TOGGLE_AGG_PIN = 16;
const int YTD_LED_PIN = 19;
const int MTD_LED_PIN = 18;
const int TDY_LED_PIN = 17;

//switch bounce prevention timer
unsigned long lastAggToggle = 0;


void setAggLeds() {

  switch (currAggMode) {

    case YEAR_TO_DATE:

      digitalWrite(YTD_LED_PIN, HIGH);
      digitalWrite(MTD_LED_PIN, LOW);
      digitalWrite(TDY_LED_PIN, LOW);
      break;

    case MONTH_TO_DATE:

      digitalWrite(YTD_LED_PIN, LOW);
      digitalWrite(MTD_LED_PIN, HIGH);
      digitalWrite(TDY_LED_PIN, LOW);
      break;

    case TODAY:

      digitalWrite(YTD_LED_PIN, LOW);
      digitalWrite(MTD_LED_PIN, LOW);
      digitalWrite(TDY_LED_PIN, HIGH);
      break;

  }

}

void IRAM_ATTR handleAggTogglePress() {

  unsigned long pressTime = millis();

  if ((pressTime - lastAggToggle) < 1000) {

    return;

  }

  lastAggToggle = pressTime;
  
  switch (currAggMode) {

    case YEAR_TO_DATE:

      currAggMode = MONTH_TO_DATE;
      break;

    case MONTH_TO_DATE:

      currAggMode = TODAY;
      break;

    case TODAY:

      currAggMode = YEAR_TO_DATE;
      break;

  }

  setAggLeds();

  loopCounter = 15;

}
