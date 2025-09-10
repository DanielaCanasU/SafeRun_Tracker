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

bool parseLoRaMessage(String message, SensorData &data) {
  message.replace("*", "");
  int latIndex = message.indexOf("LAT:");
  int lonIndex = message.indexOf("LON:");
  int stIndex  = message.indexOf("ST:");
  int accIndex = message.indexOf("ACC:");

  if (latIndex != -1 && lonIndex != -1 && stIndex != -1) {
    String latStr = message.substring(latIndex + 4, message.indexOf(",", latIndex));
    data.latitude = latStr.toFloat();

    String lonStr = message.substring(lonIndex + 4, message.indexOf(";", lonIndex));
    data.longitude = lonStr.toFloat();

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
    Serial.println(data.state);

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
