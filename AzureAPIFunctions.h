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
bool haltAllVM(String subscriptionId);

//fetch azure access token using service principal
String fetchAzureAccessTokenSP(String tenantId, String appId, String secretKey, String scope) {

  String requestURL = "https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/token";

  String postData = "client_id=" + appId + "&grant_type=client_credentials" + "&scope=" + scope + "&client_secret=" + secretKey;

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

bool haltAllVM(String subscriptionId, String accessToken) {
  
  String requestURL = "https://management.azure.com/subscriptions/" + subscriptionId + "/providers/Microsoft.Compute/virtualMachines?api-version=2024-07-01?statusOnly=true";

  httpClient.begin(secureWifiClient, requestURL);
  httpClient.addHeader("Authorization", "Bearer " + accessToken);

  int responseCode = httpClient.GET();

  if (responseCode > 0) {

    if (responseCode == HTTP_CODE_OK) {
            
      String payload = httpClient.getString();

      httpClient.end();

      StaticJsonDocument<4096> doc;

      // Parse the JSON response
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {

        Serial.print(F("Failed to parse JSON: "));
        Serial.println(error.f_str());
        return false;  
        
      }

      //vms
      JsonArray vms = doc["value"].as<JsonArray>();

      //iterate over vms
      for (JsonObject vm : vms) {

        String vmName = vm["name"].as<String>();
        String vmId = vm["id"].as<String>();

        String resourceGroup = vmId.substring(vmId.indexOf("/resourceGroups/") + strlen("/resourceGroups/"), vmId.indexOf("/providers/", vmId.indexOf("/resourceGroups/") + strlen("/resourceGroups/")));

        //check if is active
        JsonArray statuses = vm["properties"]["instanceView"]["statuses"].as<JsonArray>();

        if (statuses.isNull() || statuses.size() == 0) {

          continue;

        }

        if (statuses[statuses.size() - 1]["code"].as<String>() == "PowerState/deallocated") {

          continue;

        }

        //now, we need to deallocate the vm
        String deallocationRequestURL = "https://management.azure.com/subscriptions/" + subscriptionId + "/resourceGroups/"+ resourceGroup + "/providers/Microsoft.Compute/virtualMachines/" + vmName + "/deallocate?api-version=2024-07-01";

        httpClient.begin(secureWifiClient, deallocationRequestURL);
        httpClient.addHeader("Authorization", "Bearer " + accessToken);

        int deallocationResponseCode = httpClient.POST("");

        if (deallocationResponseCode != HTTP_CODE_ACCEPTED) {

          Serial.println("Deallocation on " + vmName + " Virtual Machine Failed.");

          httpClient.end();
          return false;

        } else {

          httpClient.end();
          return true;

        }


      }

    } else {

      Serial.print("Failed, HTTP response code: " + String(responseCode));
      httpClient.end();

    }

  } else {

    Serial.println("Connection ERROR");
    httpClient.end();

  }

}