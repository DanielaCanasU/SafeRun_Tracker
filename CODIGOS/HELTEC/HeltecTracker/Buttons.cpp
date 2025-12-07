#include "Buttons.h"
#include "Arduino.h"
#include "Display.h"
#include "DFPlayerMod.h"
#include "GPS.h"
#include "AppState.h"
#include "Pins.h"
#include "Accel.h"
#include "Backtrack.h"
#include "LoRaComm.h"

Botones::Botones() {
  // Constructor implementation
}

void Botones::init(){
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);

  // Interrupciones botones
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), handleLeftInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), handleRightInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_SELECT), handleSelectInterrupt, CHANGE);

  leftButton = {0,0,false,false,0,0,0};
  rightButton = {0,0,false,false,0,0,0};
  selectButton = {0,0,false,false,0,0,0};
  Serial.println("--------------------------------");
  Serial.println("BOTONES INICIADO CORRECTAMENTE");
  Serial.println("--------------------------------");
}

void Botones::checkUserEntry(){

  //PROCESAR LAS ENTRADAS NO DE GESTOS
  currentTime = millis();

  left = leftButton.isPressed; 
  right = rightButton.isPressed; 
  sel = selectButton.isPressed;
  // Si el combo se acaba de activar, esperar un período de gracia antes de procesar otros botones
  if (comboJustActivated && (currentTime - comboActivatedTime >= 2000) && !left && !right && !sel) { // 1 segundo de gracia
    comboJustActivated = false;
  }
  
  // Si estamos en período de gracia después del combo, no procesar otros botones
  if (comboJustActivated) {
    return;
  }
  this->checkEmergencyCombo();
  
  // Combo LEFT + RIGHT para agregar waypoint (solo en Waypoint Manager)
  static bool comboAddWaypoint = false;
  static unsigned long comboAddStart = 0;
  if (currentScreen == SCREEN_WAYPOINT_MANAGER && left && right && !sel) {
    if (!comboAddWaypoint) {
      comboAddWaypoint = true;
      comboAddStart = currentTime;
    } else if (currentTime - comboAddStart >= 1000) { // 1 segundo para activar
      if (waypointCount < MAX_WAYPOINTS && localPositionSet) {
        String waypointName = "Punto " + String(waypointCount + 1);
        saveCurrentPositionAsWaypoint(waypointName);
        if (waypointCount > 3) {
          waypointScrollOffset = max(0, waypointCount - 3);
        }
        drawWaypointManagerScreen(false);
      }
      comboAddWaypoint = false;
    }
  } else {
    comboAddWaypoint = false;
  }

  if (!needProcessButton && !left && !right) return; //Si no hay entrada del usuario nos retiramos por donde vinimos

  if (currentScreen == SCREEN_MP3_PLAYER) {
    if (left && (currentTime - leftButton.pressStartTime >= LONG_PRESS_TIME)) {
      if (currentTime - musica.lastVolumeUpdateTime >= VOLUME_UPDATE_INTERVAL) {
        musica.volumeDown();
        musica.lastVolumeUpdateTime = currentTime;
      }
      return;
    }
    if (right && (currentTime - rightButton.pressStartTime >= LONG_PRESS_TIME)) {
      if (currentTime - musica.lastVolumeUpdateTime >= VOLUME_UPDATE_INTERVAL) {
        musica.volumeUp();
        musica.lastVolumeUpdateTime = currentTime;
      }
      return;
    }
  }

  //PROCESAR LOS GESTOS - UN SOLO TOQUE - DOS TOQUES - TRES TOQUES - TOQUE MANTENIDO LARGO

  if (!needProcessButton || leftButton.isPressed || rightButton.isPressed || selectButton.isPressed) return; 
  ButtonState* activeButton = nullptr; uint8_t buttonType = 0;

  //Identificar ultimo boton presionado 
  if (leftButton.lastPressTime > rightButton.lastPressTime && leftButton.lastPressTime > selectButton.lastPressTime) { activeButton = &leftButton; buttonType = BUTTON_LEFT; }
  else if (rightButton.lastPressTime > leftButton.lastPressTime && rightButton.lastPressTime > selectButton.lastPressTime) { activeButton = &rightButton; buttonType = BUTTON_RIGHT; }
  else if (selectButton.lastPressTime > leftButton.lastPressTime && selectButton.lastPressTime > rightButton.lastPressTime) { activeButton = &selectButton; buttonType = BUTTON_SELECT; }
  if (activeButton != nullptr) {
    //Si ya pasó el tiempo para que el usuario le pueda dar más toques 
    //o si ya se tienen la maxima cantidad de toques seguidos 
    if ((currentTime - activeButton->lastClickTime >= CLICK_TIMEOUT) || activeButton->clickCount >= 3) { 
      if (activeButton->wasLongPress) this->handleUserGestures(buttonType, true, false, false);
      else { bool isDoubleClick = (activeButton->clickCount == 2); bool isTripleClick = (activeButton->clickCount == 3); this->handleUserGestures(buttonType, false, isDoubleClick, isTripleClick); }
      activeButton->clickCount = 0; needProcessButton = false;
    }
  }

}

