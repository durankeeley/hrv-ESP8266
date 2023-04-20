#ifndef TEMPERATURE_UTILS_H
#define TEMPERATURE_UTILS_H

#include <Arduino.h>

String decToHex(byte decValue, byte desiredStringLength);
unsigned int hexToDec(String hexString);
int iTotalDelay = 0;
void myDelay(int ms);

#endif
