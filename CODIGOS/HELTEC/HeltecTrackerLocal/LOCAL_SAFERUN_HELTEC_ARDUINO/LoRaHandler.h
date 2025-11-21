#pragma once
#include "SensorData.h"
#include <SX126XLT.h>
#include "AppState.h"


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


class LoRa {
    private:
        int mensaje;
        bool WAITING_LORA = false;
        unsigned long lastLoraCheck = 0;
    public:
        LoRa();
        void init();
        void checkIncomeMessage();
};