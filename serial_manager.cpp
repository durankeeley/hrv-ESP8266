#include "serial_manager.h"
#include "utils.h"
#include "constants.h"
#include "mock_data.h"

// variable declarations
extern bool debug_console_serial_txMessage;
extern bool debug_console_serial_rxDataAvailable;
extern bool debug_console_serial_rxReadData;
extern bool debug_console_serial_rxStartingData;
extern float currentRoofTemperature;
extern byte targetFanSpeed;
extern bool dataStarted;
extern bool dataReceived;
extern byte serialData[10];
extern byte dataIndex;
extern byte checksumIndex;
extern const byte MSGSTARTSTOP;
byte calculateChecksum(byte* data, size_t length);

void dumpMessage(const byte* message, size_t length) {
  // Each byte will occupy up to 3 characters: 2 hex digits + a space.
  // Add some space for a prefix like "Message: " and the terminator.
  char buffer[3 * length + 16];
  int offset = 0;

  // Start the string
  offset += sprintf(buffer + offset, "Message: ");

  // Append each byte in hex
  for (size_t i = 0; i < length; i++) {
    // %02X prints two hex digits (zero-padded if <0x10), uppercase
    offset += sprintf(buffer + offset, "%02X ", message[i]);
  }

  // Print it all in one go
  Serial.printf("%s\n", buffer);
}

void checkSwSerial(SoftwareSerial* ss) {

  if (debug_mockRoofTemp) {
    injectMockData();
    return;
  }


  // Send the packet to the HRV Roof 
  byte messageData[] = {0x31, 0x01, 0x9E, targetFanSpeed, 0x0E, 0x80, 0x70};

  // Calculate the checksum
  byte checksum = calculateChecksum(messageData, sizeof(messageData));

  // Positions:
  //   [0] => MSGSTARTSTOP (0x7E) - Start of frame
  //   [1] => ID: 0x31 = Control Panel
  //   [2] => Control Panel Temp: first part
  //   [3] => Control Panel Temp: second part
  //   [4] => Target Fan Speed
  //   [5] => Control Panel Set Temp (degrees C)
  //   [6] => Unknown: Control Panel Mode first part ?
  //   [7] => Unknown: Control Panel Fan Mode second part ?
  //   [8] => Checksum
  //   [9] => MSGSTARTSTOP (0x7E) - End of frame

  // Construct the full message
  byte message[] = {MSGSTARTSTOP, 0x31, 0x01, 0x9E, targetFanSpeed, 0x0E, 0x80, 0x70, checksum, MSGSTARTSTOP};
  dumpMessage(message, sizeof(message));

  ss->enableTx(true);
  ss->write(message, sizeof(message));
  if (debug_console_serial_txMessage) {
    Serial.print(F("TX: "));
    Serial.write(message, sizeof(message));
    Serial.println();
  }
  ss->enableTx(false);
  delay(10);

  // Read the response
  while (ss->available()) {
    byte ch = (byte)ss->read();
    if (debug_console_serial_rxDataAvailable) {
      Serial.println(F("Data available on serial port!"));
    }

    if (debug_console_serial_rxReadData) {
      Serial.print(F("Byte: "));
      Serial.println(ch, HEX);
    }

    // Detect start of frame or end
    if (ch == MSGSTARTSTOP || dataIndex > 8) {
      if (dataIndex == 0) {
        // We are starting to read a data frame
        dataStarted = true;
        if (debug_console_serial_rxStartingData) {
          Serial.println(F("Started Data Block"));
        }
      } else {
        // We reached the end
        checksumIndex = dataIndex - 1;
        dataReceived  = true;
        parseReceivedData();
        // Reset for next frame
        dataIndex = 0;
        dataStarted = false;
        dataReceived = false;
      }
    } else {
      // If we are in the frame, collect data
      if (dataStarted && (dataIndex < sizeof(serialData))) {
        serialData[dataIndex] = ch;
        dataIndex++;
      }
    }
    yield();
  }
}

void parseReceivedData() {
  if (dataIndex < 4) {
    return;
  }

  // Positions:
  //   [0] => tempLocation
  //   [1] => first part of temp
  //   [2] => second part of temp
  //   [3] => checksum

  tempLocation = (char)serialData[0];
  String firstHexPart = decToHex(serialData[1], 2);
  String secondHexPart = decToHex(serialData[2], 2);

  float temperature = hexToDec(firstHexPart + secondHexPart);
  temperature *= 0.0625f;
  temperature = (int)(temperature * 2 + 0.5) / 2.0f;

  currentRoofTemperature = temperature;
}

byte calculateChecksum(byte* data, size_t length) {
  int checksum = 0;
  for (size_t i = 0; i < length; i++) {
    checksum -= data[i];
  }
  return (byte)(checksum % 0x100);
}
