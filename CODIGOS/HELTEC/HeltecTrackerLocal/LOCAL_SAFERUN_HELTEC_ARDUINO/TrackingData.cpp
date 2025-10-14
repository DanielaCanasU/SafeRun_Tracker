#include "TrackingData.h"
#include <math.h>

// Calcula la distancia entre dos puntos GPS usando la fórmula de Haversine
float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  float lat1Rad = lat1 * PI / 180.0;
  float lat2Rad = lat2 * PI / 180.0;
  float deltaLat = (lat2 - lat1) * PI / 180.0;
  float deltaLon = (lon2 - lon1) * PI / 180.0;

  float a = sin(deltaLat / 2) * sin(deltaLat / 2) +
            cos(lat1Rad) * cos(lat2Rad) *
            sin(deltaLon / 2) * sin(deltaLon / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));

  return EARTH_RADIUS * c;
}

// Calcula el bearing (dirección) entre dos puntos GPS
float calculateBearing(float lat1, float lon1, float lat2, float lon2) {
  float lat1Rad = lat1 * PI / 180.0;
  float lat2Rad = lat2 * PI / 180.0;
  float deltaLon = (lon2 - lon1) * PI / 180.0;

  float y = sin(deltaLon) * cos(lat2Rad);
  float x = cos(lat1Rad) * sin(lat2Rad) - sin(lat1Rad) * cos(lat2Rad) * cos(deltaLon);
  float bearing = atan2(y, x) * 180.0 / PI;

  // Normalizar a 0-360 grados
  if (bearing < 0) {
    bearing += 360.0;
  }

  return bearing;
}

// Convierte el bearing a dirección cardinal
String getCardinalDirection(float bearing) {
  if (bearing >= 337.5 || bearing < 22.5) return "N";
  if (bearing >= 22.5 && bearing < 67.5) return "NE";
  if (bearing >= 67.5 && bearing < 112.5) return "E";
  if (bearing >= 112.5 && bearing < 157.5) return "SE";
  if (bearing >= 157.5 && bearing < 202.5) return "S";
  if (bearing >= 202.5 && bearing < 247.5) return "SW";
  if (bearing >= 247.5 && bearing < 292.5) return "W";
  if (bearing >= 292.5 && bearing < 337.5) return "NW";
  return "N";
}

// Calcula todos los datos de navegación
NavigationData calculateNavigation(float currentLat, float currentLon, float targetLat, float targetLon) {
  NavigationData nav;
  
  // Verificar si las coordenadas son válidas
  if (currentLat == 0.0f && currentLon == 0.0f) {
    nav.hasTarget = false;
    return nav;
  }
  
  if (targetLat == 0.0f && targetLon == 0.0f) {
    nav.hasTarget = false;
    return nav;
  }

  nav.hasTarget = true;
  nav.distance = calculateDistance(currentLat, currentLon, targetLat, targetLon);
  nav.bearing = calculateBearing(currentLat, currentLon, targetLat, targetLon);
  nav.heading = nav.bearing;
  nav.direction = getCardinalDirection(nav.bearing);

  return nav;
}
