#pragma once

#include <Arduino.h>

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// PIN CONFIGURATION
// =============================================================================
#define RYLR_RX 16  // RX del ESP32 <- TX del RYLR998
#define RYLR_TX 17  // TX del ESP32 -> RX del RYLR998
#define BUTTON_PIN 13

// =============================================================================
// WIFI CREDENTIALS
// =============================================================================
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// =============================================================================
// FIREBASE CONFIGURATION
// =============================================================================
extern const char* FIREBASE_API_KEY;
extern const char* FIREBASE_PROJECT_ID;
extern const char* DEVICE_ID;
extern const char* USER_EMAIL;
extern const char* USER_PASSWORD;

// =============================================================================
// TIMING CONFIGURATION
// =============================================================================
extern const unsigned long PUBLISH_INTERVAL;
extern const unsigned long CHECK_INTERVAL;
extern const unsigned long LORA_CHECK_INTERVAL;

// =============================================================================
// BUTTON CONFIGURATION
// =============================================================================
extern const unsigned long BUTTON_DEBOUNCE_TIME;
extern const unsigned long BUTTON_CLICK_TIMEOUT;
extern const unsigned long LONG_PRESS_TIME;
extern const unsigned long PATTERN_TIMEOUT;
extern const int REQUIRED_CLICKS;

// =============================================================================
// LORA CONFIGURATION
// =============================================================================
#define RXBUFFER_SIZE 100
extern const unsigned long LORA_TIMEOUT;

// =============================================================================
// TIME ZONE CONFIGURATION
// =============================================================================
extern const int TIMEZONE_OFFSET;
extern const char* NTP_SERVER_1;
extern const char* NTP_SERVER_2;

#endif // CONFIG_H 