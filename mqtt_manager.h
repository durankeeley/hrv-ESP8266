#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#define MQTT_TARGET_FAN_SPEED "hrv/targetfanspeed"

void startMQTT();
void callback(char *topic, byte *payload, unsigned int length);
const char *topic = "hrv/status";
String mqttTargetFanSpeed;
const int mqtt_port = 1883;


#endif
