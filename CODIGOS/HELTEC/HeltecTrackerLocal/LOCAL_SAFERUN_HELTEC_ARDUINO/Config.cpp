#include "Config.h"

// =============================================================================
// CONFIGURATION IMPLEMENTATION
// =============================================================================

// WiFi credentials
const char* WIFI_SSID = "Redmi Note 9S";
const char* WIFI_PASSWORD = "28082002";

// Firebase configuration
const char* FIREBASE_API_KEY = "AIzaSyCJKe-5L4JoW3pdE-0EMIN0Sl6rUHdHIEs";
const char* FIREBASE_PROJECT_ID = "monitoreodeportistas";
const char* FIREBASE_DATABASE_URL = "https://monitoreodeportistas-default-rtdb.firebaseio.com";
const char* DEVICE_ID = "123456";
const char* USER_EMAIL = "justtoprivacy@gmail.com";
const char* USER_PASSWORD = "28082002";

// Timing configuration
const unsigned long PUBLISH_INTERVAL = 20000;  // 20 seconds
const unsigned long CHECK_INTERVAL = 10000;    // 10 seconds
const unsigned long LORA_CHECK_INTERVAL = 100; // 100ms

// Button configuration
const unsigned long BUTTON_DEBOUNCE_TIME = 200;   // milliseconds
const unsigned long BUTTON_CLICK_TIMEOUT = 600;   // milliseconds
const unsigned long LONG_PRESS_TIME = 3000;       // milliseconds
const unsigned long PATTERN_TIMEOUT = 8000;       // milliseconds
const int REQUIRED_CLICKS = 3;                    // clicks for pattern

// LoRa configuration
const unsigned long LORA_TIMEOUT = 10000;  // 10 seconds

// Time zone configuration
const int TIMEZONE_OFFSET = -5;  // GMT-5 (Colombia)
const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.nist.gov";

// WiFi stability configuration
const int WIFI_CONNECT_TIMEOUT = 30000;  // 30 segundos timeout para conexión WiFi
const int WIFI_RETRY_DELAY = 1000;       // 1 segundo entre reintentos
const int WIFI_MAX_RETRIES = 3;          // Máximo 3 reintentos de conexión

// Memory management configuration
const size_t MIN_FREE_HEAP = 15000;      // Mínimo 15KB de memoria libre
const size_t CRITICAL_HEAP = 8000;       // 8KB crítico - posible reinicio

// default remote mode is true (Firebase)
static bool remoteMode = true;

void initConfig() {
  // Nothing yet; placeholder for future board-specific init
}

bool isRemoteModeSelected() { return remoteMode; }
void setRemoteMode(bool v) { remoteMode = v; }