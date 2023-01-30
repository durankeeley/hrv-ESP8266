#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string>

#define D6 (12)
#define MSGSTARTSTOP 0x7E
#define localPort 57701

SoftwareSerial hrvSerial;

// Wifi
const char* ssid     = "";
const char* password = "";

// MQTT Broker
const char *mqtt_broker = "";
const char *topic = "hrv/status";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;

// MQTT subs
#define MQTT_ROOF_TEMP "hrv/rooftemp"
#define MQTT_FAN_SPEED "hrv/currentfanspeed"
#define MQTT_TARGET_FAN_SPEED "hrv/targetfanspeed"

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

// Define message buffer and publish string
char message_buff[16];
int iTotalDelay = 0;
String mqttPublishHRVTemperature;
String mqttTargetFanSpeed;

void setup() {
  hrvSerial.begin(1200, SWSERIAL_8N1, D6, D6, false, 256);
  hrvSerial.enableIntTx(false);
  // Debug USB Serial
  Serial.begin(115200);

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

  delay(1500);

  // check for incoming messages
  if (client.state() == MQTT_CONNECTED) {
    client.loop();
    delay(1500);
  }

    Serial.print("Target Fan Speed: ");
    Serial.println(mqttTargetFanSpeed);
    byte mqttTargetFanSpeedByte = mqttTargetFanSpeed.toInt();
    String hexValue = decToHex(mqttTargetFanSpeedByte, 2);

    const char* hexStr = hexValue.c_str();
    long int intValue = strtol(hexStr, NULL, 16);
    targetFanSpeed = (byte) intValue;

  checkSwSerial(&hrvSerial); //send fan speed to fan controller and receive back roof temperature
  delay(1500);
  
  Serial.print("CeilingTemp: ");
  Serial.println(mqttPublishHRVTemperature);
  client.publish(MQTT_ROOF_TEMP, message_buff);

  String mqttPublishFanSpeed;
  mqttPublishFanSpeed = String(targetFanSpeed);
  mqttPublishFanSpeed.toCharArray(message_buff, mqttPublishFanSpeed.length()+1);

  Serial.print("Fan Speed: ");
  Serial.println(mqttPublishFanSpeed);
  client.publish(MQTT_FAN_SPEED, message_buff);
}

void checkSwSerial(SoftwareSerial* ss) {
  byte ch;
  byte message[] = {0x31,0x01,0x9E,targetFanSpeed,0x0E,0x80,0x70};

    // Subtract from zero
  int iChar = 0;
  int iLess;
  byte bCalc;
  String sCalc;
      
  // Subtract each byte in ID and data
  for (int iPos=0; iPos < sizeof(message); iPos++)
  {
    iLess = message[iPos];
    iChar = iChar - iLess;
  }

  // Convert calculations
  bCalc = (byte) (iChar % 0x100);
  sCalc = decToHex(bCalc, 2);

  ss->enableTx(true); // tell the arduino to switch the pin into TX mode so we can send and receive on the same wire.

  {
    int i;
    ss->write(MSGSTARTSTOP); //send start message first
    for(i=0; i<sizeof(message); i++){
          ss->write(message[i]);
          // Debug
          //Serial.print(message[i],HEX); //send the stuff its expecting including our new fan speed
    }
    ss->write(bCalc);//send checksum
    ss->write(MSGSTARTSTOP); //send stop bit
  }

  ss->enableTx(false); //this is the magic, tell the board to start listening on the same pin we just wrote to
  delay(50);
  
  if (ss->available()) {
    while (ss->available()) {
      ch = (byte)ss->read();
      int inChar = int(ch);

      if (inChar == MSGSTARTSTOP || dataIndex > 8)
      {
         // Start if first time we've got the message
         if (dataIndex == 0)
         {
             Serial.println("Started Block");
             Serial.println("");
             dataStarted = true; 
         }
         else
         {
             checksumIndex = dataIndex-1;
             dataReceived = true;
             break;
         }
      }  
      
      if (dataStarted == true)
      {
        // Double check we actually got something
        if (sizeof(inChar) > 0)
        {
          // DEBUG
          // Serial.print("DEBUG - HEX: ");
          // Serial.println(inChar, HEX);
          serialData[dataIndex] = inChar;
          dataIndex++;
        }        
      }
      myDelay(1);
    }
    Serial.println();
  }

  String firstHexPart;
  String secondHexPart;
  int iPos;
    
  // Pull data out of the array, position 0 is 0x7E (start and end of message)
  for (int iPos=1; iPos <= dataIndex; iPos++)
  {
    
    // Position 1 defines house or roof temperature
    if (iPos==1) { tempLocation = (char) serialData[iPos]; }

    // Position 2 and 3 are actual temperature, convert to hex
    if (iPos == 2) { firstHexPart = decToHex(serialData[iPos], 2); }
    if (iPos == 3) { secondHexPart = decToHex(serialData[iPos], 2); }

  }

  //Concatenate first and second hex, convert back to decimal, 1/16th of dec + rounding
  //Rounding is weird - it varies between roof and house, MQTT sub rounds to nearest 0.5
  float currentRoofTemperature = 0;
  currentRoofTemperature = hexToDec(firstHexPart + secondHexPart);
  
  currentRoofTemperature = (currentRoofTemperature * 0.0625);
  currentRoofTemperature = (int)(currentRoofTemperature * 2 + 0.5) / 2.0f;
  mqttPublishHRVTemperature = String(currentRoofTemperature);
  mqttPublishHRVTemperature.toCharArray(message_buff, mqttPublishHRVTemperature.length()+1);

}