void Botones::checkEmergencyCombo() {
  if (left && right && sel) {
    if (!comboPressed) {
      comboPressed = true; 
      comboStart = currentTime; 
    }
    else if (currentTime - comboStart >= 3000) {
      if (!detectorCaida.emergencia) {
        detectorCaida.emergencia = true; 
        emergencyConfirmDeactivate = false;
      } else {
        emergencyConfirmDeactivate = true;
      }
      currentScreen = SCREEN_EMERGENCY;
      drawEmergencyScreen(true);
      comboPressed = false;
      comboJustActivated = true; // Marcar que el combo se acaba de activar
      comboActivatedTime = currentTime; // Guardar el tiempo de activación
      return; // Salir para evitar procesar otros botones
    }
  } else {
    comboPressed = false;
  }
}

void Botones::handleUserGestures(uint8_t button, bool isLongPress, bool isDoubleClick, bool isTripleClick) {
  // Manejo del menú principal
  if (currentScreen == SCREEN_MAIN_MENU) {
    if (button == BUTTON_LEFT && !isLongPress) {
      // Navegación hacia arriba (anterior)
      mainMenuSelection = (mainMenuSelection - 1 + MAIN_MENU_OPTIONS) % MAIN_MENU_OPTIONS;
      drawMainMenu();
    } else if (button == BUTTON_RIGHT && !isLongPress) {
      // Navegación hacia abajo (siguiente)
      mainMenuSelection = (mainMenuSelection + 1) % MAIN_MENU_OPTIONS;
      drawMainMenu();
    } else if (button == BUTTON_SELECT && !isLongPress) {
      // Navegar a la pantalla seleccionada
      switch (mainMenuSelection) {
        case 0: // GPS
          currentScreen = SCREEN_GPS;
          drawGPSScreen();
          break;
        case 1: // MP3 Player
          currentScreen = SCREEN_MP3_FOLDER;
          folderSelected = false;
          drawFolderScreen(true);
          break;
        case 2: // Monitoreo
          currentScreen = SCREEN_MONITORING;
          prev_impacto = detectorCaida.impacto;
          prev_free_fall = detectorCaida.free_fall;
          prev_segunda_condicion_caida = detectorCaida.segunda_condicion_caida;
          prev_emergencia = detectorCaida.emergencia;
          drawMonitoringScreen();
          break;
        case 3: // Info
          currentScreen = SCREEN_INFO;
          drawInfoScreen(true);
          break;
        case 4: // Ejercicio
          currentScreen = SCREEN_EXERCISE;
          drawExerciseScreen(true);
          break;
        case 5: // Emergencia
          currentScreen = SCREEN_EMERGENCY;
          drawEmergencyScreen(true);
          break;
        case 6: // Backtrack
          currentScreen = SCREEN_WAYPOINT_MANAGER;
          // Inicializar selectedWaypointIndex si hay waypoints
          loadWaypointsFromStorage();
          if (waypointCount > 0) {
            // Si no hay selección válida, seleccionar el primero
            if (selectedWaypointIndex < 0 || selectedWaypointIndex >= waypointCount) {
              selectedWaypointIndex = 0;
            }
          } else {
            selectedWaypointIndex = -1;
            hasWaypointTarget = false;
          }
          waypointScrollOffset = 0; // Resetear scroll
          drawWaypointManagerScreen(true);
          break;
        case 7: // Config Distancia
          currentScreen = SCREEN_DISTANCE_CONFIG;
          drawDistanceConfigScreen(true);
          break;
      }
    }
    return;
  }

  if (currentScreen == SCREEN_MP3_FOLDER) {
    if (button == BUTTON_SELECT && !isLongPress) {
      folderSelected = true; currentScreen = SCREEN_MP3_PLAYER;
      if (!isPlaying || currentFolder != lastFolder) { dfPlayer.playFolder(currentFolder, 1); isPlaying = true; currentSong = 1; }
      drawMP3Screen(true);
      Serial.print("Cancion:"); Serial.println(currentSong);
      Serial.print("Playlist:"); Serial.println(currentFolder);
      return;
    } else if (button == BUTTON_LEFT && !isLongPress) {
      if (currentFolder > 1) { currentFolder--; drawFolderScreen(true); }
    } else if (button == BUTTON_RIGHT && !isLongPress) {
      if (currentFolder < maxFolders) { currentFolder++; drawFolderScreen(true); } else { currentFolder = 1; drawFolderScreen(true); }
    }
  }

  if (button == BUTTON_SELECT) {
    if (isLongPress) {
      // Volver al menú principal desde cualquier pantalla
      // Si estaba en pantalla de desconexión, marcar que ya se mostró
      if (currentScreen == SCREEN_DISCONNECTED) {
        //disconnectedScreenShown = true;
      }
      lastRenderedScreen = currentScreen;
      currentScreen = SCREEN_MAIN_MENU;
      drawMainMenu();
      Serial.println("Volviendo al menú principal");
    } else if (isTripleClick) {
      if (currentScreen == SCREEN_MP3_PLAYER) { dfPlayer.reset(); drawMP3Screen(); }
    } else if (isDoubleClick) {
      if (currentScreen == SCREEN_MP3_PLAYER) { dfPlayer.enableLoop(); drawMP3Screen(); }
    } else {
      // Si está en pantalla de desconexión y presiona cualquier botón, marcar que ya se mostró
      if (currentScreen == SCREEN_DISCONNECTED && !isLongPress) {
        disconnectedScreenShown = true;
        currentScreen = SCREEN_MAIN_MENU;
        drawMainMenu();
        return;
      }
      if (currentScreen == SCREEN_MP3_PLAYER) {
        switch (currentMP3Option) {
          case MP3_PLAY_PAUSE:
            isPlaying = !isPlaying; if (isPlaying) dfPlayer.start(); else dfPlayer.pause(); break;
          case MP3_NEXT:
            isPlaying = true; currentSong++; if (currentSong > lista_canciones[currentFolder - 1]) { currentSong = 1; dfPlayer.playFolder(currentFolder, 1); break; } dfPlayer.next(); break;
          case MP3_PREV:
            currentSong--; isPlaying = true; if (currentSong < 1) { currentSong = lista_canciones[currentFolder - 1]; dfPlayer.playFolder(currentFolder, lista_canciones[currentFolder - 1]); break; } dfPlayer.previous(); break;
          case MP3_VOL_UP:
            if (currentVolume < 30) { currentVolume++; dfPlayer.volume(currentVolume); } break;
          case MP3_VOL_DOWN:
            if (currentVolume > 0) { currentVolume--; dfPlayer.volume(currentVolume); } break;
        }
        drawMP3Screen();
      }
      if (currentScreen == SCREEN_MONITORING) {
        if (!isMonitoringActive) { detectorCaida.impacto = false; detectorCaida.free_fall = false; detectorCaida.segunda_condicion_caida = false; detectorCaida.emergencia = false; detectorCaida.isCalibrated = false; }
        prev_impacto = detectorCaida.impacto; prev_free_fall = detectorCaida.free_fall; prev_segunda_condicion_caida = detectorCaida.segunda_condicion_caida; prev_emergencia = detectorCaida.emergencia;
        isMonitoringActive = !isMonitoringActive; drawMonitoringScreen();
      }
      if (currentScreen == SCREEN_EXERCISE) {
        // Select: iniciar/detener
        if (!isMonitoringActive) { 
          isMonitoringActive = true; 
          exerciseStartMs = millis(); 
          lastExercisePosSet = false;
          // Configurar SX1262 según la distancia seleccionada cuando se inicia el ejercicio
          // Verificar que selectedExerciseDistance esté en rango válido
          if (selectedExerciseDistance < DISTANCE_COUNT) {
            configureSX1262ForDistance(selectedExerciseDistance);
          } else {
            // Si está fuera de rango, usar valor por defecto
            Serial.println("ERROR: selectedExerciseDistance fuera de rango, usando 1.5km");
            configureSX1262ForDistance(DISTANCE_1_5KM);
          }
        }
        else { 
          isMonitoringActive = false; 
          exercisePausedAccumMs = exercisePausedAccumMs + (millis() - exerciseStartMs); 
        }
        drawExerciseScreen(false);
      }
      // Configuración de distancia: navegación y selección
      if (currentScreen == SCREEN_DISTANCE_CONFIG && button == BUTTON_SELECT && !isLongPress) {
        // Confirmar selección y volver al menú principal
        currentScreen = SCREEN_MAIN_MENU;
        Serial.println("Volviendo al menú principal");

        // Actualizar el intervalo de envío según la distancia
        switch(selectedExerciseDistance) {
          case DISTANCE_500M: sendInterval_LoRa_receive_ack = 2000; break;
          case DISTANCE_1KM:  sendInterval_LoRa_receive_ack = 5000; break;
          case DISTANCE_1_5KM: default: sendInterval_LoRa_receive_ack = 10000; break;
        }

        display.drawMenu(SCREEN_MAIN_MENU, true);
        Serial.println("Volviendo al menú principal");
        //return;
      }
      if (currentScreen == SCREEN_EMERGENCY && detectorCaida.emergencia && emergencyConfirmDeactivate) {
        // Confirmar apagado de emergencia
        detectorCaida.emergencia = false; emergencyConfirmDeactivate = false;
        drawEmergencyScreen(true);
      }
      // Backtrack: Waypoint Manager - SELECT para entrar a navegación
      if (currentScreen == SCREEN_WAYPOINT_MANAGER && button == BUTTON_SELECT && !isLongPress) {
        // Solo entrar a navegación si hay waypoints y uno está seleccionado
        if (waypointCount > 0 && selectedWaypointIndex >= 0 && selectedWaypointIndex < waypointCount) {
          // Seleccionar el waypoint y establecer el target ANTES de cambiar la pantalla
          selectWaypoint(selectedWaypointIndex);
          
          // Verificar que se estableció correctamente antes de cambiar
          if (hasWaypointTarget && selectedWaypointIndex >= 0 && selectedWaypointIndex < waypointCount) {
            // Cambiar a pantalla de navegación
            currentScreen = SCREEN_BACKTRACK;
            // Dibujar la pantalla
            drawBacktrackScreen(true);
            // IMPORTANTE: Salir inmediatamente para evitar que se ejecute la siguiente condición
            return;
          }
        }
        // Si no hay waypoints o ninguno seleccionado, quedarse en Waypoint Manager
        return; // También salir si no se puede entrar a navegación
      }
      // Backtrack: Navegación - SELECT para volver a gestión
      // IMPORTANTE: Usar else if para que NO se ejecute si ya se cambió a SCREEN_BACKTRACK arriba
      // Y verificar que NO sea long press (long press es para volver al menú principal)
      else if (currentScreen == SCREEN_BACKTRACK && button == BUTTON_SELECT && !isLongPress) {
        currentScreen = SCREEN_WAYPOINT_MANAGER;
        drawWaypointManagerScreen(true);
        return; // Salir después de cambiar
      }
      // Backtrack: Mapa - SELECT para volver a gestión
      else if (currentScreen == SCREEN_BACKTRACK_MAP && button == BUTTON_SELECT) {
        currentScreen = SCREEN_WAYPOINT_MANAGER;
        drawWaypointManagerScreen(true);
        return; // Salir después de cambiar
      }
    }
  } else {
    if (isTripleClick) {
      if (button == BUTTON_LEFT) {
        currentSong--; isPlaying = true; if (currentSong < 1) { currentSong = lista_canciones[currentFolder - 1]; dfPlayer.playFolder(currentFolder, lista_canciones[currentFolder - 1]); } else { dfPlayer.previous(); }
      } else if (button == BUTTON_RIGHT) {
        currentSong++; if (currentSong > lista_canciones[currentFolder - 1]) { currentSong = 1; dfPlayer.playFolder(currentFolder, 1); isPlaying = true; } else { isPlaying = true; dfPlayer.next(); }
      }
      drawMP3Screen();
    } else if (isDoubleClick) {
      if (button == BUTTON_LEFT) { dfPlayer.pause(); isPlaying = false; }
      else if (button == BUTTON_RIGHT) { dfPlayer.start(); isPlaying = true; }
      drawMP3Screen();
    } else if (!isLongPress) {
      if (currentScreen == SCREEN_MP3_PLAYER) {
        if (button == BUTTON_RIGHT) currentMP3Option = (MP3Option)((currentMP3Option + 1) % MP3_OPTION_COUNT);
        else if (button == BUTTON_LEFT) currentMP3Option = (MP3Option)((currentMP3Option + MP3_OPTION_COUNT - 1) % MP3_OPTION_COUNT);
        drawMP3Screen();
      }
      if (currentScreen == SCREEN_EXERCISE) {
        // Izquierda cancela popup de caída
        static bool fallPopupActiveFlag = false; // local no persistente
        // drawExerciseScreen ya gestiona con variable global en Display.cpp
        // No hacemos nada aquí aparte de redibujar
        drawExerciseScreen(false);
      }
      // Configuración de distancia: navegación LEFT/RIGHT
      if (currentScreen == SCREEN_DISTANCE_CONFIG && !isLongPress) {
        if (button == BUTTON_LEFT) {
          selectedExerciseDistance = (ExerciseDistance)((selectedExerciseDistance - 1 + DISTANCE_COUNT) % DISTANCE_COUNT);
          drawDistanceConfigScreen(false);
        } else if (button == BUTTON_RIGHT) {
          selectedExerciseDistance = (ExerciseDistance)((selectedExerciseDistance + 1) % DISTANCE_COUNT);
          drawDistanceConfigScreen(false);
        }
      }
      // Backtrack: Waypoint Manager - navegación
      if (currentScreen == SCREEN_WAYPOINT_MANAGER) {
        if (button == BUTTON_LEFT && !isLongPress) {
          if (waypointCount > 0) {
            selectedWaypointIndex = (selectedWaypointIndex - 1 + waypointCount) % waypointCount;
            int maxScrollOffset = max(0, waypointCount - 3);
            if (selectedWaypointIndex < waypointScrollOffset) {
              waypointScrollOffset = max(0, selectedWaypointIndex);
            }
            drawWaypointManagerScreen(false);
          }
        } else if (button == BUTTON_RIGHT && !isLongPress) {
          if (waypointCount > 0) {
            int prevSelected = selectedWaypointIndex;
            selectedWaypointIndex = (selectedWaypointIndex + 1) % waypointCount;
            int maxScrollOffset = max(0, waypointCount - 3);
            if (selectedWaypointIndex >= waypointScrollOffset + 3) {
              waypointScrollOffset = min(maxScrollOffset, selectedWaypointIndex - 2);
            }
            if (prevSelected == waypointCount - 1 && selectedWaypointIndex == 0) {
              waypointScrollOffset = 0;
            }
            drawWaypointManagerScreen(false);
          }
        } else if (button == BUTTON_LEFT && isLongPress) {
          // LEFT long press: eliminar waypoint seleccionado
          // PROTECCIÓN: Verificar índices antes de borrar
          if (waypointCount > 0 && selectedWaypointIndex >= 0 && selectedWaypointIndex < waypointCount) {
            deleteWaypoint(selectedWaypointIndex);
            // PROTECCIÓN: Verificar que selectedWaypointIndex sea válido después de borrar
            if (selectedWaypointIndex >= waypointCount) {
              selectedWaypointIndex = -1;
            }
            int maxScrollOffset = max(0, waypointCount - 3);
            if (waypointScrollOffset > maxScrollOffset) {
              waypointScrollOffset = maxScrollOffset;
            }
            drawWaypointManagerScreen(false);
          }
        }
      }
      // Backtrack: Navegación - cambiar a mapa
      if (currentScreen == SCREEN_BACKTRACK && (button == BUTTON_LEFT || button == BUTTON_RIGHT)) {
        currentScreen = SCREEN_BACKTRACK_MAP;
        drawBacktrackMapScreen(true);
        return;
      } else if (currentScreen == SCREEN_BACKTRACK_MAP && (button == BUTTON_LEFT || button == BUTTON_RIGHT)) {
        currentScreen = SCREEN_BACKTRACK;
        drawBacktrackScreen(true);
        return;
      }
    }
  }
  
}




