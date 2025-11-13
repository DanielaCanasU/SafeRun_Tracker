#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>



#include "Config.h"
#include "SX1262_Settings.h"
#include "SensorData.h"
#include "LoRaHandler.h"
#include "FirebaseHandler.h"
#include "UI.h"
#include "AppState.h"
#include "Utils.h"
#include "GPS.h"

HT_st7735 st7735;

static uint8_t RXBUFFER[RXBUFFER_SIZE];
static uint8_t RXPacketL = 0;
bool PUBLISH_FIREBASE = true;
bool OPEN_TO_PAIR = false;
bool WAITING_LORA = false;

// =============================================================================
// MAIN FUNCTIONS
// =============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("=== SafeRun Device Local Iniciando ===");

    //Inicia de subsistemas
    datos.init();
    lora.init();
    botones.init();
    display.init();
    geolocation.init();
    
    Serial.println("Subsistemas iniciados");
}

void loop() {
    unsigned long now = millis();
    static int lastSensor4 = 0;
    //Chequeo de subsistemas
    lora.checkIncomeMessage();
    geolocation.getGpsData();
    display.updateUI(now);

    //Chequeo de base de datos

    if (!dataBase.firebaseStarted && isRemoteModeSelected() && WiFi.status() == WL_CONNECTED) {
        dataBase.init();
    }

    if(dataBase.firebaseStarted) {
        dataBase.checkEnlace(OPEN_TO_PAIR);
    }
    if (!datos.timeSynced && WiFi.status() == WL_CONNECTED) {
        datos.setupTime();
    }

    if (isRemoteModeSelected() && (dataBase.isBaseDatosReady() && (now - dataBase.lastPublishTime >= PUBLISH_INTERVAL || dataBase.lastPublishTime == 0) && (PUBLISH_FIREBASE))) {
        dataBase.publicarDatos();
    }
        
    delay(10);
}