// Convert from decimal to hex
String decToHex(byte decValue, byte desiredStringLength) 
{
  String hexString = String(decValue, HEX);
  while (hexString.length() < desiredStringLength) hexString = "0" + hexString;
  return hexString;
}

// Convert from hex to decimal
unsigned int hexToDec(String hexString) 
{
  unsigned int decValue = 0;
  int nextInt;
  for (int i = 0; i < hexString.length(); i++) {
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    decValue = (decValue * 16) + nextInt;
  }
  return decValue;
}

// This function yields back to the watchdog to avoid random ESP8266 resets
void myDelay(int ms)  
{
  int i;
  for(i=1; i!=ms; i++) 
  {
    delay(1);
    if(i%100 == 0) 
   {
      ESP.wdtFeed(); 
      yield();
    }
  }
  iTotalDelay+=ms;
}

// Starts WIFI connection
void startWIFI() {
    // If we are not connected
    if (WiFi.status() != WL_CONNECTED) {
        int connectResult;
        Serial.println("Initiating WiFi connection process.");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(); 
        WiFi.begin(ssid, password);

        // Wait for connection result
        connectResult = WiFi.waitForConnectResult();

        // If not WiFi connected, retry every 2 seconds for 15 minutes
        while (connectResult != WL_CONNECTED) {
            Serial.print(".");
            delay(2000);
            connectResult = WiFi.waitForConnectResult();
            // If can't get to Wifi for 15 minutes, reboot ESP
            if (iTotalDelay > 900000)
            {
                Serial.println("Excessive WiFi connection attempts detected, initiating reboot.");
                ESP.reset();
            }
            iTotalDelay+=2000;
        }

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println(WiFi.localIP());
        // Let network have a chance to start up
        myDelay(1500);
    }
}

void startMQTT()
{
  // Initialize the client and set the callback function
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  
  // Connect to the MQTT broker
  String client_id = "hrv-client-";
  client_id += String(WiFi.macAddress());
  // DEBUG
  //Serial.print("Initiating connection to MQTT broker using client ID: ");
  //Serial.println(client_id);
  
  while (!client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.print("Unable to connect to MQTT broker, current state: ");
      Serial.println(client.state());
      delay(2000);
  }
  
  // Publish and subscribe
  client.publish(topic, "on");
  client.subscribe(MQTT_TARGET_FAN_SPEED);
  delay(500);
}

void callback(char *topic, byte *payload, unsigned int length) {
  // DEBUG
  // Print the topic and message
  //Serial.print("Message arrived in topic: ");
  //Serial.println(topic);
  //Serial.print("Message:");
  
  // Clear the target fan speed variable
  mqttTargetFanSpeed = "";
  
  // Loop through the payload and print the message
  for (int i = 0; i < length; i++) {
    // DEBUG
    //Serial.print((char) payload[i]);
    mqttTargetFanSpeed += (char) payload[i];
  }
}
