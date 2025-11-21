#pragma once
#include <Arduino.h>

// WiFi credentials
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// Firebase configuration
extern const char* FIREBASE_API_KEY;
extern const char* FIREBASE_PROJECT_ID;
extern const char* FIREBASE_DATABASE_URL;
extern const char* DEVICE_ID;
extern const char* USER_EMAIL;
extern const char* USER_PASSWORD;

// Timing configuration
extern const unsigned long PUBLISH_INTERVAL;
extern const unsigned long CHECK_INTERVAL;
extern const unsigned long LORA_CHECK_INTERVAL;

// Button configuration
extern const unsigned long BUTTON_DEBOUNCE_TIME;
extern const unsigned long BUTTON_CLICK_TIMEOUT;
extern const unsigned long LONG_PRESS_TIME;
extern const unsigned long PATTERN_TIMEOUT;
extern const int REQUIRED_CLICKS;

// LoRa configuration
extern const unsigned long LORA_TIMEOUT;

// Time zone configuration
extern const int TIMEZONE_OFFSET;
extern const char* NTP_SERVER_1;
extern const char* NTP_SERVER_2;

// WiFi stability configuration
extern const int WIFI_CONNECT_TIMEOUT;
extern const int WIFI_RETRY_DELAY;
extern const int WIFI_MAX_RETRIES;

// Memory management configuration
extern const size_t MIN_FREE_HEAP;
extern const size_t CRITICAL_HEAP;

// Button pins for Heltec Wireless Tracker v1.2
#define BTN_UP_PIN 4
#define BTN_DOWN_PIN 5
#define BTN_OK_PIN 6
#define BTN_BACK_PIN 18

// Function declarations
void initConfig();
bool isRemoteModeSelected();
void setRemoteMode(bool v);