#include <WiFi.h>

//function definitions
bool connectToWifiEAP(String ssid, String username, String password, int timeout);

//connect to Wifi with WPA2-EAP (for eduroam)
bool connectToWifiEAP(String ssid, String username, String password, int timeout) {

  WiFi.begin(ssid, WPA2_AUTH_PEAP, username, username, password);
  WiFi.setHostname("AzureCloudSpendMonitor");
  WiFi.enableSTA(true);

  Serial.print("Connecting to Wifi...");

  int time_elapsed = 0;

  while (WiFi.status() != WL_CONNECTED) {

    delay(1000);
    Serial.print(".");

    time_elapsed = time_elapsed + 1;

    if (time_elapsed > timeout) {

      return false;

    }

  }

  Serial.println("Wifi Connection Successful");

  return true;

}