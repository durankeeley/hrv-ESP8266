#include "mqtt_manager.h"
#include "constants.h"
#include "secrets.h"

extern PubSubClient client;
extern bool debug_console_mqtt_brokerConnection;
extern bool debug_console_mqtt_msgFanSpeedTopic;
extern String mqttTargetFanSpeed;


void startMQTT()
{
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  String client_id = "hrv-client-";
  client_id += String(WiFi.macAddress());

  if (debug_console_mqtt_brokerConnection) {
    Serial.print(F("Initiating connection to MQTT broker using client ID: "));
    Serial.println(client_id);
  }

  client.connect(client_id.c_str(), mqtt_username, mqtt_password);
  if (client.connected()) {
    client.subscribe(MQTT_TARGET_FAN_SPEED);
  } else {
    if (debug_console_mqtt_brokerConnection) {
      Serial.print(F("Unable to connect to MQTT broker, state: "));
      Serial.println(client.state());
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  if (debug_console_mqtt_msgFanSpeedTopic) {
    Serial.print(F("Message arrived in topic: "));
    Serial.println(topic);
    Serial.print(F("Message: "));
  }

  mqttTargetFanSpeed = "";

  for (unsigned int i = 0; i < length; i++) {
    if (debug_console_mqtt_msgFanSpeedTopic) {
      Serial.print((char)payload[i]);
    }
    mqttTargetFanSpeed += (char)payload[i];
  }
  if (debug_console_mqtt_msgFanSpeedTopic) {
    Serial.println();
  }
}
