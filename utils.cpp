#include "utils.h"

String decToHex(byte decValue, byte desiredStringLength) {
  char hexString[desiredStringLength + 1];
  sprintf(hexString, "%0*X", desiredStringLength, decValue);
  return String(hexString);
}

// unsigned int hexToDec(String hexString) {
//   unsigned int decValue = 0;
//   for (int i = 0; i < (int)hexString.length(); i++) {
//     int nextInt = (int)hexString.charAt(i);
//     if (nextInt >= '0' && nextInt <= '9') {
//       nextInt = map(nextInt, '0', '9', 0, 9);
//     } else if (nextInt >= 'A' && nextInt <= 'F') {
//       nextInt = map(nextInt, 'A', 'F', 10, 15);
//     } else if (nextInt >= 'a' && nextInt <= 'f') {
//       nextInt = map(nextInt, 'a', 'f', 10, 15);
//     } else {
//       nextInt = 0;
//     }
//     decValue = (decValue * 16) + nextInt;
//   }
//   return decValue;
// }

unsigned int hexToDec(String hexString) {
  return strtoul(hexString.c_str(), nullptr, 16);
}

// // This function yields back to the watchdog to avoid random ESP8266 resets
// void myDelay(int ms)  
// {
//   int i;
//   for(i=1; i!=ms; i++) 
//   {
//     delay(1);
//     if(i%100 == 0) 
//    {
//       ESP.wdtFeed(); 
//       yield();
//     }
//   }
//   iTotalDelay+=ms;
// }
