#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <AggModeEnum.h>

//variables from main file
extern WiFiClientSecure secureWifiClient;
extern HTTPClient httpClient;

//requires global scope SecureWifiClient and HTTPClient

//function declarations
String fetchAzureAccessTokenSP(String tenantId, String appId, String secretKey);
float fetchCost(String billingAccount, aggMode aggregationMode);

//fetch azure access token using service principal
String fetchAzureAccessTokenSP(String tenantId, String appId, String secretKey) {

  String requestURL = "https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/token";

  String postData = "client_id=" + appId + "&grant_type=client_credentials" + "&scope=https://management.azure.com/.default" + "&client_secret=" + secretKey;

  //format http request
  httpClient.begin(secureWifiClient, requestURL);
  httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");

  //get response
  int responseCode = httpClient.POST(postData);

  if (responseCode > 0) {

    if (responseCode == HTTP_CODE_OK) {
            
      String payload = httpClient.getString();

      // parse out the access token from the response
      int startIdx = payload.indexOf("access_token\":\"") + strlen("access_token\":\"");
      int endIdx = payload.indexOf("\"", startIdx);
      String accessToken = payload.substring(startIdx, endIdx);

      httpClient.end();
      return accessToken;

    } else {

      Serial.print("Failed, HTTP response code: " + String(responseCode));

    }

  } else {

    Serial.println("Connection ERROR");

  }

  httpClient.end();
  return "";

}

float fetchCost(String billingAccount, aggMode aggregationMode, String accessToken) {

  String requestURL = "https://management.azure.com/providers/Microsoft.Billing/billingAccounts/" + billingAccount + "/providers/Microsoft.CostManagement/query?api-version=2023-11-01";

  String aggModeText;
  String granularity = "";

  if (aggregationMode == YEAR_TO_DATE) {

    aggModeText = "YearToDate";

  } else if (aggregationMode == MONTH_TO_DATE) {

    aggModeText = "MonthToDate";

  } else if (aggregationMode == TODAY) {

    aggModeText = "MonthToDate";
    granularity = "\"granularity\": \"Daily\",";

  }

  String postString = "{\"type\":\"ActualCost\",\"timeframe\":\"" + aggModeText + "\",\"dataset\":{" + granularity + "\"aggregation\":{\"totalCost\":{\"name\":\"Cost\",\"function\":\"Sum\"}}}}";

  httpClient.begin(secureWifiClient, requestURL);
  httpClient.addHeader("Authorization", "Bearer " + accessToken);
  httpClient.addHeader("Content-Type", "application/json");
  
  //get response
  int responseCode = httpClient.POST(postString);

  if (responseCode > 0) {

    if (responseCode == HTTP_CODE_OK) {
            
      String payload = httpClient.getString();

      StaticJsonDocument<1024> doc;

      // Parse the JSON response
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {

        Serial.print(F("Failed to parse JSON: "));
        Serial.println(error.f_str());
        return -1.0;  
        
      }

      float cost = -1.0;

      // Extract based on aggregation mode
      switch (aggregationMode) {

        case YEAR_TO_DATE:

          cost = doc["properties"]["rows"][0][0].as<float>();
          break;

        case MONTH_TO_DATE:

          cost = doc["properties"]["rows"][0][0].as<float>();
          break;

        case TODAY:
          
          JsonArray rows = doc["properties"]["rows"].as<JsonArray>();

          if (!rows.isNull() && rows.size() > 0) {

            cost = rows[rows.size() - 1][0].as<float>();

          }

          break;
      }

      httpClient.end();
      return cost;

    } else if (responseCode == 401) {

      return -2.0;

    } else {

      Serial.print("Failed, HTTP response code: " + String(responseCode));

    }

  } else {

    Serial.println("Connection ERROR");

  }

  httpClient.end();
  return -1.0;

}