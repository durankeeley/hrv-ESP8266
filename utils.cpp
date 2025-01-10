#include "utils.h"

String decToHex(byte decValue, byte desiredStringLength) {
  String hexString = String(decValue, HEX);
  while (hexString.length() < desiredStringLength) hexString = "0" + hexString;
  return hexString;
}

unsigned int hexToDec(String hexString) {
  return strtoul(hexString.c_str(), nullptr, 16);
}
