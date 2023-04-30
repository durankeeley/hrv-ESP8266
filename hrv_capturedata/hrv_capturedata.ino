#include <SoftwareSerial.h>

// Pin definitions
const int rxPin = D6;
const int txPin = D6;

// Create a SoftwareSerial instance
SoftwareSerial mySerial(rxPin, txPin);

// State variables
bool msgStarted = false;
bool lastByteWasStart = false;

void setup() {
  // Start the hardware serial
  Serial.begin(115200);
  Serial.println("Starting SoftwareSerial");

  // Start the software serial
  mySerial.begin(1200);
}

void loop() {
  // Check if there is data available on the software serial
  if (mySerial.available()) {
    // Read the incoming byte
    char incomingByte = mySerial.read();

    if (incomingByte == 0x7E) {
      if (msgStarted) {
        if (!lastByteWasStart) {
          // End of message
          msgStarted = false;
          Serial.println("\n");
        }
      } else {
        // Start of message
        msgStarted = true;
        //Serial.println("Start of message");
      }
      lastByteWasStart = true;
    } else {
      if (msgStarted) {
        // Print the incoming byte to the hardware serial
        Serial.print(" ");
        Serial.print(incomingByte, HEX);
        Serial.print(" ");
      }
      lastByteWasStart = false;
    }
  }
}
