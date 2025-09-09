#pragma once
#include <Arduino.h>

struct SensorData {
  float latitude;
  float longitude;
  int state;
  int sensor1;
  int sensor2;
  int sensor3;
  int sensor4;
  int8_t rssi;
  int8_t snr;
  unsigned long receivedAt;
  float accelX, accelY, accelZ;
};

void initSensorData(SensorData &data);
bool parseLoRaMessage(const String& message, SensorData &data);
void printSensorData(const SensorData &data);