void IRAM_ATTR handleLeftInterrupt() {
  unsigned long currentTime = millis();
  bool buttonState = digitalRead(BUTTON_LEFT);
  if (currentTime - leftButton.lastPressTime >= DEBOUNCE_TIME) {
    if (buttonState == LOW && !leftButton.isPressed) {
      leftButton.pressStartTime = currentTime;
      leftButton.isPressed = true;
      leftButton.lastPressTime = currentTime;
      leftButton.lastVolumeUpdateTime = currentTime;
    }
  }
  if (buttonState == HIGH && leftButton.isPressed) {
    leftButton.isPressed = false;
    leftButton.wasLongPress = (currentTime - leftButton.pressStartTime >= LONG_PRESS_TIME);
    if (!leftButton.wasLongPress) {
      if (currentTime - leftButton.lastClickTime <= TRIPLE_CLICK_TIME) leftButton.clickCount++;
      else leftButton.clickCount = 1;
      leftButton.lastClickTime = currentTime;
    }
    needProcessButton = true;
  }
}

void IRAM_ATTR handleRightInterrupt() {
  unsigned long currentTime = millis();
  bool buttonState = digitalRead(BUTTON_RIGHT);
  if (currentTime - rightButton.lastPressTime >= DEBOUNCE_TIME) {
    if (buttonState == LOW && !rightButton.isPressed) {
      rightButton.pressStartTime = currentTime;
      rightButton.isPressed = true;
      rightButton.lastPressTime = currentTime;
      rightButton.lastVolumeUpdateTime = currentTime;
    }
  }
  if (buttonState == HIGH && rightButton.isPressed) {
    rightButton.isPressed = false;
    rightButton.wasLongPress = (currentTime - rightButton.pressStartTime >= LONG_PRESS_TIME);
    if (!rightButton.wasLongPress) {
      if (currentTime - rightButton.lastClickTime <= TRIPLE_CLICK_TIME) rightButton.clickCount++;
      else rightButton.clickCount = 1;
      rightButton.lastClickTime = currentTime;
    }
    needProcessButton = true;
  }
}

