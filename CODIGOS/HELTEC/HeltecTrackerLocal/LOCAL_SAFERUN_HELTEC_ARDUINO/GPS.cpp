#include "GPS.h"

// Variables globales del GPS
TinyGPSPlus gps;
String latitude = "";
String longitude = "";
String time_str = "";
bool gpsDataValid = false;
unsigned long lastGPSUpdate = 0;

// Pines GPS para Heltec
#define GPS_RX_PIN 33
#define GPS_TX_PIN 34
#define GPS_POWER_PIN 21  // VGNSS_CTRL

void configureGPS() {
    // Configurar pin de alimentación del GPS
    pinMode(GPS_POWER_PIN, OUTPUT);
    digitalWrite(GPS_POWER_PIN, HIGH);
    
    // Inicializar comunicación serial con GPS
    Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    delay(100);
    
    Serial.println("GPS configurado en pines 33 (RX) y 34 (TX)");
}

void getGpsData() {
    while (Serial1.available() > 0) {
        if (Serial1.peek() != '\n') {
            gps.encode(Serial1.read());
        } else {
            Serial1.read();
            if (gps.time.second() == 0) continue;
            
            // Actualizar strings de tiempo y coordenadas
            String new_time_str = String(gps.time.hour()) + ":" + 
                                 String(gps.time.minute()) + ":" + 
                                 String(gps.time.second()) + ":" + 
                                 String(gps.time.centisecond());
            
            String new_latitude = String("LAT: ") + String(gps.location.lat(), 6);
            String new_longitude = String("LON: ") + String(gps.location.lng(), 6);

            // Actualizar variables globales
            time_str = new_time_str;
            latitude = new_latitude;
            longitude = new_longitude;
            
            // Marcar datos como válidos si tenemos coordenadas
            if (gps.location.isValid()) {
                gpsDataValid = true;
                lastGPSUpdate = millis();
            }
            
            // Limpiar buffer
            while (Serial1.read() > 0) {}
        }
    }
}

bool isGPSValid() {
    return gpsDataValid && gps.location.isValid() && 
           (millis() - lastGPSUpdate) < 60000; // 1 minuto de timeout
}

float getLatitude() {
    return gps.location.lat();
}

float getLongitude() {
    return gps.location.lng();
}

String getTimeString() {
    return time_str;
}
