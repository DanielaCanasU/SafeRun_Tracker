#pragma once
#include <Arduino.h>
#include "SensorData.h" // Necesario para el tipo SensorData
#include "AppState.h"

class Display {
    private:
        int menu;
    public:
        Display();
        void init();
        void updateUI(unsigned long now);
        //void drawMenu(MenuScreen currentMenu, bool firstDraw = true);
        //void drawStaticWelcome();  // Añadir declaración del método
};
// Funciones principales de la UI
void initUI();
void updateUI(unsigned long now);

// Funciones para actualizar el estado de la UI desde otros módulos
void updateLocalLoRaData(const SensorData& data);
void updateTrackingData(const SensorData& data);
void updateLocalGPSPosition(float lat, float lon);
void renderInfoLoRa(const SensorData &data);