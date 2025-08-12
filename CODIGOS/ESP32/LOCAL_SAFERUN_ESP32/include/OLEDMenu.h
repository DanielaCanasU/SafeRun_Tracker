#pragma once

#include <Arduino.h>
#include "SensorData.h"

// Initialize OLED and UI buttons
void initOLEDMenu();

// Call frequently from loop
void updateOLEDMenu(unsigned long now);

// Expose current selections
bool isRemoteModeSelected();
bool isWiFiConnectedViaMenu();

// Optionally render data on OLED in local mode
void renderLocalLoRaOnOLED(const SensorData &data);


