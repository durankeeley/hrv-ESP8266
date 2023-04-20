#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#include <SoftwareSerial.h>

#define MSGSTARTSTOP 0x7E
byte dataIndex;
byte serialData[10];
byte checksumIndex;
byte targetFanSpeed;
bool dataStarted = false;
bool dataReceived = false;
void checkSwSerial(SoftwareSerial* ss);
char tempLocation;

#endif
