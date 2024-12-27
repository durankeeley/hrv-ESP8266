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
int          mqtt_port             = 1883;             // e.g. 1883
const char*  topic                 = "hrv/status";
const char*  MQTT_TARGET_FAN_SPEED = "hrv/targetfanspeed";
const char*  MQTT_ROOF_TEMP        = "hrv/rooftemp";
const char*  MQTT_FAN_SPEED        = "hrv/currentfanspeed";

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

// Serial / temperature data
byte  serialData[10];

byte  dataIndex        = 0;
byte  checksumIndex    = 0;
bool  dataStarted      = false;
bool  dataReceived     = false;
float currentRoofTemperature = 0.0;
float lastRoofTemperatures[3] = {0.0, 0.0, 0.0};
byte  targetFanSpeed   = 0x00;
byte  lastTargetFanSpeed = 0;
char  tempLocation     = 'R';

// Buffers
char  HRVTemperature_buff[16];
char  FanSpeed_buff[16];

// Strings
String txMessage;
String mqttTargetFanSpeed;
String mqttPublishHRVTemperature;

// Define your timing variables and constants:
unsigned long previousReadMillis = 0;
unsigned long previousMQTTMillis = 0;
const unsigned long SERIAL_READ_INTERVAL = 5000;      // e.g. 5 seconds
const unsigned long MQTT_PUBLISH_INTERVAL = 3000;     // e.g. 3 seconds

// First Boot Variables
bool firstBoot = true;
bool firstValidReading = false;
int validReadingsCount = 0;

// We'll set up a SoftwareSerial for HRV if needed
SoftwareSerial hrvSerial;

void setup() {

  // Debug USB Serial
  if (debug_console_enable) {
    Serial.begin(115200);
    Serial.println(F("Booting..."));
  }

  // Note: We use the same pin for RX/TX, so we set half-duplex, buffer size 256
  hrvSerial.begin(1200, SWSERIAL_8N1, D6, D6, false, 256);
  hrvSerial.enableIntTx(false); // disable tx interrupt

  targetFanSpeed = 0x00;
  mqttTargetFanSpeed = "";

  startWIFI();  // from wifi_manager.h
  startMQTT();  // from mqtt_manager.h

  // Publish initial "online" status
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
    targetFanSpeed = 0;
    checkSwSerial(&hrvSerial); // send fan speed to controller and receive roof temp

    // Debug
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

    // *** Only do the out-of-range check if we have at least 3 valid readings ***
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

        //reset softwareserial
        hrvSerial.read(); // clear the buffer
        hrvSerial.end(); // end the serial
        //RESET ALL VARIABLES
        currentRoofTemperature = 0.0f; // reset the current temperature
        lastRoofTemperatures[0] = 0.0f; // reset the last temperature
        lastRoofTemperatures[1] = 0.0f; // reset the last temperature
        lastRoofTemperatures[2] = 0.0f; // reset the last temperature
        firstBoot = true; // reset the first boot flag
        firstValidReading = false; // reset the first valid reading flag
        validReadingsCount = 0;   // reset the valid readings count

        // Restart the softwareserial
        hrvSerial.begin(1200, SWSERIAL_8N1, D6, D6, false, 256); // restart the serial
        hrvSerial.enableIntTx(false); // disable tx interrupt

        //read the target fan speed
        client.subscribe(MQTT_TARGET_FAN_SPEED);
        //set the fan speed to the last known value
        targetFanSpeed = lastTargetFanSpeed;

      } 
      else {
        // If we are within range, publish the current temperature
        String mqttPublishHRVTemperatureStr = String(currentRoofTemperature);
        mqttPublishHRVTemperatureStr.toCharArray(HRVTemperature_buff, sizeof(HRVTemperature_buff));
        client.publish(MQTT_ROOF_TEMP, HRVTemperature_buff);
      }
    }
  }

  // 2) Handle MQTT publishing separately
  if (currentMillis - previousMQTTMillis >= MQTT_PUBLISH_INTERVAL) {
    previousMQTTMillis = currentMillis;

    // Convert MQTT target speed to byte
    byte mqttTargetFanSpeedByte = mqttTargetFanSpeed.toInt();
    // Convert to hex
    String hexValue = convertBase(mqttTargetFanSpeedByte, 10, 16);
    long intValue = strtol(hexValue.c_str(), NULL, 16);
    targetFanSpeed = (byte)intValue;

    // Publish current fan speed to MQTT
    String mqttPublishFanSpeed = String(targetFanSpeed);
    mqttPublishFanSpeed.toCharArray(FanSpeed_buff, sizeof(FanSpeed_buff));
    if (debug_console_mqtt_targetFanSpeed) {
      Serial.print(F("Fan Speed: "));
      Serial.println(mqttPublishFanSpeed);
    }
    client.publish(MQTT_FAN_SPEED, FanSpeed_buff);
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

String convertBase(int num, int fromBase, int toBase) {
  return String(num, toBase);
}
