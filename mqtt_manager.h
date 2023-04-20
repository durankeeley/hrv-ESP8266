#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <ESP8266WiFi.h>

void startMQTT();
void callback(char *topic, byte *payload, unsigned int length);

#endif
