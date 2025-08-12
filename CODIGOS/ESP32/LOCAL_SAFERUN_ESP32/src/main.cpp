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
#include "OLEDMenu.h"

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
SensorData currentData;
unsigned long lastPublishTime = 0;
unsigned long lastCheck = 0;
unsigned long lastLoraCheck = 0;
static int lastState = 0;
static String sessionId = "";


static uint8_t RXBUFFER[RXBUFFER_SIZE];
static uint8_t RXPacketL = 0;
bool PUBLISH_FIREBASE = true;
bool OPEN_TO_PAIR = false;
bool WAITING_LORA = false;

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
    
    // Initialize Firebase handler (objects only; do not connect yet)
    if (!initFirebaseHandler()) {
        errorPrint("Failed to initialize Firebase handler");
        while (1) delay(1000);
    }
    
    // Initialize button handler
    initButtonHandler();
    // Initialize OLED menu (WiFi/time will be handled after user connects)
    initOLEDMenu();
    // Do not call setupTime() here to avoid blocking before WiFi is connected
    
    successPrint("All systems initialized successfully");
    debugPrint("Device ready for operation");
    pinMode(4, INPUT);
}

/**
 * @brief Main loop function - Handle all ongoing operations
 */
void loop() {
    unsigned long now = millis();
    static int lastSensor4 = 0;
    // Check for pending link requests periodically
    if (OPEN_TO_PAIR) {
        if (now - lastCheck >= CHECK_INTERVAL) {
            lastCheck = now;
            if (checkForLinkRequest()) {
                debugPrint("Link request found - waiting for button pattern");
                onLinkRequestPatternComplete();
            }
        }
    }
    uint8_t busy_timeout_cnt;
    busy_timeout_cnt = 0;
    /*
    while (digitalRead(22))
    {
      delay(1);
      busy_timeout_cnt++;
  
      if (busy_timeout_cnt > 10) //wait 10mS for busy to complete
      {
        busy_timeout_cnt = 0;
        Serial.println(F("ERROR - Busy Timeout!"));
        break;
      }
    }
      */
    //RXPacketL = LT.receive(RXBUFFER, RXBUFFER_SIZE, LORA_TIMEOUT, NO_WAIT);
    //LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);  //set for IRQ on RX done or timeout
    /*//LENTOOOO
    LT.setRx(LORA_TIMEOUT);
    uint16_t IRQStatus;
    IRQStatus = LT.readIrqStatus(); 
    */
   if(!WAITING_LORA) {
        LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);  //set for IRQ on RX done or timeout
        LT.setRx(LORA_TIMEOUT);
        WAITING_LORA = true;
   }
    /*Serial.printf("IRQStatus: 0b");
    for (int i = 15; i >= 0; i--) {
        Serial.print((IRQStatus >> i) & 1);
    }
    Serial.println();
    */
    
    //LT.printIrqStatus();
    // Process LoRa messages
    if (now - lastLoraCheck >= LORA_CHECK_INTERVAL) {
        //if (IRQStatus & IRQ_RX_DONE) {
        if(digitalRead(4)) {
            //Serial.printf("SE ACTIVO");
            processLoRaMessage(currentData);
            debugPrint("Valid LoRa message received");
            printSensorData(currentData);
            WAITING_LORA = false;
        }
    }
    
   /* RAPIDOOOO
    LT.setRx(LORA_TIMEOUT);
    processLoRaMessage(currentData);
    debugPrint("Valid LoRa message received");
    printSensorData(currentData);
    */
    // UI update first
    updateOLEDMenu(now);

    // Start Firebase only when in remote mode and WiFi is connected via menu
    static bool firebaseStarted = false;
    static bool timeSynced = false;
    if (!firebaseStarted && isRemoteModeSelected() && isWiFiConnectedViaMenu()) {
        firebaseStarted = startFirebase();
    }
    // Sync time once WiFi is available (remote or local) and not yet synced
    if (!timeSynced && isWiFiConnectedViaMenu()) {
        setupTime();
        timeSynced = true;
    }

    // Publish data to Firebase periódicamente only if remote mode
    if (isRemoteModeSelected() && (isFirebaseReady() && (now - lastPublishTime >= PUBLISH_INTERVAL || lastPublishTime == 0) && (PUBLISH_FIREBASE))) {
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
    OPEN_TO_PAIR = processButtonPattern(now);

    // In local mode, optionally draw quick status onto OLED
    if (!isRemoteModeSelected()) {
        renderLocalLoRaOnOLED(currentData);
    }
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