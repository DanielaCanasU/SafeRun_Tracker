#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "SensorData.h"

// Funciones
bool initDataWebServer();
void handleDataWebServer();
void updateLastLoRaData(const SensorData& data);

// Variable global del servidor
extern WebServer server;
