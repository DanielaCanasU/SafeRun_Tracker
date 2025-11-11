#pragma once
#include <Arduino.h>
#include "SensorData.h" // Necesario para el tipo SensorData

extern bool manualWifi;
// Funciones principales de la UI
void initUI();
void updateUI(unsigned long now);

// Funciones para actualizar el estado de la UI desde otros módulos
void updateLocalLoRaData(const SensorData& data);
void updateTrackingData(const SensorData& data);
void updateLocalGPSPosition(float lat, float lon);
void renderInfoLoRa(const SensorData &data);