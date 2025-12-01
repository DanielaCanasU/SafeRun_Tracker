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
}

void loop() {
  const unsigned long currentTime = millis();

  //Manejar si el usuario utilizó algún botón
  botones.checkUserEntry();

  
  // Verificar si hay desconexión (diferencia > 10)
  if (diferencia > 3) {
    if (!disconnectedScreenShown && currentScreen != SCREEN_DISCONNECTED) {
      // Mostrar pantalla de desconexión solo la primera vez
      currentScreen = SCREEN_DISCONNECTED;
      drawDisconnectedScreen(true);
      disconnectedScreenShown = true;
    }
    // Si el usuario ya salió de la pantalla, solo mostrar el símbolo (se maneja en los headers)
  } else {
    // Si se reconecta (diferencia <= 10)
    if (disconnectedScreenShown) {
      disconnectedScreenShown = false;
      // Si estaba en pantalla de desconexión, volver al menú principal
      if (currentScreen == SCREEN_DISCONNECTED) {
        currentScreen = SCREEN_MAIN_MENU;
        display.drawMenu(SCREEN_MAIN_MENU, true);
      } else {
        // Redibujar la pantalla actual para quitar el símbolo
        if (currentScreen == SCREEN_MAIN_MENU) {
          display.drawMenu(SCREEN_MAIN_MENU, true);
        } else if (currentScreen == SCREEN_GPS) {
          drawGPSScreen(true);
        } else if (currentScreen == SCREEN_MP3_FOLDER) {
          drawFolderScreen(true);
        } else if (currentScreen == SCREEN_MP3_PLAYER) {
          drawMP3Screen(true);
        } else if (currentScreen == SCREEN_MONITORING) {
          drawMonitoringScreen();
        } else if (currentScreen == SCREEN_INFO) {
          drawInfoScreen(true);
        } else if (currentScreen == SCREEN_EXERCISE) {
          drawExerciseScreen(true);
        }
      }
    }
  }

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
    // La pantalla de info se actualiza cuando diferencia cambia
    drawInfoScreen(false);
  }
  else if (currentScreen == SCREEN_EXERCISE) {
    handleExerciseScreen();
  } else if (currentScreen == SCREEN_DISCONNECTED) {
    // La pantalla de desconexión no necesita actualizaciones constantes
  }

  detectorCaida.checkStatus();
  
  // GPS feed
  getGpsData();
  // Envío periódico por LoRa (siempre). El contenido incluye GPS solo cuando isMonitoringActive == true
  //ENVIAR DATOS
  //if ((currentTime - lastSendTime_LoRa >= sendInterval_LoRa_receive_ack || !TRANSMISION_COMPLETADA)) {
  if ((currentTime - lastSendTime_LoRa >= sendInterval_LoRa_receive_ack) && (isMonitoringActive || detectorCaida.emergencia)) {

    //lastSendTime_LoRa = currentTime;
    
    char message[128];
    int len = 0;
    //if (isMonitoringActive || detectorCaida.emergencia) {

      len = snprintf(message, sizeof(message),
                     "GPS:%s,%s; ST:%d,%d,%d,%d,%d",
                     latitude.c_str(), longitude.c_str(),
                     1, detectorCaida.impacto ? 1 : 0,
                     detectorCaida.free_fall ? 1 : 0, detectorCaida.segunda_condicion_caida ? 1 : 0,
                     detectorCaida.emergencia ? 1 : 0);
    /*} else {

      len = snprintf(message, sizeof(message),
                     "ST:%d,%d,%d,%d,%d",
                     0, detectorCaida.impacto ? 1 : 0,
                     detectorCaida.free_fall ? 1 : 0, detectorCaida.segunda_condicion_caida ? 1 : 0,
                     detectorCaida.emergencia ? 1 : 0);
    }*/
    if (len > 0 && len < (int)sizeof(message) - 1) {
      message[len] = '*';
      message[len + 1] = '\0';
    }
    loRa.sendMessage(message, false);
    //LT.setMode(MODE_STDBY_RC);

    //LT.setRx(100);
  }

  if(waitingACK && ((currentTime - lastSendTime_LoRa) < 30000) && (isMonitoringActive || detectorCaida.emergencia)) {
    receivedACK = loRa.checkIncomeMessage();
    waitingACK = !receivedACK;
    if (!waitingACK) { 
      Serial.println("SI LLEGO ACK");
    }
  }
}


