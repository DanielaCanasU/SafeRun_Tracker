#include "Utils.h"
#include "Config.h"
#include <WiFi.h>

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
char timestamp[20];

// =============================================================================
// UTILS IMPLEMENTATION
// =============================================================================

void setupTime() {
    configTime(TIMEZONE_OFFSET * 3600, 0, NTP_SERVER_1, NTP_SERVER_2);
    Serial.print("Esperando sincronización NTP");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("¡Sincronizado!");
}

void obtenerTimestamp(char* timestamp, size_t bufferSize) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Formato: AñoMesDíaHoraMinuto → "202506052210"
    strftime(timestamp, bufferSize, "%Y%m%d%H%M%S", &timeinfo);
}

void obtenerTimestamp() {
    obtenerTimestamp(timestamp, sizeof(timestamp));
    Serial.print("Timestamp: ");
    Serial.println(timestamp);
}

void debugPrint(const String& message) {
    obtenerTimestamp();
    Serial.printf("[DEBUG][%s] %s\n", timestamp, message.c_str());
}

void errorPrint(const String& message) {
    obtenerTimestamp();
    Serial.printf("[ERROR][%s] %s\n", timestamp, message.c_str());
}

void successPrint(const String& message) {
    obtenerTimestamp();
    Serial.printf("[SUCCESS][%s] %s\n", timestamp, message.c_str());
}

String formatTime(unsigned long ms) {
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    char timeStr[20];
    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(timeStr);
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void printWiFiStatus() {
    if (isWiFiConnected()) {
        Serial.printf("WiFi conectado. IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("WiFi desconectado");
    }
} 

void obtenerHoraId(char* horaId, size_t bufferSize) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    // Formato: HoraMinutoSegundo → "163754"
    strftime(horaId, bufferSize, "%H%M%S", &timeinfo);
}
