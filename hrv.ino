#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string>
#include "secrets.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "serial_manager.h"
#include "utils.h"
#include "constants.h"

#define D6 (12)
const byte MSGSTARTSTOP = 0x7E;
#define localPort 57701

SoftwareSerial hrvSerial;

// MQTT Broker
const char* topic = "hrv/status";
const int mqtt_port = 1883;

// MQTT subs
const char* MQTT_ROOF_TEMP = "hrv/rooftemp";
const char* MQTT_FAN_SPEED = "hrv/currentfanspeed";
const char* MQTT_TARGET_FAN_SPEED = "hrv/targetfanspeed";

// The MAC address of the Arduino
// 34:94:54:61:EE:70
byte mac[] = { 0x34, 0x94, 0x54, 0x61, 0xEE, 0x70 };

// Wifi Client
WiFiClient wifiClient;
PubSubClient client(wifiClient);
IPAddress ipadd;
char packetBuffer[255];

// Temperature from Roof or House
char tempLocation;

// TTL hrvSerial data array, dataIndex, dataIndex of checksum and temperature
byte serialData[10];
byte dataIndex;
byte checksumIndex;
byte targetFanSpeed;
bool dataStarted = false;
bool dataReceived = false;
float currentRoofTemperature = 0;
float lastRoofTemperature = 0;

// Define message buffer and publish string
char HRVTemperature_buff[16];
char FanSpeed_buff[16];
int iTotalDelay = 0;
String mqttPublishHRVTemperature;
String mqttTargetFanSpeed;
String txMessage;

// Non-blocking delay variables
// unsigned long previousMillis = 0;
// const long interval = 1500;

//bug
int tempCheckCounter = 0;

//debug "feature" flags
bool debug_console_enable = true;
bool debug_console_hrvController_currentRoofTemperature = false;
bool debug_console_mqtt_targetFanSpeed = false;
bool debug_console_mqtt_brokerConnection = false;
bool debug_console_mqtt_msgFanSpeedTopic = false;
bool debug_console_serial_txMessage = false;
bool debug_console_serial_rxDataAvailable = false;
bool debug_console_serial_rxReadData = false;
bool debug_console_serial_rxStartingData = false;
bool debug_console_wifi_connection = true;

void setup() {
  hrvSerial.begin(1200, SWSERIAL_8N1, D6, D6, false, 256);
  hrvSerial.enableIntTx(false);
  // Debug USB Serial
  if (debug_console_enable == true) {
  Serial.begin(115200);
  }

  // Initialize defaults
  dataIndex = 0;
  checksumIndex = 0;
  iTotalDelay = 0;
  targetFanSpeed = 0x00;
  mqttTargetFanSpeed = 0;

  startWIFI();
  startMQTT();
}

void loop() {
  // put your main code here, to run repeatedly:
  // unsigned long currentMillis = millis();

  // check for incoming messages
  if (client.state() == MQTT_CONNECTED) {
    client.loop();
  }

  // Non-blocking delay
  // if (currentMillis - previousMillis >= interval) {
  //   previousMillis = currentMillis;

    checkSwSerial(&hrvSerial); //send fan speed to fan controller and receive back roof temperature
    delay(5000);

    if (debug_console_hrvController_currentRoofTemperature = true) {
      Serial.print("Current Roof Temperature: ");
      Serial.println(String(currentRoofTemperature));
    }
    if (currentRoofTemperature != lastRoofTemperature || currentRoofTemperature == 0) {
      tempCheckCounter++;
      if (debug_console_hrvController_currentRoofTemperature == true) {
        Serial.println("Roof Temperature is not the same as last time");
        Serial.print("increased tempCheckCounter to: ");
        Serial.println(String(tempCheckCounter));
      }

      if (currentRoofTemperature > 0 || currentRoofTemperature < 0) {
        if (currentRoofTemperature < 60) {
          lastRoofTemperature = currentRoofTemperature;
          mqttPublishHRVTemperature = String(currentRoofTemperature);
          mqttPublishHRVTemperature.toCharArray(HRVTemperature_buff, mqttPublishHRVTemperature.length()+1);
          client.publish(MQTT_ROOF_TEMP, HRVTemperature_buff);
        }
      }
    } else {
    tempCheckCounter = 0; // Reset counter if temperature has changed
  }

  if (tempCheckCounter >= 2) {
    if (debug_console_hrvController_currentRoofTemperature == true) {
      Serial.println("Roof temperature has reported incorrectly twice, restarting serial port for HRV controller");
    }
    hrvSerial.end(); // stop software serial
    delay(1000); // wait for 1 second
    hrvSerial.begin(1200, SWSERIAL_8N1, D6, D6, false, 256); // start software serial
    delay(5000);
    tempCheckCounter = 0;
  }

    byte mqttTargetFanSpeedByte = mqttTargetFanSpeed.toInt();
    String hexValue = convertBase(mqttTargetFanSpeedByte, 10, 16);

    const char* hexStr = hexValue.c_str();
    long int intValue = strtol(hexStr, NULL, 16);
    targetFanSpeed = (byte) intValue;

    String mqttPublishFanSpeed;
    mqttPublishFanSpeed = String(targetFanSpeed);
    mqttPublishFanSpeed.toCharArray(FanSpeed_buff, mqttPublishFanSpeed.length()+1);

    if (debug_console_mqtt_targetFanSpeed == true) {
      Serial.print("Fan Speed: ");
      Serial.println(mqttPublishFanSpeed);
    }
    client.publish(MQTT_FAN_SPEED, FanSpeed_buff);
  // }
}

// Convert a number from one base to another
String convertBase(int num, int fromBase, int toBase) {
String result = String(num, toBase);
return result;
}
