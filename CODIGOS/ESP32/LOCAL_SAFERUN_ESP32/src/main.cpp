/**
 * @file main.cpp
 * @brief Main application file for ESP32 SafeRun device
 * @author Daniela Cañas y Gabriel Gimenez
 * @date 2025
 * 
 * This file contains the main application logic for the ESP32 SafeRun device.
 * It orchestrates the interaction between different modules:
 * - LoRa communication for sensor data reception
 * - Firebase integration for data storage and link requests
 * - Button handling for user interactions
 * - WiFi connectivity management
 */

// =============================================================================
// INCLUDES
// =============================================================================
#include <Arduino.h>
#include <time.h>

// Project includes
#include "config.h"
#include "SensorData.h"
#include "Utils.h"
#include "ButtonHandler.h"
#include "LoRaHandler.h"
#include "FirebaseHandler.h"

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
SensorData currentData;
unsigned long lastPublishTime = 0;
unsigned long lastCheck = 0;
static int lastState = 0;
static String sessionId = "";

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
void onLinkRequestPatternComplete();
String getFirestoreTimestamp();

// =============================================================================
// MAIN FUNCTIONS
// =============================================================================

/**
 * @brief Setup function - Initialize all modules and systems
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RYLR_RX, RYLR_TX);
    
    debugPrint("=== ESP32 SafeRun Device Starting ===");
    
    
    // Initialize sensor data structure
    initSensorData(currentData);
    
    // Initialize LoRa handler
    if (!initLoRaHandler()) {
        errorPrint("Failed to initialize LoRa handler");
        while (1) delay(1000);
    }
    
    // Initialize Firebase handler (includes WiFi setup)
    if (!initFirebaseHandler()) {
        errorPrint("Failed to initialize Firebase handler");
        while (1) delay(1000);
    }
    
    // Initialize button handler
    initButtonHandler();
    // Initialize time synchronization
    setupTime();
    
    successPrint("All systems initialized successfully");
    debugPrint("Device ready for operation");
}

/**
 * @brief Main loop function - Handle all ongoing operations
 */
void loop() {
    unsigned long now = millis();
    static int lastSensor4 = 0;
    // Check for pending link requests periodically
    if (now - lastCheck >= CHECK_INTERVAL) {
        lastCheck = now;
        if (checkForLinkRequest()) {
            debugPrint("Link request found - waiting for button pattern");
        }
    }
    // Process LoRa messages
    if (processLoRaMessage(currentData)) {
        debugPrint("Valid LoRa message received");
        printSensorData(currentData);
    }
    // Publish data to Firebase periódicamente
    if (isFirebaseReady() && (now - lastPublishTime >= PUBLISH_INTERVAL || lastPublishTime == 0)) {
        if (publishSensorDataToFirebase(currentData)) {
            lastPublishTime = now;
            // Manejo de sesiones
            if (lastState == 0 && currentData.state == 1) {
                // Inicia nueva sesión
                char sessionIdBuf[15];
                obtenerTimestamp(sessionIdBuf, sizeof(sessionIdBuf));
                sessionId = String(sessionIdBuf);
                startTrainingSession(String(DEVICE_ID), sessionId, currentData);
            } else if (lastState == 1 && currentData.state == 1 && sessionId != "") {
                // Sube punto a sesión activa
                char pointIdBuf[7];
                obtenerHoraId(pointIdBuf, sizeof(pointIdBuf));
                uploadSessionPoint(String(DEVICE_ID), sessionId, currentData, String(pointIdBuf));
            } else if (lastState == 1 && currentData.state == 0 && sessionId != "") {
                // Finaliza sesión
                endTrainingSession(String(DEVICE_ID), sessionId, getFirestoreTimestamp());
                sessionId = "";
            }
            lastState = currentData.state;
            // Publicar alarma solo en flanco ascendente de sensor4
            if (lastSensor4 == 0 && currentData.sensor4 == 1) {
                publishAlertToFirestore(currentData);
            }
            lastSensor4 = currentData.sensor4;
        }
    }
    // Process button patterns and long press detection
    processButtonPattern(now);
    // Small delay to prevent watchdog issues
    delay(10);
}

// =============================================================================
// CALLBACK FUNCTIONS
// =============================================================================

/**
 * @brief Callback function called when link request pattern is completed
 * This function is called from ButtonHandler when the user completes
 * the button pattern (3 clicks + long press)
 */
void onLinkRequestPatternComplete() {
    debugPrint("Link request pattern completed - processing request");
    
    if (processLinkRequest()) {
        successPrint("Link request processed successfully");
    } else {
        errorPrint("Failed to process link request");
    }
}