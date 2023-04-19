#include "wifi_manager.h"

extern int iTotalDelay;
extern const char* ssid;
extern const char* password;

// Starts WIFI connection
void startWIFI() {
  // If we are not connected
  if (WiFi.status() != WL_CONNECTED) {
    int connectResult;
    Serial.println("Initiating WiFi connection process.");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); 
    WiFi.begin(ssid, password);

    // Wait for connection result
    connectResult = WiFi.waitForConnectResult();

    // If not WiFi connected, retry every 2 seconds for 15 minutes
    while (connectResult != WL_CONNECTED) {
      Serial.print(".");
      delay(2000);
      connectResult = WiFi.waitForConnectResult();
      // If can't get to Wifi for 15 minutes, reboot ESP
      if (iTotalDelay > 900000)
      {
        Serial.println("Excessive WiFi connection attempts detected, initiating reboot.");
        ESP.reset();
      }
      iTotalDelay+=2000;
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());
    // Let network have a chance to start up
    myDelay(1500);
  }
}

// This function yields back to the watchdog to avoid random ESP8266 resets
void myDelay(int ms)  
{
  int i;
  for(i=1; i!=ms; i++) 
  {
    delay(1);
    if(i%100 == 0) 
   {
      ESP.wdtFeed(); 
      yield();
    }
  }
  iTotalDelay+=ms;
}