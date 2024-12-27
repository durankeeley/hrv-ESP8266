#include "serial_manager.h"
#include "utils.h"
#include "constants.h"

extern bool debug_console_serial_txMessage;
extern bool debug_console_serial_rxDataAvailable;
extern bool debug_console_serial_rxReadData;
extern bool debug_console_serial_rxStartingData;

// Shared from .ino or a common .h file
extern float currentRoofTemperature;
extern byte targetFanSpeed;
extern bool dataStarted;
extern bool dataReceived;
extern byte serialData[10];
extern byte dataIndex;
extern byte checksumIndex;
extern const byte MSGSTARTSTOP;

static void injectMockData()
{
  // Convert mockRoofTempValue to raw data
  float val = mockRoofTempValue; // e.g., 32.0
  int raw = (int)(val / 0.0625f); // 512
  byte high = (raw >> 8) & 0xFF; // 0x02
  byte low = raw & 0xFF;         // 0x00

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
      }
      else {
        // End of frame
        checksumIndex = dataIndex - 1;
        dataReceived = true;
        parseReceivedData();
        // Reset for next frame
        dataIndex = 0;
        dataStarted = false;
        dataReceived = false;
      }
    }
    else {
      if (dataStarted && (dataIndex < sizeof(serialData))) {
        serialData[dataIndex] = ch;
        dataIndex++;
      }
    }
    yield();
  }
}

void checkSwSerial(SoftwareSerial* ss) {

  if (debug_mockRoofTemp) {
    injectMockData();
    return;
  }

  byte message[] = {0x31, 0x01, 0x9E, targetFanSpeed, 0x0E, 0x80, 0x70};

  int iChar = 0;
  for (unsigned int iPos = 0; iPos < sizeof(message); iPos++) {
    iChar -= message[iPos];
  }
  byte bCalc = (byte)(iChar % 0x100);

  // Send the packet
  ss->enableTx(true);
  {
    if (debug_console_serial_txMessage) {
      Serial.print(F("TX: "));
      Serial.write(MSGSTARTSTOP);
    }
    ss->write(MSGSTARTSTOP);
    for (unsigned int i = 0; i < sizeof(message); i++) {
      ss->write(message[i]);
      if (debug_console_serial_txMessage) {
        Serial.write(message[i]);
      }
    }
    ss->write(bCalc);
    if (debug_console_serial_txMessage) {
      Serial.write(bCalc);
    }
    ss->write(MSGSTARTSTOP);
    if (debug_console_serial_txMessage) {
      Serial.write(MSGSTARTSTOP);
      Serial.println(); // newline
    }
  }
  ss->enableTx(false);
  delay(50);

  // 2) Read response (non-blocking partial read)
  //    Typically you’d do this in small chunks, but here we read what’s available now:
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
