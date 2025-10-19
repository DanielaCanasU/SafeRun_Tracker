#pragma once
#include <Arduino.h>
#include "AppState.h"
#include "Pins.h"

void configureGPS();
void getGpsData();
void handleGPSScreen();
void handleMP3Screen();
void handleMonitoringScreen();
void handleExerciseScreen();

//void drawFolderScreen(bool firstDraw = false);
void handleFolderScreen();

// Funciones de utilidad GPS
bool isGPSValid();
float getLatitude();
float getLongitude();
String getTimeString();
unsigned long getExerciseElapsed(unsigned long now);


class Geolocation {
    private:
        int ubi;
    public:
        Geolocation();
        void init();
};
