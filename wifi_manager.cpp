#include "wifi_manager.h"
#include "constants.h"

// Starts WIFI connection
void startWIFI() {
  // If we are not connected
  if (WiFi.status() != WL_CONNECTED) {
    int connectResult;
    if (debug_console_wifi_connection) {
      Serial.println("Initiating WiFi connection process.");
    }
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); 
    WiFi.begin(ssid, password);

    // Wait for connection result
    connectResult = WiFi.waitForConnectResult();

    // If not WiFi connected, retry every 2 seconds for 15 minutes
    while (connectResult != WL_CONNECTED) {
      if (debug_console_wifi_connection) {
        Serial.print(".");
      }
      delay(2000);
      connectResult = WiFi.waitForConnectResult();
      // If can't get to Wifi for 15 minutes, reboot ESP
      if (iTotalDelay > 900000)
      {
        if (debug_console_wifi_connection) {
          Serial.println("Excessive WiFi connection attempts detected, initiating reboot.");
        }
        ESP.reset();
      }
      iTotalDelay+=2000;
    }

    if (debug_console_wifi_connection) {
      Serial.println("WiFi connected");
      Serial.println(WiFi.localIP());
    }
    // Let network have a chance to start up
    myDelay(1500);
  }
}
