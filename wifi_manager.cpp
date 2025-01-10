#include "wifi_manager.h"
#include "constants.h"
#include "secrets.h"

/// Starts WIFI connection
void startWIFI() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (debug_console_wifi_connection) {
    Serial.println("Initiating WiFi connection process.");
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  int connectResult = WiFi.waitForConnectResult();
  int totalDelay = 0;

  while (connectResult != WL_CONNECTED) {
    if (debug_console_wifi_connection) {
      Serial.print(".");
    }
    delay(2000);
    totalDelay += 2000;

    if (totalDelay > 900000) { // 15 minutes
      if (debug_console_wifi_connection) {
        Serial.println("Excessive WiFi connection attempts detected, initiating reboot.");
      }
      ESP.reset();
    }

    connectResult = WiFi.waitForConnectResult();
  }

  if (debug_console_wifi_connection) {
    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());
  }

  // Let network have a chance to start up
  delay(1500);
}
