#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <wifiFunctions.h>
#include <AzureAPIFunctions.h>
#include <AggModeEnum.h>

//declare secure wifi client and http client
WiFiClientSecure secureWifiClient;
HTTPClient httpClient;

//network credentials, later will be initialized via bluetooth
String SSID = "eduroam";
String WIFI_USERNAME = "sjaskowski3@gatech.edu";
String WIFI_PASSWORD = "sJ!2003March4";

//azure credentials, later will be initialized via bluetooth
String AZURE_TENANT_ID = "482198bb-ae7b-4b25-8b7a-6d7f32faa083";
String AZURE_SP_APP_ID = "291f3150-93cf-424c-a2a7-3365001f1b34";
String AZURE_SP_SK = "WNZ8Q~JzO3u7uurPGb.aH1kAMgI0mik54VLYuaO6";
String BILLING_ACCOUNT_ID = "3f6e673c-aaeb-4c82-a5cf-07f492ecd074:e8087449-5d49-4633-ba1b-d0b078088dc1_2019-05-31";

//hold azure access token
String azureAccessToken = "";

//aggregation mode
aggMode currAggMode = MONTH_TO_DATE;

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

}

void loop() {
  
  //first, if azure access token is not set, fetch azure access token
  if (azureAccessToken == "") {

    azureAccessToken = fetchAzureAccessTokenSP(AZURE_TENANT_ID, AZURE_SP_APP_ID, AZURE_SP_SK);

  }

  //send request for cost data
  float costData = fetchCost(BILLING_ACCOUNT_ID, currAggMode, azureAccessToken);

  //if costData is -2, that means accessToken has expired (401 returned from api call), and we need to regenerate the token
  if (costData == -2.0) {

    azureAccessToken = "";
    continue;

  }

  Serial.println(costData);

  delay(5000);

}
