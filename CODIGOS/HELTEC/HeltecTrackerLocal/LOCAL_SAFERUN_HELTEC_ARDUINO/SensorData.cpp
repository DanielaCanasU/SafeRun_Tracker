#include "SensorData.h"

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

bool parseLoRaMessage(const String& message, SensorData &data) {
  // Formato esperado: "LAT:lat_val,LON:lon_val;ST:s0,s1,s2,s3,s4;ACC:ax,ay,az"
  // O sin ACC: "LAT:lat_val,LON:lon_val;ST:s0,s1,s2,s3,s4"
  
  String cleanMessage = message;
  cleanMessage.replace("*", ""); // Eliminar caracteres especiales si los hay

  float lat, lon, ax, ay, az;
  int s0, s1, s2, s3, s4;

  // Intentar analizar el formato con la parte "ACC" primero.
  // sscanf devuelve el número de campos asignados correctamente.
  int accScanResult = sscanf(cleanMessage.c_str(), "LAT:%f,LON:%f;ST:%d,%d,%d,%d,%d;ACC:%f,%f,%f",
                             &lat, &lon, &s0, &s1, &s2, &s3, &s4, &ax, &ay, &az);

  if (accScanResult == 10) { // Los 10 campos se analizaron con éxito
    data.latitude = lat;
    data.longitude = lon;
    data.state = s0;
    data.sensor1 = s1;
    data.sensor2 = s2;
    data.sensor3 = s3;
    data.sensor4 = s4;
    data.accelX = ax;
    data.accelY = ay;
    data.accelZ = az;
    data.receivedAt = millis();
    return true;
  }

  // Si falló, intentar analizar sin la parte "ACC"
  int noAccScanResult = sscanf(cleanMessage.c_str(), "LAT:%f,LON:%f;ST:%d,%d,%d,%d,%d",
                               &lat, &lon, &s0, &s1, &s2, &s3, &s4);

  if (noAccScanResult == 7) { // Los 7 campos se analizaron con éxito
    data.latitude = lat;
    data.longitude = lon;
    data.state = s0;
    data.sensor1 = s1;
    data.sensor2 = s2;
    data.sensor3 = s3;
    data.sensor4 = s4;
    // Reiniciar datos del acelerómetro si no están en el mensaje
    data.accelX = 0.0f;
    data.accelY = 0.0f;
    data.accelZ = 0.0f;
    data.receivedAt = millis();
    return true;
  }
  // Si ambos intentos de análisis fallan
  return false;
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
