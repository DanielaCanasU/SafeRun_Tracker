#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include <Arduino.h>
#include <SPI.h>
#include <SX126XLT.h>
#include "SensorData.h"

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize LoRa handler
 * @return true if initialization successful, false otherwise
 */
bool initLoRaHandler();

/**
 * @brief Configure SX1262 LoRa module
 */
void configureSX1262();

/**
 * @brief Process incoming LoRa messages
 * @param data Reference to SensorData struct to populate
 * @return true if valid message received, false otherwise
 */
bool processLoRaMessage(SensorData &data);

/**
 * @brief Send command to LoRa module (for RYLR998 compatibility)
 * @param cmd Command to send
 */
void sendLoRaCommand(String cmd);

/**
 * @brief Get last received packet info
 * @param rssi Pointer to store RSSI value
 * @param snr Pointer to store SNR value
 * @return Length of last received packet
 */
uint8_t getLastPacketInfo(int8_t* rssi, int8_t* snr);

/**
 * @brief Get packet statistics
 * @param packetCount Pointer to store packet count
 * @param errorCount Pointer to store error count
 */
void getPacketStats(uint32_t* packetCount, uint32_t* errorCount);

/**
 * @brief Print packet statistics
 */
void printPacketStats();

/**
 * @brief Get last received packet as string
 * @return Last packet as string
 */
String getLastPacketString();

// =============================================================================
// EXTERNAL VARIABLES
// =============================================================================
extern SX126XLT LT;

#endif // LORA_HANDLER_H 