#include "Backtrack.h"
#include <Preferences.h>
#include <math.h>
#include "AppState.h"

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

// Inicializar preferencias para waypoints
void initWaypointPrefs() {
  loadWaypointsFromStorage();
}

// Cargar waypoints desde almacenamiento
void loadWaypointsFromStorage() {
  Preferences waypointPrefs;
  waypointPrefs.begin("waypoints", false);
  waypointCount = waypointPrefs.getInt("count", 0);
  if (waypointCount > MAX_WAYPOINTS) waypointCount = MAX_WAYPOINTS;
  
  for (int i = 0; i < waypointCount; i++) {
    String prefix = "wp" + String(i) + "_";
    waypoints[i].latitude = waypointPrefs.getFloat((prefix + "lat").c_str(), 0.0f);
    waypoints[i].longitude = waypointPrefs.getFloat((prefix + "lon").c_str(), 0.0f);
    waypoints[i].name = waypointPrefs.getString((prefix + "name").c_str(), "Punto " + String(i + 1));
    waypoints[i].timestamp = waypointPrefs.getULong((prefix + "time").c_str(), 0);
    waypoints[i].isValid = true;
  }
  waypointPrefs.end();
}

// Guardar waypoints en almacenamiento
void saveWaypointsToStorage() {
  Preferences waypointPrefs;
  waypointPrefs.begin("waypoints", false);
  waypointPrefs.putInt("count", waypointCount);
  for (int i = 0; i < waypointCount; i++) {
    String prefix = "wp" + String(i) + "_";
    waypointPrefs.putFloat((prefix + "lat").c_str(), waypoints[i].latitude);
    waypointPrefs.putFloat((prefix + "lon").c_str(), waypoints[i].longitude);
    waypointPrefs.putString((prefix + "name").c_str(), waypoints[i].name);
    waypointPrefs.putULong((prefix + "time").c_str(), waypoints[i].timestamp);
  }
  waypointPrefs.end();
}

// Guardar posición actual como waypoint
void saveCurrentPositionAsWaypoint(const String& name) {
  if (!localPositionSet || waypointCount >= MAX_WAYPOINTS) return;
  
  initWaypointPrefs();
  
  waypoints[waypointCount].latitude = localLatitude;
  waypoints[waypointCount].longitude = localLongitude;
  waypoints[waypointCount].name = name;
  waypoints[waypointCount].timestamp = millis();
  waypoints[waypointCount].isValid = true;
  
  waypointCount++;
  saveWaypointsToStorage();
}

// Eliminar waypoint
void deleteWaypoint(int index) {
  // PROTECCIÓN CRÍTICA: Verificar índice antes de cualquier operación
  if (index < 0 || index >= waypointCount || waypointCount == 0) return;
  
  // Mover waypoints posteriores hacia adelante
  for (int i = index; i < waypointCount - 1; i++) {
    waypoints[i] = waypoints[i + 1];
  }
  
  waypointCount--;
  
  // Ajustar índice seleccionado después de borrar
  if (selectedWaypointIndex == index) {
    // Si se borró el seleccionado, ajustar al siguiente o al anterior
    if (waypointCount > 0) {
      selectedWaypointIndex = (index < waypointCount) ? index : waypointCount - 1;
    } else {
      selectedWaypointIndex = -1;
      hasWaypointTarget = false;
    }
  } else if (selectedWaypointIndex > index) {
    // Si el seleccionado está después del borrado, decrementar
    selectedWaypointIndex--;
  }
  
  // Verificación final de seguridad
  if (selectedWaypointIndex >= waypointCount) {
    selectedWaypointIndex = -1;
    hasWaypointTarget = false;
  }
  
  saveWaypointsToStorage();
}

// Seleccionar waypoint para navegación
void selectWaypoint(int index) {
  if (index >= 0 && index < waypointCount) {
    selectedWaypointIndex = index;
    hasWaypointTarget = true; // IMPORTANTE: Establecer esto ANTES de calcular navegación
    
    // Calcular navegación hacia el waypoint (incluso si no hay GPS local todavía)
    waypointNavigation.hasTarget = true;
    if (localPositionSet) {
      waypointNavigation.distance = calculateDistance(localLatitude, localLongitude, 
                                                   waypoints[index].latitude, waypoints[index].longitude);
      waypointNavigation.bearing = calculateBearing(localLatitude, localLongitude, 
                                                  waypoints[index].latitude, waypoints[index].longitude);
      waypointNavigation.heading = waypointNavigation.bearing;
      waypointNavigation.direction = getCardinalDirection(waypointNavigation.bearing);
    } else {
      // Si no hay GPS local, establecer valores por defecto
      waypointNavigation.distance = 0.0f;
      waypointNavigation.bearing = 0.0f;
      waypointNavigation.heading = 0.0f;
      waypointNavigation.direction = "N";
    }
  }
}

// Obtener waypoint seleccionado
bool getSelectedWaypoint(float& lat, float& lon, String& name) {
  if (selectedWaypointIndex >= 0 && selectedWaypointIndex < waypointCount) {
    lat = waypoints[selectedWaypointIndex].latitude;
    lon = waypoints[selectedWaypointIndex].longitude;
    name = waypoints[selectedWaypointIndex].name;
    return true;
  }
  return false;
}

// Obtener cantidad de waypoints
int getWaypointCount() {
  return waypointCount;
}

// Obtener nombre del waypoint
String getWaypointName(int index) {
  if (index >= 0 && index < waypointCount) {
    return waypoints[index].name;
  }
  return "";
}

// Limpiar todos los waypoints
void clearAllWaypoints() {
  waypointCount = 0;
  selectedWaypointIndex = -1;
  hasWaypointTarget = false;
  Preferences waypointPrefs;
  waypointPrefs.begin("waypoints", false);
  waypointPrefs.clear();
  waypointPrefs.end();
}

// Actualizar navegación hacia waypoint cuando cambia la posición local
void updateWaypointNavigation() {
  if (hasWaypointTarget && selectedWaypointIndex >= 0 && localPositionSet) {
    waypointNavigation.distance = calculateDistance(localLatitude, localLongitude, 
                                                 waypoints[selectedWaypointIndex].latitude, 
                                                 waypoints[selectedWaypointIndex].longitude);
    waypointNavigation.bearing = calculateBearing(localLatitude, localLongitude, 
                                                waypoints[selectedWaypointIndex].latitude, 
                                                waypoints[selectedWaypointIndex].longitude);
    waypointNavigation.heading = waypointNavigation.bearing;
    waypointNavigation.direction = getCardinalDirection(waypointNavigation.bearing);
  }
}

// Función para verificar si el GPS local está disponible
bool isLocalGPSAvailable() {
  return localPositionSet && (millis() - lastLocalGPSUpdate) < 60000; // 1 minuto timeout
}

// Función para actualizar la posición GPS local
void updateLocalGPSPosition(float lat, float lon) {
  if (lat != 0.0f && lon != 0.0f) {
    localLatitude = lat;
    localLongitude = lon;
    localPositionSet = true;
    lastLocalGPSUpdate = millis();
    
    // Si tenemos un waypoint seleccionado, recalcular navegación hacia él
    if (hasWaypointTarget && selectedWaypointIndex >= 0) {
      updateWaypointNavigation();
    }
  }
}

