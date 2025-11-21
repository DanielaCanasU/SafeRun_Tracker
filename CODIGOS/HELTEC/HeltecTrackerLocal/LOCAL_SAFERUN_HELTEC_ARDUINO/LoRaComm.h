#pragma once
#include <Arduino.h>
#include <SX126XLT.h>
#include "AppState.h"

bool sendCommand(String command, unsigned long timeout = 1000, bool waitForOK = true);
void configureSX1262();
void sendMessage(const char* message, bool verbose = false);
void checkForIncomingMessage();


class LoRa {
    private:
        int mensaje;
    public:
        LoRa();
        void init();
};


