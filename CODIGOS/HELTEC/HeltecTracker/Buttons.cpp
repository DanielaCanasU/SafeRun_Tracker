#include "Buttons.h"
#include "Display.h"
#include "DFPlayerMod.h"
#include "GPS.h"

void initButtonsState() {
  leftButton = {0,0,false,false,0,0,0};
  rightButton = {0,0,false,false,0,0,0};
  selectButton = {0,0,false,false,0,0,0};
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
  if (!needProcessButton && !leftButton.isPressed && !rightButton.isPressed) return;
  unsigned long currentTime = millis();
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

void handleButtonPress(uint8_t button, bool isLongPress, bool isDoubleClick, bool isTripleClick) {
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
      if (currentScreen == SCREEN_MP3_FOLDER) currentScreen = (MenuScreen)((currentScreen + 2) % SCREEN_COUNT);
      else if (currentScreen == SCREEN_MP3_PLAYER) currentScreen = SCREEN_MP3_FOLDER;
      else { currentScreen = (MenuScreen)((currentScreen + 1) % SCREEN_COUNT); Serial.println(currentScreen); }
      if (currentScreen == SCREEN_GPS) drawGPSScreen();
      else if (currentScreen == SCREEN_MP3_FOLDER) { folderSelected = false; drawFolderScreen(true); }
      else if (currentScreen == SCREEN_MONITORING) { prev_impacto = impacto; prev_free_fall = free_fall; prev_segunda_condicion_caida = segunda_condicion_caida; prev_emergencia = emergencia; drawMonitoringScreen(); }
      Serial.println(currentScreen);
    } else if (isTripleClick) {
      if (currentScreen == SCREEN_MP3_PLAYER) { dfPlayer.reset(); drawMP3Screen(); }
    } else if (isDoubleClick) {
      if (currentScreen == SCREEN_MP3_PLAYER) { dfPlayer.enableLoop(); drawMP3Screen(); }
    } else {
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
        if (!isMonitoringActive) { impacto = false; free_fall = false; segunda_condicion_caida = false; emergencia = false; isCalibrated = false; }
        prev_impacto = impacto; prev_free_fall = free_fall; prev_segunda_condicion_caida = segunda_condicion_caida; prev_emergencia = emergencia;
        isMonitoringActive = !isMonitoringActive; drawMonitoringScreen();
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
    }
  }
}


