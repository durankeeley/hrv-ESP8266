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

#endif