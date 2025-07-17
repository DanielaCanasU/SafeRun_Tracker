#include "SensorData.h"
#define PARSE_ACELEROMETRO

// =============================================================================
// SENSOR DATA IMPLEMENTATION
// =============================================================================

void initSensorData(SensorData &data) {
    data.latitude = 0.0f;
    data.longitude = 0.0f;
    data.hour = 0;
    data.minute = 0;
    data.second = 0;
    data.state = 0;
    data.sensor1 = 0;
    data.sensor2 = 0;
    data.sensor3 = 0;
    data.sensor4 = 0;
    data.rssi = 0;
    data.snr = 0;
    data.receivedAt = 0;
}

void parseLoRaMessage(String message, SensorData &data) {
    message.replace("*", ""); // Elimina asterisco final si está

    int latIndex = message.indexOf("LAT:");
    int lonIndex = message.indexOf("LON:");
    int stIndex = message.indexOf("ST:");
#ifdef PARSE_ACELEROMETRO
    int accIndex = message.indexOf("ACC:");
#endif

    if (latIndex != -1 && lonIndex != -1 && stIndex != -1) {
        // Extraer LATITUD
        String latStr = message.substring(latIndex + 4, message.indexOf(",", latIndex));
        data.latitude = latStr.toFloat();

        // Extraer LONGITUD
        String lonStr = message.substring(lonIndex + 4, message.indexOf(";", lonIndex));
        data.longitude = lonStr.toFloat();

        // Extraer estado y sensores (5 valores)
        String stStr;
#ifdef PARSE_ACELEROMETRO
        if (accIndex != -1) {
            stStr = message.substring(stIndex + 3, accIndex - 1); // hasta antes de ACC
        } else {
            stStr = message.substring(stIndex + 3);
        }
#else
        stStr = message.substring(stIndex + 3);
#endif
        int stParts[5] = {0};
        int lastIndex = 0;
        int currentIndex;

        for (int i = 0; i < 5; i++) {
            currentIndex = stStr.indexOf(",", lastIndex);
            String val;
            if (i < 4 && currentIndex != -1) {
                val = stStr.substring(lastIndex, currentIndex);
                lastIndex = currentIndex + 1;
            } else {
                val = stStr.substring(lastIndex);
            }
            stParts[i] = val.toInt();
        }

        // Asignar los valores
        data.state   = stParts[0];
        data.sensor1 = stParts[1];
        data.sensor2 = stParts[2];
        data.sensor3 = stParts[3];
        data.sensor4 = stParts[4];
        
#ifdef PARSE_ACELEROMETRO
        // Extraer acelerómetro si existe
        if (accIndex != -1) {
            String accStr = message.substring(accIndex + 4);
            int firstComma = accStr.indexOf(",");
            int secondComma = accStr.indexOf(",", firstComma + 1);
            if (firstComma != -1 && secondComma != -1) {
                data.accelX = accStr.substring(0, firstComma).toFloat();
                data.accelY = accStr.substring(firstComma + 1, secondComma).toFloat();
                data.accelZ = accStr.substring(secondComma + 1).toFloat();
            }
        }
#endif
        // RSSI y SNR se deben establecer desde el manejador LoRa
        data.receivedAt = millis();
        
        Serial.println("✅ Mensaje LoRa parseado correctamente");
    } else {
        Serial.println("❌ Formato inválido en mensaje LoRa.");
    }
}

void printSensorData(const SensorData &data) {
    Serial.println("=== DATOS DEL SENSOR ===");
    Serial.printf("Latitud: %.6f\n", data.latitude);
    Serial.printf("Longitud: %.6f\n", data.longitude);
    Serial.printf("Estado: %d\n", data.state);
    Serial.printf("Sensor 1: %d\n", data.sensor1);
    Serial.printf("Sensor 2: %d\n", data.sensor2);
    Serial.printf("Sensor 3: %d\n", data.sensor3);
    Serial.printf("Sensor 4: %d\n", data.sensor4);
    Serial.printf("RSSI: %d dBm\n", data.rssi);
    Serial.printf("SNR: %d dB\n", data.snr);
    Serial.printf("Recibido en: %lu ms\n", data.receivedAt);
    Serial.println("========================");
}

bool isValidSensorData(const SensorData &data) {
    // Verificar que las coordenadas estén en rangos válidos
    if (data.latitude < -90.0f || data.latitude > 90.0f) return false;
    if (data.longitude < -180.0f || data.longitude > 180.0f) return false;
    
    // Verificar que los sensores tengan valores binarios válidos
    if (data.sensor1 < 0 || data.sensor1 > 1) return false;
    if (data.sensor2 < 0 || data.sensor2 > 1) return false;
    if (data.sensor3 < 0 || data.sensor3 > 1) return false;
    if (data.sensor4 < 0 || data.sensor4 > 1) return false;
    
    // Verificar que el estado sea válido
    if (data.state < 0 || data.state > 1) return false;
    
    return true;
} 