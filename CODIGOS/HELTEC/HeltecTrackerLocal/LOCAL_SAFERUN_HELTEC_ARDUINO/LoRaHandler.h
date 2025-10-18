#pragma once
#include "SensorData.h"
#include <SX126XLT.h>

// External LoRa object declaration
extern SX126XLT LT;

// Function declarations
bool initLoRaHandler();
bool processLoRaMessage(SensorData &data);
void configureSX1262();
uint8_t getLastPacketInfo(int8_t* rssi, int8_t* snr);
void getPacketStats(uint32_t* packetCount, uint32_t* errorCount);
void printPacketStats();
String getLastPacketString();


