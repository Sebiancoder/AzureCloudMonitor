#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <wifiFunctions.h>
#include <AzureAPIFunctions.h>
#include <AggModeEnum.h>
#include <MotorControlTransmitter.h>
#include <AlarmControl.h>
#include <ToggleControl.h>
#include <Shutdown.h>

//time between each run of logic loop
const int LOOP_DELAY = 1000;

//number of milliseconds between each Azure call (to prevent 429 too many requests errors)
const int AZURE_CALL_FREQUENCY = 15000;

//count number of loops
int loopCounter = 0;

//declare secure wifi client and http client
WiFiClientSecure secureWifiClient;
HTTPClient httpClient;

//network credentials, later will be initialized via bluetooth
String SSID = "eduroam";
String WIFI_USERNAME = "sjaskowski3@gatech.edu";
String WIFI_PASSWORD = "sJ!2003March4";

//azure scope constants
const String MANAGEMENT_SCOPE = "https://management.azure.com/.default";

//azure credentials, later will be initialized via bluetooth
String AZURE_TENANT_ID = "482198bb-ae7b-4b25-8b7a-6d7f32faa083";
String SUBSCRIPTION_ID = "c5aebeeb-38f0-4fe1-8a9c-f383c06e478a";
String AZURE_SP_APP_ID = "291f3150-93cf-424c-a2a7-3365001f1b34";
String AZURE_SP_SK = "WNZ8Q~JzO3u7uurPGb.aH1kAMgI0mik54VLYuaO6";
String BILLING_ACCOUNT_ID = "3f6e673c-aaeb-4c82-a5cf-07f492ecd074:e8087449-5d49-4633-ba1b-d0b078088dc1_2019-05-31";

//hold azure access token
String azureAccessToken = "";

//aggregation mode
aggMode currAggMode = TODAY;

//curr cost value to display
float currDisplayValue = 0.0;

//alarmMode
bool alarmMode = false;

//current alarm thresholds
float alarmThresholdYTD = 0;
float alarmThresholdMTD = 0;
float alarmThresholdTDY = 0;

//whether alarm activated
bool alarmActivated = false;

//if true, vm shutdown message will be sent on next cycle
bool shutdownVMs = false;

//number of maximum attempts to shutdown vms
const int MAX_VM_SHUTDOWN_ATTEMPTS = 10;

//number of current shutdown attempts
int vmShutdownAttempts = 0;


void setup() {

  //serial stream
  Serial.begin(115200);

  //wifi configuration
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  secureWifiClient.setInsecure();

  bool wifiSuccess = connectToWifiEAP(SSID, WIFI_USERNAME, WIFI_PASSWORD, 30);

  if (!wifiSuccess) {

    Serial.println("Failed to Connect to WIFI");

  }

  //set up transmission to Motor Control Microprocessor
  Wire.begin();

  //set up pins
  pinMode(ALARM_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TOGGLE_AGG_PIN, INPUT_PULLUP);
  pinMode(SHUTDOWN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(YTD_LED_PIN, OUTPUT);
  pinMode(MTD_LED_PIN, OUTPUT);
  pinMode(TDY_LED_PIN, OUTPUT);
  pinMode(ALARM_LIGHT_PIN, OUTPUT);
  pinMode(ALARM_SOUND_PIN, OUTPUT);

  //add button handlers
  attachInterrupt(digitalPinToInterrupt(ALARM_BUTTON_PIN), handleAlarmButtonPress, RISING);
  attachInterrupt(digitalPinToInterrupt(TOGGLE_AGG_PIN), handleAggTogglePress, RISING);
  attachInterrupt(digitalPinToInterrupt(SHUTDOWN_BUTTON_PIN), handleShutdownPress, RISING);

}

void loop() {

  Serial.println("------------------------");
  
  if (!alarmMode) {
  
    if ((loopCounter * LOOP_DELAY) >= AZURE_CALL_FREQUENCY) {

      //first, if azure access token is not set, fetch azure access token
      if (azureAccessToken == "") {

        azureAccessToken = fetchAzureAccessTokenSP(AZURE_TENANT_ID, AZURE_SP_APP_ID, AZURE_SP_SK, MANAGEMENT_SCOPE);

      }

      //send request for cost data
      float costData = fetchCost(BILLING_ACCOUNT_ID, currAggMode, azureAccessToken);

      //if costData is -2, that means accessToken has expired (401 returned from api call), and we need to regenerate the token
      if (costData == -2.0) {

        azureAccessToken = "";
        return;

      } else if (costData == -1.0) {

        return;

      }

      Serial.println(azureAccessToken);

      currDisplayValue = costData;

      loopCounter = 0;

    }
    
    sendCostAndAlarmToDisplay(currDisplayValue, alarmMode);

    //keep alarm light off if alarm is not activated
    if (!alarmActivated) {

      digitalWrite(ALARM_LIGHT_PIN, LOW);

    }

  } else {

    sendCostAndAlarmToDisplay(0.0, true);

    digitalWrite(ALARM_LIGHT_PIN, HIGH);

    //set alarm threshold
    switch (currAggMode) {

      case YEAR_TO_DATE:

        alarmThresholdYTD = requestAlarmThreshold();
        break;

      case MONTH_TO_DATE:

        alarmThresholdMTD = requestAlarmThreshold();
        break;

      case TODAY:

        alarmThresholdTDY = requestAlarmThreshold();
        break;

    }

    Serial.print("YTD Alarm: ");
    Serial.println(alarmThresholdYTD);
    Serial.print("MTD Alarm: ");
    Serial.println(alarmThresholdMTD);
    Serial.print("TODAY Alarm: ");
    Serial.println(alarmThresholdTDY);

  }

  setAggLeds();
  Serial.print("Aggregation Mode: ");
  Serial.println(currAggMode);
  Serial.print("Alarm Mode Activated: ");
  Serial.println(alarmMode);

  //vm shutdown
  if (shutdownVMs) {

    Serial.println("Attempting VM Shutdown ... ");

    bool shutdown_success = haltAllVM(SUBSCRIPTION_ID, azureAccessToken);

    if (shutdown_success) {

      shutdownVMs = false;
      vmShutdownAttempts = 0;

      //sound a success tone
      tone(ALARM_SOUND_PIN, 294);
      delay(200);
      tone(ALARM_SOUND_PIN, 330);
      delay(200);
      tone(ALARM_SOUND_PIN, 350);
      delay(200);
      noTone(ALARM_SOUND_PIN);

    } else {

      vmShutdownAttempts += 1;

    }

    if (vmShutdownAttempts > MAX_VM_SHUTDOWN_ATTEMPTS) {

      shutdownVMs = false;
      vmShutdownAttempts = 0;

      //sound a failure tone
      tone(ALARM_SOUND_PIN, 350);
      delay(200);
      tone(ALARM_SOUND_PIN, 330);
      delay(200);
      tone(ALARM_SOUND_PIN, 294);
      delay(200);
      noTone(ALARM_SOUND_PIN);

    }

  }
  
  //this function will check if alarm should be turned on, and will sound the alarm if so.
  handleAlarmActivation(currDisplayValue);

  loopCounter++;
  delay(LOOP_DELAY);

}
