#pragma once
#include <Arduino.h>
#include "SensorData.h"
#include "TrackingData.h"

void initUI();
void updateUI(unsigned long now);
void renderInfoLoRa(const SensorData &data);

// Funciones del sistema de rastreo
void updateTrackingData(const SensorData &data);
bool getTrackingData(TrackingData &data);
bool getNavigationData(NavigationData &nav);
void updateLocalGPSPosition(float lat, float lon);
bool isLocalGPSAvailable();


