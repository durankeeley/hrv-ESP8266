#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ---------------------------------------------------------------------------
// External variable declarations (no definitions)
// ---------------------------------------------------------------------------

// WiFi / MQTT objects
extern WiFiClient      wifiClient;
extern PubSubClient    client;
extern IPAddress       ipadd;
extern char            packetBuffer[255];
extern int             iTotalDelay;

// Basic MQTT info (port, topics, etc.)
extern int             mqtt_port;           // e.g. 1883
extern const char*     topic;               // e.g. "hrv/status"
extern const char*     MQTT_TARGET_FAN_SPEED;
extern const char*     MQTT_ROOF_TEMP;
extern const char*     MQTT_FAN_SPEED;

// Debug flags
extern bool debug_console_enable;
extern bool debug_console_hrvController_currentRoofTemperature;
extern bool debug_console_mqtt_targetFanSpeed;
extern bool debug_console_mqtt_brokerConnection;
extern bool debug_console_mqtt_msgFanSpeedTopic;
extern bool debug_console_serial_txMessage;
extern bool debug_console_serial_rxDataAvailable;
extern bool debug_console_serial_rxReadData;
extern bool debug_console_serial_rxStartingData;
extern bool debug_console_wifi_connection;

// Serial / temperature data
extern byte  serialData[10];
extern byte  dataIndex;
extern byte  checksumIndex;
extern bool  dataStarted;
extern bool  dataReceived;
extern float currentRoofTemperature;
extern float lastRoofTemperatures[3];
extern byte  targetFanSpeed;
extern byte  lastTargetFanSpeed;
extern char  tempLocation;

// Buffers
extern char  HRVTemperature_buff[16];
extern char  FanSpeed_buff[16];

// Strings
extern String txMessage;
extern String mqttTargetFanSpeed;
extern String mqttPublishHRVTemperature;

// Mock
extern bool debug_mockRoofTemp;
extern float mockRoofTempValue;

// Start and stop marker
extern const byte MSGSTARTSTOP;
#endif
