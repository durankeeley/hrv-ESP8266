#include "serial_manager.h"
#include "utils.h"
#include "constants.h"

// Sends fan speed to fan controller and receives back roof temperature
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
    if (debug_console_serial_txMessage == true) {
      txMessage = "";
      txMessage += MSGSTARTSTOP;
    }
    ss->write(MSGSTARTSTOP); //send start message first

    for(i=0; i<sizeof(message); i++){
          ss->write(message[i]);
          if (debug_console_serial_txMessage == true) {
            txMessage += (char) message[i];
          }
    }
    ss->write(bCalc);//send checksum
    if (debug_console_serial_txMessage == true) {
      txMessage += bCalc;
    }
    ss->write(MSGSTARTSTOP); //send stop bit
    if (debug_console_serial_txMessage == true) {
      txMessage += MSGSTARTSTOP;
      Serial.print(txMessage);
    }
  }

  ss->enableTx(false); //this is the magic, tell the board to start listening on the same pin we just wrote to
  delay(50);
  
  if (ss->available()) {
    if (debug_console_serial_rxDataAvailable == true) {
      Serial.println("Data available on serial port!");
    }
    while (ss->available()) {
      ch = (byte)ss->read();
      int inChar = int(ch);

      if (debug_console_serial_rxReadData == true) {
        Serial.print("ASCII value: ");
        Serial.println(inChar);
        Serial.println("");
        Serial.print("Character: ");
        Serial.println((char)ch);
      }

      if (inChar == MSGSTARTSTOP || dataIndex > 8)
      {
         // Start if first time we've got the message
         if (dataIndex == 0)
         {
            
            dataStarted = true;
            if (debug_console_serial_rxStartingData == true) {
              Serial.println("Started Data Block");
            }
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
          serialData[dataIndex] = inChar;
          dataIndex++;
        }        
      }
      myDelay(1);
    }
  }

  String firstHexPart;
  String secondHexPart;
  int iPos;
    
  // Pull data out of the array, position 0 is 0x7E (start and end of message)
  for (int iPos=1; iPos <= dataIndex; iPos++)
  {
    // Position 1 defines house or roof temperature
    if (iPos == 1) { tempLocation = (char) serialData[iPos]; }

    // Position 2 and 3 are actual temperature, convert to hex
    if (iPos == 2) { firstHexPart = decToHex(serialData[iPos], 2); }
    if (iPos == 3) { secondHexPart = decToHex(serialData[iPos], 2); }
  }

  //Concatenate first and second hex, convert back to decimal, 1/16th of dec + rounding
  //Rounding is weird - it varies between roof and house, MQTT sub rounds to nearest 0.5
  currentRoofTemperature = hexToDec(firstHexPart + secondHexPart);
  currentRoofTemperature = (currentRoofTemperature * 0.0625);
  currentRoofTemperature = (int)(currentRoofTemperature * 2 + 0.5) / 2.0f;

  if (dataIndex == 9) {
    dataIndex = 0;
  }
}
