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

void setup() {
  // put your setup code here, to run once:

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
  pinMode(YTD_LED_PIN, OUTPUT);
  pinMode(MTD_LED_PIN, OUTPUT);
  pinMode(TDY_LED_PIN, OUTPUT);

  //add button handlers
  attachInterrupt(digitalPinToInterrupt(ALARM_BUTTON_PIN), handleAlarmButtonPress, RISING);
  attachInterrupt(digitalPinToInterrupt(TOGGLE_AGG_PIN), handleAggTogglePress, RISING);

}

void loop() {
  
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

  } else {

    sendCostAndAlarmToDisplay(0.0, true);

  }

  setAggLeds();
  Serial.print("Aggregation Mode: ");
  Serial.println(currAggMode);
  Serial.print("Alarm Mode Activated: ");
  Serial.println(alarmMode);

  loopCounter++;
  delay(LOOP_DELAY);

}
