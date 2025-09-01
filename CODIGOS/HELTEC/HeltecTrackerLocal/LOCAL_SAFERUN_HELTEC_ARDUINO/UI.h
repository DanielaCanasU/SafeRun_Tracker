#pragma once
#include <Arduino.h>
#include "SensorData.h"
#include "TrackingData.h"

void initUI();
void updateUI(unsigned long now);
void renderInfoLoRa(const SensorData &data);

// Funciones de la pantalla de bienvenida
void renderWelcomeScreen();
void renderBlinkingEyes();
void renderWelcomeMessage();
bool isWelcomeScreenComplete();

// Funciones del sistema de rastreo
void updateTrackingData(const SensorData &data);
bool getTrackingData(TrackingData &data);
bool getNavigationData(NavigationData &nav);
void updateLocalGPSPosition(float lat, float lon);
bool isLocalGPSAvailable();

// Funciones del sistema de backtrack
void saveCurrentPositionAsWaypoint(const String& name);
void deleteWaypoint(int index);
void selectWaypoint(int index);
bool getSelectedWaypoint(float& lat, float& lon, String& name);
int getWaypointCount();
String getWaypointName(int index);
void clearAllWaypoints();



