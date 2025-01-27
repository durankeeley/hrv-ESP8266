// Credit to: Spencer (data structure) http://www.hexperiments.com/?page_id=47
// Credit to: chimera (Original Logic) - https://www.geekzone.co.nz/forums.asp?forumid=141&topicid=195424
// Credit to: millst (TX/RX on the same pin) - https://www.geekzone.co.nz/forums.asp?forumid=141&topicid=195424&page_no=2#2982537

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include "secrets.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "serial_manager.h"
#include "utils.h"
#include "constants.h"

#include "secrets.h"

// Include the external declarations
#include "constants.h"

// custom modules
#include "wifi_manager.h"    // startWIFI()
#include "mqtt_manager.h"    // startMQTT()
#include "serial_manager.h"  // checkSwSerial()
#include "utils.h"           // decToHex()

// Pin definitions
#define D6 (12)
const byte MSGSTARTSTOP = 0x7E;

// WiFi / MQTT
WiFiClient   wifiClient;
PubSubClient client(wifiClient);
IPAddress    ipadd;
char         packetBuffer[255];
int          iTotalDelay = 0;

// MQTT settings
int          mqtt_port             = 1883;
const char*  topic                 = "hrv/status";
const char*  MQTT_TARGET_FAN_SPEED = "hrv/targetfanspeed";
const char*  MQTT_ROOF_TEMP        = "hrv/rooftemp";
const char*  MQTT_FAN_SPEED        = "hrv/currentfanspeed";
const char*  MQTT_TARGETFAN_SPEEDBYTE        = "hrv/fanspeedbyte";

// Debug flags
bool debug_console_enable                            = true;
bool debug_console_hrvController_currentRoofTemperature = true;
bool debug_console_mqtt_targetFanSpeed               = true;
bool debug_console_mqtt_brokerConnection             = false;
bool debug_console_mqtt_msgFanSpeedTopic             = false;
bool debug_console_serial_txMessage                  = false;
bool debug_console_serial_rxDataAvailable            = false;
bool debug_console_serial_rxReadData                 = false;
bool debug_console_serial_rxStartingData             = false;
bool debug_console_wifi_connection                   = false;
// Mock Debug
bool debug_mockRoofTemp = false;
float mockRoofTempValue  = 32.0;

// serial and temperature data
byte  serialData[10];
byte  dataIndex        = 0;
byte  checksumIndex    = 0;
bool  dataStarted      = false;
bool  dataReceived     = false;
float currentRoofTemperature = 0.0;
float lastRoofTemperatures[3] = {0.0, 0.0, 0.0};
byte  targetFanSpeed;
byte  lastTargetFanSpeed = 0;
char  tempLocation     = 'R';
char  HRVTemperature_buff[16];
char  FanSpeed_buff[16];
String txMessage;
String mqttTargetFanSpeed;
String mqttPublishHRVTemperature;

// timing variables and constants:
unsigned long previousReadMillis = 0;
unsigned long previousMQTTMillis = 0;
const unsigned long SERIAL_READ_INTERVAL = 5000;      // 5 seconds
const unsigned long MQTT_PUBLISH_INTERVAL = 3000;     // 3 seconds

// First Boot Variables
bool firstBoot = true;
bool firstValidReading = false;
int validReadingsCount = 0;

SoftwareSerial hrvSerial;

void setup() {
  if (debug_console_enable) {
    Serial.begin(115200);
    Serial.println(F("Booting..."));
  }

  // Note: RX/TX on the same pin, so setting half-duplex, buffer size 256
  hrvSerial.begin(1200, SWSERIAL_8N1, D6, D6, false, 256);
  hrvSerial.enableIntTx(false); // disable tx interrupt

  targetFanSpeed = 0x00;
  mqttTargetFanSpeed = "";

  startWIFI();
  startMQTT();

  client.publish(topic, "on");
}

