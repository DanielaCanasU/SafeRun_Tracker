#include "SensorData.h"
#include <Preferences.h>
#include "Config.h"

// Estructura para waypoints del backtrack
struct Waypoint {
  float latitude;
  float longitude;
  String name;
  unsigned long timestamp;
  bool isValid;
};

static const int MAX_WAYPOINTS = 30;
static int waypointCount = 0;
static Waypoint waypoints[MAX_WAYPOINTS];


Data::Data() {}

void Data::init() {
  initPaquete(currentData);

  {
    Preferences wifiPrefs;
    wifiPrefs.begin("wifi", false);
    // Leer datos si es necesario aquí
    wifiPrefs.end();
  }

  // Initialize waypoint preferences storage (NVS)
  {
    Preferences waypointPrefs;
    waypointPrefs.begin("waypoints", false);
    loadWaypointsFromStorage();
    waypointPrefs.end();
  }
}

void Data::initPaquete(SensorData &data) {
  data.latitude = 0.0f;
  data.longitude = 0.0f;
  data.state = 0;
  data.sensor1 = data.sensor2 = data.sensor3 = data.sensor4 = 0;
  data.rssi = 0;
  data.snr = 0;
  data.receivedAt = 0;
  data.accelX = data.accelY = data.accelZ = 0.0f;
}

void Data::setupTime() {
  configTime(TIMEZONE_OFFSET * 3600, 0, NTP_SERVER_1, NTP_SERVER_2);
  Serial.print("Esperando sincronización NTP");
  
  // Esperar máximo 10 segundos para evitar timeout del watchdog
  int attempts = 0;
  const int maxAttempts = 20; // 10 segundos (500ms * 20)
  
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2 && attempts < maxAttempts) {
      delay(500);
      Serial.print(".");
      now = time(nullptr);
      attempts++;
      
      // Alimentar el watchdog durante la espera
      yield();
  }
  
  if (now >= 8 * 3600 * 2) {
      Serial.println("¡Sincronizado!");
  } else {
      Serial.println("Timeout - continuando sin sincronización completa");
  }
  timeSynced = true;
}


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
/*
void initSensorData(SensorData &data) {
  data.latitude = 0.0f;
  data.longitude = 0.0f;
  data.state = 0;
  data.sensor1 = data.sensor2 = data.sensor3 = data.sensor4 = 0;
  data.rssi = 0;
  data.snr = 0;
  data.receivedAt = 0;
  data.accelX = data.accelY = data.accelZ = 0.0f;
}
  */

bool parseLoRaMessage(String message, SensorData &data) {
  message.replace("*", "");
  int stIndex  = message.indexOf("ST:");
  int accIndex = message.indexOf("ACC:");

  if (stIndex == -1) return false;

  // Intentar extraer GPS si existe
  int latIndex = message.indexOf("LAT:");
  int lonIndex = message.indexOf("LON:");
  if (latIndex != -1 && lonIndex != -1) {
    String latStr = message.substring(latIndex + 4, message.indexOf(",", latIndex));
    data.latitude = latStr.toFloat();
    String lonStr = message.substring(lonIndex + 4, message.indexOf(";", lonIndex));
    data.longitude = lonStr.toFloat();
  } else {
    data.latitude = 0.0f;
    data.longitude = 0.0f;
  }

  // Extraer ST hasta antes de ACC o hasta final
  String stStr;
  if (accIndex != -1) stStr = message.substring(stIndex + 3, accIndex - 1);
  else stStr = message.substring(stIndex + 3);

  int stParts[5] = {0};
  int lastIndex = 0;
  int currentIndex;
  for (int i = 0; i < 5; i++) {
    currentIndex = stStr.indexOf(",", lastIndex);
    String val;
    if (i < 4 && currentIndex != -1) { val = stStr.substring(lastIndex, currentIndex); lastIndex = currentIndex + 1; }
    else { val = stStr.substring(lastIndex); }
    stParts[i] = val.toInt();
  }
  data.state = stParts[0];
  data.sensor1 = stParts[1];
  data.sensor2 = stParts[2];
  data.sensor3 = stParts[3];
  data.sensor4 = stParts[4];

  if (accIndex != -1) {
    String accStr = message.substring(accIndex + 4);
    int c1 = accStr.indexOf(",");
    int c2 = accStr.indexOf(",", c1 + 1);
    if (c1 != -1 && c2 != -1) {
      data.accelX = accStr.substring(0, c1).toFloat();
      data.accelY = accStr.substring(c1 + 1, c2).toFloat();
      data.accelZ = accStr.substring(c2 + 1).toFloat();
    }
  }
  data.receivedAt = millis();
  return true;
}

void printSensorData(const SensorData &data) {
  Serial.println("=== Sensor Data ===");
  Serial.printf("Latitude: %.6f\n", data.latitude);
  Serial.printf("Longitude: %.6f\n", data.longitude);
  Serial.printf("State: %d\n", data.state);
  Serial.printf("Sensor1: %d\n", data.sensor1);
  Serial.printf("Sensor2: %d\n", data.sensor2);
  Serial.printf("Sensor3: %d\n", data.sensor3);
  Serial.printf("Sensor4: %d\n", data.sensor4);
  Serial.printf("RSSI: %d dBm\n", data.rssi);
  Serial.printf("SNR: %d dB\n", data.snr);
  Serial.printf("Accel X: %.2f\n", data.accelX);
  Serial.printf("Accel Y: %.2f\n", data.accelY);
  Serial.printf("Accel Z: %.2f\n", data.accelZ);
  Serial.printf("Received at: %lu ms\n", data.receivedAt);
  Serial.println("==================");
}