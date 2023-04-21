#include "mqtt_manager.h"
#include "constants.h"

void startMQTT()
{
  // Initialize the client and set the callback function
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  
  // Connect to the MQTT broker
  String client_id = "hrv-client-";
  client_id += String(WiFi.macAddress());
  if (debug_console_mqtt_brokerConnection == true) {
    Serial.print("Initiating connection to MQTT broker using client ID: ");
    Serial.println(client_id);
  }
  
  while (!client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
    if (debug_console_mqtt_brokerConnection == true) {
      Serial.print("Unable to connect to MQTT broker, current state: ");
      Serial.println(client.state());
    }
    delay(2000);
  }
  
  // Publish and subscribe
  client.publish(topic, "on");
  client.subscribe(MQTT_TARGET_FAN_SPEED);
  delay(500);
}

void callback(char *topic, byte *payload, unsigned int length) {
  if (debug_console_mqtt_msgFanSpeedTopic == true) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
  }

  // Clear the target fan speed variable
  mqttTargetFanSpeed = "";
  
  // Loop through the payload and print the message
  for (int i = 0; i < length; i++) {
    if (debug_console_mqtt_msgFanSpeedTopic == true) {
      Serial.print((char) payload[i]);
    }
    mqttTargetFanSpeed += (char) payload[i];
  }
}