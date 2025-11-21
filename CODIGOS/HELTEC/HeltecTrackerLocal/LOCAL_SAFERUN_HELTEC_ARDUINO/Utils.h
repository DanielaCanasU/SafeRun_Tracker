#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <time.h>

// =============================================================================
// TIME UTILITIES
// =============================================================================
void setupTime();
void obtenerTimestamp(char* timestamp, size_t bufferSize);
void obtenerTimestamp();
void obtenerHoraId(char* horaId, size_t bufferSize);

// =============================================================================
// DEBUG UTILITIES
// =============================================================================
void debugPrint(const String& message);
void errorPrint(const String& message);
void successPrint(const String& message);

// =============================================================================
// FORMATTING UTILITIES
// =============================================================================
String formatTime(unsigned long ms);

// =============================================================================
// WIFI UTILITIES
// =============================================================================
bool isWiFiConnected();
void printWiFiStatus();

// =============================================================================
// MEMORY MANAGEMENT FUNCTIONS
// =============================================================================
void checkMemoryStatus();
bool isMemoryLow();
void forceGarbageCollection();

// =============================================================================
// WATCHDOG AND STABILITY FUNCTIONS
// =============================================================================
void resetWatchdog();
void checkSystemStability();

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
extern char timestamp[20];

#endif // UTILS_H