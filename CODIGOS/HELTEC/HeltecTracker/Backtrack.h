#pragma once
#include <Arduino.h>
#include "AppState.h"

// Constantes
#define EARTH_RADIUS 6371000.0  // Radio de la Tierra en metros
#define PI 3.14159265359

// Funciones de cálculo de distancia y dirección
float calculateDistance(float lat1, float lon1, float lat2, float lon2);
float calculateBearing(float lat1, float lon1, float lat2, float lon2);
String getCardinalDirection(float bearing);
NavigationData calculateNavigation(float currentLat, float currentLon, float targetLat, float targetLon);

// Funciones de gestión de waypoints
void initWaypointPrefs();
void loadWaypointsFromStorage();
void saveWaypointsToStorage();
void saveCurrentPositionAsWaypoint(const String& name);
void deleteWaypoint(int index);
void selectWaypoint(int index);
bool getSelectedWaypoint(float& lat, float& lon, String& name);
int getWaypointCount();
String getWaypointName(int index);
void clearAllWaypoints();
void updateWaypointNavigation();
bool isLocalGPSAvailable();
void updateLocalGPSPosition(float lat, float lon);

