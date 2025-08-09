#pragma once
#include <Arduino.h>
#include "AppState.h"
#include "Pins.h"

void configureGPS();
void getGpsData();
void handleGPSScreen();
void handleMP3Screen();
void handleMonitoringScreen();
void drawFolderScreen(bool firstDraw = false);
void handleFolderScreen();


