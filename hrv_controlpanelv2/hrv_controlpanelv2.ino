#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

// WiFi and MQTT settings
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const char* mqtt_user = "";
const char* mqtt_pass = "";
const char* mqtt_topic = "hrv/rooftemp";

// Pin definitions
const int rxPin = D6;
const int txPin = D6;

// Create a SoftwareSerial instance
SoftwareSerial mySerial(rxPin, txPin);

// State variables
bool msgStarted = false;
bool lastByteWasStart = false;
int hexCount = 0;
byte hexBytes[5];
uint16_t roofTemp;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Start the hardware serial
  Serial.begin(115200);
  Serial.println("Starting SoftwareSerial");

  // Start the software serial
  mySerial.begin(1200);

  // Set roofTemp value by combining 0x01 and 0x6D
  //int inputValue = 55; // Replace this with the desired integer value
  //roofTemp = (uint16_t)(inputValue / 0.0625);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Setup MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

}

String decToHex(byte decValue, byte desiredStringLength) {
  String hexString = String(decValue, HEX);
  while (hexString.length() < desiredStringLength) hexString = "0" + hexString;

  return hexString;
}

void sendData(byte message[], size_t messageSize) {
  byte MSGSTARTSTOP = 0x7E;

  int iChar = 0;
  int iLess;
  byte bCalc;
  String sCalc;

  for (int iPos = 0; iPos < messageSize; iPos++) {
    iLess = message[iPos];
    iChar = iChar - iLess;
  }

  bCalc = (byte)(iChar % 0x100);
  sCalc = decToHex(bCalc, 2);

  mySerial.enableTx(true);
  int i;
  mySerial.write(MSGSTARTSTOP);
  for (i = 0; i < messageSize; i++) {
    mySerial.write(message[i]);
  }
  mySerial.write(bCalc);
  mySerial.write(MSGSTARTSTOP);

  mySerial.enableTx(false);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;

  for (unsigned int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }

  int inputValue = messageTemp.toInt();
  roofTemp = (uint16_t)(inputValue / 0.0625);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
    if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // Check if there is data available on the software serial
  if (mySerial.available()) {
    // Read the incoming byte
    char incomingByte = mySerial.read();

    if (incomingByte == 0x7E) {
      if (msgStarted) {
        if (!lastByteWasStart) {
          // End of message
          msgStarted = false;
          int combinedValue = (hexBytes[1] << 8) + hexBytes[2];
          float calculatedValue = combinedValue * 0.0625;
          Serial.print("Control Panel Temp: ");
          Serial.println(calculatedValue);
          Serial.print("Fan Control: ");
          Serial.println(hexBytes[4]);

          // Call sendData() function after processing the incoming data
          byte message1[] = {0x30, (byte)(roofTemp >> 8), (byte)(roofTemp & 0xFF), 0x0, 0x0E, 0x80, 0x70};
          sendData(message1, sizeof(message1));

          //byte message2[] = {0x31, 0x0, 0x0, 0x12, 0x07, 0x80, 0x70};
          //sendData(message2, sizeof(message2));

        }
      } else {
        // Start of message
        msgStarted = true;
        hexCount = 0;
      }
      lastByteWasStart = true;
    } else {
      if (msgStarted) {
        if (hexCount < 5) {
          hexBytes[hexCount] = incomingByte;
          hexCount++;
        }
      }
      lastByteWasStart = false;
    }
  }
}