void loop() {
  if (client.connected()) {
    client.loop();
  } else {
    reconnectMQTTIfNeeded();
  }

  unsigned long currentMillis = millis();

  if (currentMillis - previousReadMillis >= SERIAL_READ_INTERVAL) {
    previousReadMillis = currentMillis;

    // Write and read from HRV
    // targetFanSpeed = 0;
    checkSwSerial(&hrvSerial);

    if (debug_console_hrvController_currentRoofTemperature) {
      Serial.print(F("Current Roof Temperature: "));
      Serial.println(currentRoofTemperature);
    }

    // Check if the reading is valid (non-zero)
    if (currentRoofTemperature != 0.0f) { 
      // Store the temperature in the history
      lastRoofTemperatures[2] = lastRoofTemperatures[1];
      lastRoofTemperatures[1] = lastRoofTemperatures[0];
      lastRoofTemperatures[0] = currentRoofTemperature;

      if (firstBoot && validReadingsCount < 3) {
        validReadingsCount++;
        Serial.print(F("Valid Readings Count: "));
        Serial.println(validReadingsCount);
        if (validReadingsCount >= 3) {
          firstValidReading = true;
          firstBoot = false;
          Serial.println(F("Initialized with 3 valid temperature readings."));
        }
      }
    }

    // out-of-range check if at least 3 valid readings to compare
    if (firstValidReading) {
      bool withinRange = true;
      for (int i = 0; i < 2; i++) {
        if (fabs(lastRoofTemperatures[i] - lastRoofTemperatures[i + 1]) > 2.0f) {
          withinRange = false;
          break;
        }
      }

      if (!withinRange) {
        if (debug_console_hrvController_currentRoofTemperature) {
          Serial.println(F("Temperature jump beyond 2 degrees, restarting device..."));
        }
        lastTargetFanSpeed = targetFanSpeed;
        String lastFanSpeedStr = String(lastTargetFanSpeed);
        client.publish(MQTT_TARGET_FAN_SPEED, lastFanSpeedStr.c_str(), true); // retain=true

        // clear softwareserial
        hrvSerial.read();
        hrvSerial.end();

        // reset variables
        currentRoofTemperature = 0.0f;
        lastRoofTemperatures[0] = 0.0f;
        lastRoofTemperatures[1] = 0.0f;
        lastRoofTemperatures[2] = 0.0f;
        firstBoot = true;
        firstValidReading = false;
        validReadingsCount = 0;

        // restart the softwareserial
        hrvSerial.begin(1200, SWSERIAL_8N1, D6, D6, false, 256);
        hrvSerial.enableIntTx(false); 

        client.subscribe(MQTT_TARGET_FAN_SPEED);
        targetFanSpeed = lastTargetFanSpeed;

      } 
      else {
        String mqttPublishHRVTemperatureStr = String(currentRoofTemperature);
        mqttPublishHRVTemperatureStr.toCharArray(HRVTemperature_buff, sizeof(HRVTemperature_buff));
        client.publish(MQTT_ROOF_TEMP, HRVTemperature_buff);
      }
    }
  }

  if (currentMillis - previousMQTTMillis >= MQTT_PUBLISH_INTERVAL) {
    previousMQTTMillis = currentMillis;

    targetFanSpeed = mqttTargetFanSpeed.toInt();

    // Publish current fan speed to MQTT
    String mqttPublishFanSpeed = String(targetFanSpeed);
    String mqttPublishFanSpeedByte = String(targetFanSpeed, HEX);
    mqttPublishFanSpeed.toCharArray(FanSpeed_buff, sizeof(FanSpeed_buff));
    if (debug_console_mqtt_targetFanSpeed) {
      Serial.print(F("Fan Speed: "));
      Serial.println(mqttPublishFanSpeed);
      Serial.print(F("Fan Speed Byte: "));
      Serial.println(mqttPublishFanSpeedByte);
    }
    client.publish(MQTT_FAN_SPEED, FanSpeed_buff);
    client.publish(MQTT_TARGETFAN_SPEEDBYTE, mqttPublishFanSpeedByte.c_str());
  }
}

void reconnectMQTTIfNeeded() {
  if (!client.connected()) {
    while (!client.connected()) {
      if (client.connect("hrv-client-reconnect", mqtt_username, mqtt_password)) {
        if (debug_console_mqtt_brokerConnection) {
          Serial.println(F("Reconnected to MQTT!"));
        }
        client.subscribe(MQTT_TARGET_FAN_SPEED);
      } else {
        if (debug_console_mqtt_brokerConnection) {
          Serial.print(F("MQTT connect failed, rc="));
          Serial.println(client.state());
          Serial.println(F(" Trying again in 2 seconds..."));
        }
        delay(2000);
      }
    }
  }
}
