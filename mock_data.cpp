#include <Arduino.h>
#include "mock_data.h"
#include "constants.h"

// External variables used in injectMockData
extern float mockRoofTempValue;
extern bool debug_console_serial_rxDataAvailable;
extern bool debug_console_serial_rxReadData;
extern bool debug_console_serial_rxStartingData;
// extern byte MSGSTARTSTOP;
extern byte serialData[10];
// extern int dataIndex;
extern bool dataStarted;
extern bool dataReceived;
// extern int checksumIndex;

// Forward declaration of parseReceivedData
void parseReceivedData();

void injectMockData() {
  // Convert mockRoofTempValue to raw data
  float val = mockRoofTempValue;  // e.g., 32.0
  int raw = (int)(val / 0.0625f); // 512
  byte high = (raw >> 8) & 0xFF;  // 0x02
  byte low = raw & 0xFF;          // 0x00

  // Define tempLocation (e.g., 'R' = 0x31)
  byte tempLocationByte = 0x31;

  // Compute checksum
  int iChar = 0;
  iChar -= tempLocationByte;
  iChar -= high;
  iChar -= low;
  byte checksum = (byte)(iChar % 0x100); // 0xCD

  // Construct the mock message with checksum
  byte mockMessage[] = {0x7E, tempLocationByte, high, low, checksum, 0x7E};

  for (unsigned int i = 0; i < sizeof(mockMessage); i++) {
    byte ch = mockMessage[i];

    if (debug_console_serial_rxDataAvailable) {
      Serial.println(F("Data available on serial port (MOCK)!"));
    }
    if (debug_console_serial_rxReadData) {
      Serial.print(F("Mock Byte: 0x"));
      Serial.println(ch, HEX);
    }

    if (ch == MSGSTARTSTOP || dataIndex > 8) {
      if (dataIndex == 0) {
        dataStarted = true;
        if (debug_console_serial_rxStartingData) {
          Serial.println(F("Started Data Block (MOCK)"));
        }
      } else {
        // End of frame
        checksumIndex = dataIndex - 1;
        dataReceived = true;
        parseReceivedData();
        // Reset for next frame
        dataIndex = 0;
        dataStarted = false;
        dataReceived = false;
      }
    } else {
      if (dataStarted && (dataIndex < sizeof(serialData))) {
        serialData[dataIndex] = ch;
        dataIndex++;
      }
    }
    yield();
  }
}