// HeltecTracker.ino - Sketch principal para Arduino IDE

#include <Arduino.h>

// Configuración y estado global
#include "Config.h"
#include "Pins.h"
#include "AppState.h"
#include "SX1262_Settings.h"

// Módulos
#include "Display.h"
#include "Buttons.h"
#include "GPS.h"
#include "LoRaComm.h"
#include "DFPlayerMod.h"
#include "Accel.h"
#include "Battery.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  //Inicio de subsistemas
  botones.init();
  display.init(); //Bienvenida
  geolocation.init();
  display.drawMenu(SCREEN_MAIN_MENU, true);

  loRa.init();
  detectorCaida.init();
  analogReadResolution(12);
  // Transición automática: dibujar menú principal completo
  display.drawMenu(SCREEN_MAIN_MENU, true);
}

void loop() {
  const unsigned long currentTime = millis();

  //Manejar si el usuario utilizó algún botón
  botones.checkUserEntry();

  
  // Pantallas según menú
  if (currentScreen == SCREEN_MAIN_MENU) {
    // El menú principal no necesita actualizaciones constantes
  } else if (currentScreen == SCREEN_GPS) {
    handleGPSScreen();
  } else if (currentScreen == SCREEN_MP3_FOLDER) {
    handleFolderScreen();
  } else if (currentScreen == SCREEN_MP3_PLAYER) {
    handleMP3Screen();
  } else if (currentScreen == SCREEN_MONITORING) {
    handleMonitoringScreen();
  } else if (currentScreen == SCREEN_INFO) {
    // La pantalla de info no necesita actualizaciones constantes
  }
  else if (currentScreen == SCREEN_EXERCISE) {
    handleExerciseScreen();
  }

  detectorCaida.checkStatus();
  
  
  // Monitoreo acelerómetro
  //accelLoop();

  // GPS feed
  getGpsData();
  // Envío periódico por LoRa (siempre). El contenido incluye GPS solo cuando isMonitoringActive == true
  if ((currentTime - lastSendTime_LoRa >= sendInterval_LoRa)) {
    lastSendTime_LoRa = currentTime;
    char message[128];
    int len = 0;
    if (isMonitoringActive) {
#ifdef ENVIAR_ACELEROMETRO
      len = snprintf(message, sizeof(message),
                     "GPS:%s,%s; ST:%d,%d,%d,%d,%d; ACC:%.2f,%.2f,%.2f",
                     latitude.c_str(), longitude.c_str(),
                     1, detectorCaida.impacto ? 1 : 0,
                     detectorCaida.free_fall ? 1 : 0, detectorCaida.segunda_condicion_caida ? 1 : 0,
                     detectorCaida.emergencia ? 1 : 0,
                     x, y, z);
#else
      len = snprintf(message, sizeof(message),
                     "GPS:%s,%s; ST:%d,%d,%d,%d,%d",
                     latitude.c_str(), longitude.c_str(),
                     1, detectorCaida.impacto ? 1 : 0,
                     detectorCaida.free_fall ? 1 : 0, detectorCaida.segunda_condicion_caida ? 1 : 0,
                     detectorCaida.emergencia ? 1 : 0);
#endif
    } else {
#ifdef ENVIAR_ACELEROMETRO
      len = snprintf(message, sizeof(message),
                     "ST:%d,%d,%d,%d,%d; ACC:%.2f,%.2f,%.2f",
                     0, detectorCaida.impacto ? 1 : 0,
                     detectorCaida.free_fall ? 1 : 0, detectorCaida.segunda_condicion_caida ? 1 : 0,
                     detectorCaida.emergencia ? 1 : 0,
                     x, y, z);
#else
      len = snprintf(message, sizeof(message),
                     "ST:%d,%d,%d,%d,%d",
                     0, detectorCaida.impacto ? 1 : 0,
                     detectorCaida.free_fall ? 1 : 0, detectorCaida.segunda_condicion_caida ? 1 : 0,
                     detectorCaida.emergencia ? 1 : 0);
#endif
    }
    if (len > 0 && len < (int)sizeof(message) - 1) {
      message[len] = '*';
      message[len + 1] = '\0';
    }
    sendMessage(message, true);
  }

  // Batería
  batteryVoltage = readBatteryVoltage();
  int newPercent = batteryVoltageToPercent(batteryVoltage);
  if (newPercent != batteryPercent) {
    batteryPercent = newPercent;
  }
  // Actualizar indicador de batería en pantalla GPS solo cuando cambia
  //updateGPSBatteryIndicator();
}


