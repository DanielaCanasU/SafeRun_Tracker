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

/*
void setupTime() {
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
}

*/

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

// =============================================================================
// MEMORY MANAGEMENT FUNCTIONS
// =============================================================================

void checkMemoryStatus() {
    size_t freeHeap = ESP.getFreeHeap();
    
    Serial.printf("Memoria libre: %d bytes\n", freeHeap);
    
    // Advertencia si la memoria está baja
    if (freeHeap < 15000) { // Menos de 15KB libres
        Serial.println("ADVERTENCIA: Memoria baja detectada!");
    }
    
    if (freeHeap < 8000) { // Menos de 8KB libres - crítico
        Serial.println("CRITICO: Memoria muy baja! Posible reinicio inminente!");
    }
}

bool isMemoryLow() {
    return ESP.getFreeHeap() < 10000; // Menos de 10KB libres
}

void forceGarbageCollection() {
    // Forzar limpieza de memoria
    Serial.println("Forzando limpieza de memoria...");
    
    // Alimentar el watchdog durante la limpieza
    yield();
    delay(100);
    
    // Liberar memoria del WiFi si es necesario
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect(true);
        yield();
        delay(500);
    }
    
    // Verificar memoria después de la limpieza
    size_t freeHeap = ESP.getFreeHeap();
    Serial.printf("Memoria después de limpieza: %d bytes\n", freeHeap);
}

// =============================================================================
// WATCHDOG AND STABILITY FUNCTIONS
// =============================================================================

void resetWatchdog() {
    // Alimentar el watchdog para prevenir reinicios
    yield();
    
    // También verificar si hay tareas pendientes
    if (WiFi.status() == WL_CONNECTED) {
        // Pequeña operación para mantener el WiFi activo
        WiFi.RSSI();
    }
}

void checkSystemStability() {
    static unsigned long lastCheck = 0;
    unsigned long now = millis();
    
    // Verificar cada 10 segundos (menos frecuente para reducir carga)
    if (now - lastCheck >= 10000) {
        lastCheck = now;
        
        // Verificar memoria solo si es crítica
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 15000) {
            Serial.printf("Memoria baja: %d bytes\n", freeHeap);
        }
        
        // Verificar estado WiFi solo si está conectado
        if (WiFi.status() == WL_CONNECTED) {
            int rssi = WiFi.RSSI();
            if (rssi < -80) {
                Serial.println("Senal WiFi debil detectada");
            }
        }
        
        // Resetear watchdog
        resetWatchdog();
    }
}
