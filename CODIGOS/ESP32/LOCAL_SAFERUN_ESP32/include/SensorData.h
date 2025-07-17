#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <Arduino.h>

// =============================================================================
// SENSOR DATA STRUCTURE
// =============================================================================
struct SensorData {
    float latitude;
    float longitude;
    int hour;
    int minute;
    int second;
    int state;
    int sensor1;
    int sensor2;
    int sensor3;
    int sensor4;
    int rssi;
    int snr;
    unsigned long receivedAt; // Timestamp de cuándo se recibió el mensaje
    float accelX = 0.0f; // Acelerómetro X
    float accelY = 0.0f; // Acelerómetro Y
    float accelZ = 0.0f; // Acelerómetro Z
};

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Parse a LoRa message and extract sensor data
 * @param message The raw LoRa message string
 * @param data Reference to SensorData struct to populate
 */
void parseLoRaMessage(String message, SensorData &data);

/**
 * @brief Initialize a SensorData struct with default values
 * @param data Reference to SensorData struct to initialize
 */
void initSensorData(SensorData &data);

/**
 * @brief Print sensor data to Serial for debugging
 * @param data Reference to SensorData struct to print
 */
void printSensorData(const SensorData &data);

/**
 * @brief Check if sensor data is valid
 * @param data Reference to SensorData struct to validate
 * @return true if data is valid, false otherwise
 */
bool isValidSensorData(const SensorData &data);

#endif // SENSOR_DATA_H 