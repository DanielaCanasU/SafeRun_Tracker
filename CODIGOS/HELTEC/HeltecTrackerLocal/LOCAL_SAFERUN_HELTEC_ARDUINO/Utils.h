#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <time.h>

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Setup NTP time synchronization
 */
void setupTime();

/**
 * @brief Get current timestamp as formatted string
 * @param timestamp Buffer to store the timestamp
 * @param bufferSize Size of the buffer
 */
void obtenerTimestamp(char* timestamp, size_t bufferSize);

/**
 * @brief Get current timestamp as formatted string (overloaded for global buffer)
 */
void obtenerTimestamp();

/**
 * @brief Obtiene la hora actual en formato HHMMSS para usar como pointId
 * @param horaId Buffer donde se guarda el resultado
 * @param bufferSize Tamaño del buffer
 */
void obtenerHoraId(char* horaId, size_t bufferSize);

/**
 * @brief Print formatted debug message with timestamp
 * @param message Message to print
 */
void debugPrint(const String& message);

/**
 * @brief Print error message with timestamp
 * @param message Error message to print
 */
void errorPrint(const String& message);

/**
 * @brief Print success message with timestamp
 * @param message Success message to print
 */
void successPrint(const String& message);

/**
 * @brief Convert milliseconds to formatted time string
 * @param ms Milliseconds to convert
 * @return Formatted time string
 */
String formatTime(unsigned long ms);

/**
 * @brief Check if WiFi is connected
 * @return true if connected, false otherwise
 */
bool isWiFiConnected();

/**
 * @brief Print WiFi connection status
 */
void printWiFiStatus();

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
extern char timestamp[20];

#endif // UTILS_H