void IRAM_ATTR handleSelectInterrupt() {
  unsigned long currentTime = millis();
  bool buttonState = digitalRead(BUTTON_SELECT);
  if (currentTime - selectButton.lastPressTime >= DEBOUNCE_TIME) {
    if (buttonState == LOW && !selectButton.isPressed) {
      selectButton.pressStartTime = currentTime;
      selectButton.isPressed = true;
      selectButton.lastPressTime = currentTime;
    }
  }
  if (buttonState == HIGH && selectButton.isPressed) {
    selectButton.isPressed = false;
    selectButton.wasLongPress = (currentTime - selectButton.pressStartTime >= LONG_PRESS_TIME);
    if (!selectButton.wasLongPress) {
      if (currentTime - selectButton.lastClickTime <= TRIPLE_CLICK_TIME) selectButton.clickCount++;
      else selectButton.clickCount = 1;
      selectButton.lastClickTime = currentTime;
    }
    needProcessButton = true;
  }
}

void processButtonPress() {
  unsigned long currentTime = millis();
  /*
  // Detección global de combo 3 botones (izq+der+select): mantener 3s
  static bool comboPressed = false; 
  static unsigned long comboStart = 0;
  static unsigned long comboActivatedTime = 0; // Tiempo cuando se activó el combo
  static bool comboJustActivated = false; // Flag para indicar que el combo se acaba de activar
  */
  bool left = leftButton.isPressed; 
  bool right = rightButton.isPressed; 
  bool sel = selectButton.isPressed;
  /*
  // Si el combo se acaba de activar, esperar un período de gracia antes de procesar otros botones
  if (comboJustActivated && (currentTime - comboActivatedTime >= 2000) && !left && !right && !sel) { // 1 segundo de gracia
    comboJustActivated = false;
  }
  
  // Si estamos en período de gracia después del combo, no procesar otros botones
  if (comboJustActivated) {
    return;
  }
  
  if (left && right && sel) {
    if (!comboPressed) { 
      comboPressed = true; 
      comboStart = currentTime; 
    }
    else if (currentTime - comboStart >= 3000) {
      if (!emergencia) {
        emergencia = true; 
        emergencyConfirmDeactivate = false;
      } else {
        emergencyConfirmDeactivate = true;
      }
      currentScreen = SCREEN_EMERGENCY;
      drawEmergencyScreen(true);
      comboPressed = false;
      comboJustActivated = true; // Marcar que el combo se acaba de activar
      comboActivatedTime = currentTime; // Guardar el tiempo de activación
      return; // Salir para evitar procesar otros botones
    }
  } else {
    comboPressed = false;
  }
  */
  /*
  if (!needProcessButton && !leftButton.isPressed && !rightButton.isPressed) return;
  
  if (currentScreen == SCREEN_MP3_PLAYER) {
    if (leftButton.isPressed && (currentTime - leftButton.pressStartTime >= LONG_PRESS_TIME)) {
      if (currentTime - leftButton.lastVolumeUpdateTime >= VOLUME_UPDATE_INTERVAL) {
        if (currentVolume > 0) { currentVolume--; dfPlayer.volume(currentVolume); drawMP3Screen(); }
        leftButton.lastVolumeUpdateTime = currentTime;
      }
      return;
    }
    if (rightButton.isPressed && (currentTime - rightButton.pressStartTime >= LONG_PRESS_TIME)) {
      if (currentTime - rightButton.lastVolumeUpdateTime >= VOLUME_UPDATE_INTERVAL) {
        if (currentVolume < 30) { currentVolume++; dfPlayer.volume(currentVolume); drawMP3Screen(); }
        rightButton.lastVolumeUpdateTime = currentTime;
      }
      return;
    }
  }
  */
  if (!needProcessButton || leftButton.isPressed || rightButton.isPressed || selectButton.isPressed) return;
  ButtonState* activeButton = nullptr; uint8_t buttonType = 0;
  if (leftButton.lastPressTime > rightButton.lastPressTime && leftButton.lastPressTime > selectButton.lastPressTime) { activeButton = &leftButton; buttonType = BUTTON_LEFT; }
  else if (rightButton.lastPressTime > leftButton.lastPressTime && rightButton.lastPressTime > selectButton.lastPressTime) { activeButton = &rightButton; buttonType = BUTTON_RIGHT; }
  else if (selectButton.lastPressTime > leftButton.lastPressTime && selectButton.lastPressTime > rightButton.lastPressTime) { activeButton = &selectButton; buttonType = BUTTON_SELECT; }
  if (activeButton != nullptr) {
    if ((currentTime - activeButton->lastClickTime >= CLICK_TIMEOUT) || activeButton->clickCount >= 3) {
      if (activeButton->wasLongPress) handleButtonPress(buttonType, true, false, false);
      else { bool isDoubleClick = (activeButton->clickCount == 2); bool isTripleClick = (activeButton->clickCount == 3); handleButtonPress(buttonType, false, isDoubleClick, isTripleClick); }
      activeButton->clickCount = 0; needProcessButton = false;
    }
  }
}

void handleButtonPress() {}

