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
  detectorCaida.init();
  display.init();
  loRa.init();
  geolocation.init();
  //musica.init();
  analogReadResolution(12);
}

void loop() {
  const unsigned long currentTime = millis();

  botones.checkUserEntry();

  // Botones
  //processButtonPress();

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


  // Monitoreo acelerómetro
  accelLoop();

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
                     1, impacto ? 1 : 0,
                     free_fall ? 1 : 0, segunda_condicion_caida ? 1 : 0,
                     emergencia ? 1 : 0,
                     x, y, z);
#else
      len = snprintf(message, sizeof(message),
                     "GPS:%s,%s; ST:%d,%d,%d,%d,%d",
                     latitude.c_str(), longitude.c_str(),
                     1, impacto ? 1 : 0,
                     free_fall ? 1 : 0, segunda_condicion_caida ? 1 : 0,
                     emergencia ? 1 : 0);
#endif
    } else {
#ifdef ENVIAR_ACELEROMETRO
      len = snprintf(message, sizeof(message),
                     "ST:%d,%d,%d,%d,%d; ACC:%.2f,%.2f,%.2f",
                     0, impacto ? 1 : 0,
                     free_fall ? 1 : 0, segunda_condicion_caida ? 1 : 0,
                     emergencia ? 1 : 0,
                     x, y, z);
#else
      len = snprintf(message, sizeof(message),
                     "ST:%d,%d,%d,%d,%d",
                     0, impacto ? 1 : 0,
                     free_fall ? 1 : 0, segunda_condicion_caida ? 1 : 0,
                     emergencia ? 1 : 0);
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


