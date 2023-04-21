// constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Wifi Client

extern WiFiClient wifiClient;
extern PubSubClient client;
extern IPAddress ipadd;
extern char packetBuffer[255];

// MQTT
extern const int mqtt_port;
extern const char* mqtt_broker;
extern const int mqtt_port;
extern const char* mqtt_username;
extern const char* mqtt_password;
extern PubSubClient client;
extern bool debug_console_mqtt_brokerConnection;

// Temperature from Roof or House
extern char tempLocation;

// TTL hrvSerial data array, dataIndex, dataIndex of checksum and temperature
extern const byte MSGSTARTSTOP;
extern byte serialData[10];
extern byte dataIndex;
extern byte checksumIndex;
extern byte targetFanSpeed;
extern bool dataStarted;
extern bool dataReceived;
extern float currentRoofTemperature;
extern String txMessage;

// Define message buffer and publish string
extern char HRVTemperature_buff[16];
extern char FanSpeed_buff[16];
extern int iTotalDelay;
extern String mqttPublishHRVTemperature;
extern String mqttTargetFanSpeed;
extern const char* topic;
extern const char* MQTT_TARGET_FAN_SPEED;
extern const char* MQTT_ROOF_TEMP;
extern const char* MQTT_FAN_SPEED;

// Debug Flags
extern bool debug_console_mqtt_brokerConnection;
extern bool debug_console_mqtt_msgFanSpeedTopic;
extern bool debug_console_serial_txMessage;
extern bool debug_console_serial_rxDataAvailable;
extern bool debug_console_serial_rxReadData;
extern bool debug_console_serial_rxStartingData;
extern bool debug_console_wifi_connection;

#endif