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

  // Detectar cambio de isMonitoringActive de true a false
  if (prev_isMonitoringActive && !isMonitoringActive) {
    // Cambió de true a false, necesitamos enviar mensaje de fin
    waitingEndMonitoringACK = true;
    endMonitoringACKNumber = -1; // Resetear para que se guarde el número correcto en el primer envío
    lastSendTime_LoRa = 0; // Forzar envío inmediato
  }
  // Si vuelve a activarse, cancelar la espera del mensaje de fin
  if (!prev_isMonitoringActive && isMonitoringActive) {
    waitingEndMonitoringACK = false;
    endMonitoringACKNumber = -1; // Resetear el número de ACK
  }
  // Actualizar estado anterior
  prev_isMonitoringActive = isMonitoringActive;

  
  // Verificar si hay desconexión (diferencia > 10)
  if (diferencia > 10) {
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
  
  // Envío periódico por LoRa
  // Caso 1: Esperando ACK del mensaje de fin de monitoreo
  if (waitingEndMonitoringACK && (currentTime - lastSendTime_LoRa >= sendInterval_LoRa_receive_ack)) {
    // Guardar el número de ACK que se usará para este mensaje (antes de que sendMessage lo incremente)
    if (endMonitoringACKNumber == -1) {
      // Primera vez que enviamos el mensaje de fin, guardar el número de ACK
      endMonitoringACKNumber = loRa.lastSendACK + 1;
    }
    // Enviar mensaje de fin de monitoreo con ST:0,0,0,0,0
    char message[128];
    int len = snprintf(message, sizeof(message),
                       "GPS:%s,%s; ST:%d,%d,%d,%d,%d",
                       latitude.c_str(), longitude.c_str(),
                       0, 0, 0, 0, 0);
    if (len > 0 && len < (int)sizeof(message) - 1) {
      message[len] = '*';
      message[len + 1] = '\0';
    }
    loRa.sendMessage(message, false);
  }
  // Caso 2: Envío normal cuando isMonitoringActive está activo o hay emergencia
  else if ((currentTime - lastSendTime_LoRa >= sendInterval_LoRa_receive_ack) && (isMonitoringActive || detectorCaida.emergencia)) {
    char message[128];
    int len = snprintf(message, sizeof(message),
                       "GPS:%s,%s; ST:%d,%d,%d,%d,%d",
                       latitude.c_str(), longitude.c_str(),
                       1, detectorCaida.impacto ? 1 : 0,
                       detectorCaida.free_fall ? 1 : 0, detectorCaida.segunda_condicion_caida ? 1 : 0,
                       detectorCaida.emergencia ? 1 : 0);
    if (len > 0 && len < (int)sizeof(message) - 1) {
      message[len] = '*';
      message[len + 1] = '\0';
    }
    loRa.sendMessage(message, false);
  }

  // Verificar ACK para mensaje de fin de monitoreo
  if (waitingEndMonitoringACK && waitingACK && ((currentTime - lastSendTime_LoRa) < 30000)) {
    receivedACK = loRa.checkIncomeMessage();
    if (receivedACK) {
      // Verificar que el ACK recibido corresponda al mensaje de fin de monitoreo
      // El ACK debe ser >= endMonitoringACKNumber (puede ser mayor si el receptor procesó mensajes adicionales)
      if (endMonitoringACKNumber >= 0 && loRa.ultimoACK >= endMonitoringACKNumber) {
        Serial.print("SI LLEGO ACK - Mensaje de fin de monitoreo (ACK: ");
        Serial.print(loRa.ultimoACK);
        Serial.print(", Esperado: >= ");
        Serial.print(endMonitoringACKNumber);
        Serial.println(")");
        waitingACK = false;
        waitingEndMonitoringACK = false; // ACK recibido, ya no necesitamos seguir enviando
        endMonitoringACKNumber = -1; // Resetear el número de ACK
      } else {
        // El ACK recibido es de un mensaje anterior, seguir esperando
        Serial.print("ACK recibido es de mensaje anterior. Esperado: >= ");
        Serial.print(endMonitoringACKNumber);
        Serial.print(", Recibido: ");
        Serial.println(loRa.ultimoACK);
        // Mantener waitingACK = true para seguir esperando
      }
    }
    // Si receivedACK es false, no se recibió ningún mensaje, mantener waitingACK = true
  }
  // Verificar ACK para mensajes normales (solo si no estamos esperando ACK del mensaje de fin)
  else if (!waitingEndMonitoringACK && waitingACK && ((currentTime - lastSendTime_LoRa) < 30000) && (isMonitoringActive || detectorCaida.emergencia)) {
    receivedACK = loRa.checkIncomeMessage();
    waitingACK = !receivedACK;
    if (!waitingACK) { 
      Serial.println("SI LLEGO ACK");
    }
  }
}


