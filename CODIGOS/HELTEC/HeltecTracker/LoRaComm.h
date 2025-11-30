#pragma once
#include <Arduino.h>
#include <SX126XLT.h>
#include "AppState.h"

bool sendCommand(String command, unsigned long timeout = 1000, bool waitForOK = true);
void configureSX1262();
void sendMessage(const char* message, bool verbose = false);
void checkForIncomingMessage();
bool checkForACK();
bool waitForACK();
bool processLoRaMessage();
String descifrarValor(String codificado);


class LoRa {
    private:
        int mensaje;
        bool WAITING_LORA = false;
        unsigned long lastLoraCheck = 0;
    public:
        LoRa();
        void init();
        bool checkIncomeMessage();
        void sendMessage(const char* message, bool verbose);
        String cifrarValor(String texto);
        int ultimoACK;
        int lastSendACK = 0;
};


