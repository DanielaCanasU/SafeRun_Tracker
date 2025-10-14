#pragma once
#include <Arduino.h>

// Estructura para datos de rastreo
struct TrackingData {
  float latitude;
  float longitude;
  float altitude;
  float speed;
  float course;
  unsigned long timestamp;
  uint8_t deviceId;
  bool isValid;
};

// Estructura para cálculos de navegación
struct NavigationData {
  float distance;      // Distancia en metros
  float bearing;       // Dirección en grados (0-360)
  float heading;       // Dirección hacia el objetivo
  bool hasTarget;      // Si hay un objetivo válido
  String direction;    // Dirección cardinal (N, NE, E, SE, S, SW, W, NW)
};

// Funciones de cálculo de distancia y dirección
float calculateDistance(float lat1, float lon1, float lat2, float lon2);
float calculateBearing(float lat1, float lon1, float lat2, float lon2);
String getCardinalDirection(float bearing);
NavigationData calculateNavigation(float currentLat, float currentLon, float targetLat, float targetLon);

// Constantes
#define EARTH_RADIUS 6371000.0  // Radio de la Tierra en metros
#define PI 3.14159265359
