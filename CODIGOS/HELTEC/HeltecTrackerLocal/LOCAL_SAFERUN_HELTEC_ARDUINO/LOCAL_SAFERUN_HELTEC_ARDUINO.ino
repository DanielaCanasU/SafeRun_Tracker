#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>
//hOLA

#include "Config.h"
#include "SX1262_Settings.h"
#include "SensorData.h"
#include "LoRaHandler.h"
#include "FirebaseHandler.h"
#include "UI.h"
#include "AppState.h"
#include "Utils.h"
#include "GPS.h"

// Define the global display object
HT_st7735 st7735;

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
// MAIN FUNCTIONS
// =============================================================================

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    
    Serial.println("=== Heltec SafeRun Device Starting ===");
    
    // Initialize sensor data structure
    initSensorData(currentData);
    
    // Initialize LoRa handler
    if (!initLoRaHandler()) {
        Serial.println("Failed to initialize LoRa handler");
        while (1) delay(1000);
    }
    
    // Initialize Firebase handler (objects only; do not connect yet)
    if (!initFirebaseHandler()) {
        Serial.println("Failed to initialize Firebase handler");
        while (1) delay(1000);
    }
    
    // Initialize UI
    initUI();
    
    // Initialize GPS
    configureGPS();
    
    Serial.println("All systems initialized successfully");
    Serial.println("Device ready for operation");
}

void loop() {
    unsigned long now = millis();
    static int lastSensor4 = 0;
    
    // Check for pending link requests periodically
    if (OPEN_TO_PAIR) {
        if (now - lastCheck >= CHECK_INTERVAL) {
            lastCheck = now;
            if (checkForLinkRequest()) {
                processLinkRequest();
            }
        }
    }
    
    // LoRa reception logic (exactly like ESP32)
    if(!WAITING_LORA) {
        LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);  //set for IRQ on RX done or timeout
        LT.setRx(LORA_TIMEOUT);
        WAITING_LORA = true;
    }
    
    // Process LoRa messages
    if (now - lastLoraCheck >= LORA_CHECK_INTERVAL) {
        if(digitalRead(14)) {
                processLoRaMessage(currentData);
                Serial.println("Valid LoRa message received");
                printSensorData(currentData);
                
                // Update tracking system with received data
                updateTrackingData(currentData);
                
                WAITING_LORA = false;
        }   
        lastLoraCheck = now;
    }
    
    // Get GPS data
    getGpsData();
    
    // Update local GPS position for tracking system
    if (isGPSValid()) {
        updateLocalGPSPosition(getLatitude(), getLongitude());
    }
    
    // UI update
    updateUI(now);

    // Start Firebase only when in remote mode and WiFi is connected via menu
    static bool firebaseStarted = false;
    static bool timeSynced = false;
    if (!firebaseStarted && isRemoteModeSelected() && WiFi.status() == WL_CONNECTED) {
        firebaseStarted = startFirebase();
    }
    
    // Sync time once WiFi is available and not yet synced
    if (!timeSynced && WiFi.status() == WL_CONNECTED) {
        setupTime();
        timeSynced = true;
    }

    // Publish data to Firebase periodically only if remote mode
    if (isRemoteModeSelected() && (isFirebaseReady() && (now - lastPublishTime >= PUBLISH_INTERVAL || lastPublishTime == 0) && (PUBLISH_FIREBASE))) {
        Serial.println("Publicando");
        if (publishSensorDataToFirebase(currentData)) {
            Serial.println("Exitoso");
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
    
    // In local mode, optionally draw quick status onto display
    if (!isRemoteModeSelected()) {
        renderInfoLoRa(currentData);
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}


