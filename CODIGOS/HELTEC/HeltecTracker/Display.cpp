#include "Display.h"
#include "GPS.h"

void displayInit() {
  st7735.st7735_init();
}

void drawMenuDots(MenuScreen currentMenu) {
  const int dotRadius = 1;
  const int activeDotRadius = 2;
  const int dotSpacing = 12;
  const int yPosition = 73;
  const int xStart = 80 - dotSpacing/2;
  for (int i = 0; i < 3; i++) {
    int xPos = xStart + i * dotSpacing;
    bool isActive = false;
    if (i == 0) isActive = (currentMenu == SCREEN_GPS);
    else if (i == 1) isActive = (currentMenu == SCREEN_MP3_FOLDER || currentMenu == SCREEN_MP3_PLAYER);
    else if (i == 2) isActive = (currentMenu == SCREEN_MONITORING);
    const int radius = isActive ? activeDotRadius : dotRadius;
    const uint16_t color = isActive ? ST7735_WHITE : ST7735_GRAY;
    for (int dy = -radius; dy <= radius; dy++) {
      for (int dx = -radius; dx <= radius; dx++) {
        if (dx*dx + dy*dy <= radius*radius) st7735.st7735_draw_pixel(xPos + dx, yPosition + dy, color);
      }
    }
  }
}

void drawMainMenu() {
  // Si es la primera vez que entramos a esta pantalla, dibujar el fondo estático
  if (lastRenderedScreen != SCREEN_MAIN_MENU) {
    st7735.st7735_fill_rectangle(0, 20, 128, 70, ST7735_BLACK);

    drawHeaderWithWiFi("Inicio");
    // Dibujar la barra blanca de selección UNA SOLA VEZ
    fillRectPixels(0, 34, 160, 26, ST7735_WHITE);
    lastRenderedScreen = SCREEN_MAIN_MENU;
    lastMainMenuIdx = -1; // Forzar un redibujado completo de los items del menú
  }

  // Si el índice del menú ha cambiado, redibujar solo los items
  if (lastMainMenuIdx != mainMenuSelection) {
    // --- Limpieza selectiva ---
    // 1. Limpiar el área de texto superior (encima de la barra blanca)
    st7735.st7735_fill_rectangle(0, 15, 160, 19, ST7735_BLACK);

    // 2. Limpiar el área de texto inferior (debajo de la barra blanca)
    st7735.st7735_fill_rectangle(0, 60, 160, 20, ST7735_BLACK);

    // 3. Limpiar el contenido anterior de la barra blanca (sin redibujar toda la barra).
    //    Esto es necesario para borrar el texto e icono antiguos antes de dibujar los nuevos.
    fillRectPixels(20, 34, 120, 26, ST7735_WHITE); // Limpia el centro de la barra blanca

    // --- Redibujado de items ---
    const char* menuItems[] = {"GPS", "Musica", "Monitoreo", "Info", "Ejercicio", "Emergencia"};

    // Dibujar opción de arriba (si existe)
    if (mainMenuSelection > 0) {
      int aboveIdx = mainMenuSelection - 1;
      String aboveText = String(menuItems[aboveIdx]);
      // Calculate centered positions
      int iconX = 35; // Center of screen (128/2 - 18/2 = 55)
      int textX = 55; // Icon center + icon width/2 + spacing
      switch (aboveIdx) {
        case 0: // GPS
          textX = 80;
          iconX = 55;
          break;
        case 1: // Musica
          textX = 68;
          iconX = 48;
          break;
        case 2: // Monitoreo
          textX = 57;
          iconX = 37;
          break;
        case 3: // Info
          textX = 75;
          iconX = 55;
          break;
        case 4: // Ejercicio
          textX = 53;
          iconX = 33;
          break;
      }

      // Icon for above (on black bg) - centered
      drawMenuIcon(iconX, 15, aboveIdx, false, ST7735_BLACK);
      st7735.st7735_write_str(textX, 20, aboveText.c_str(), Font_7x10, ST7735_GRAY, ST7735_BLACK);
    }

    // Dibujar opción seleccionada (sobre la barra blanca ya existente)
    String selectedText = String(menuItems[mainMenuSelection]);    
    // Calculate centered positions for selected item
    int selectedIconX = 35; // Center of screen
    int selectedTextX = 55; // Icon center + icon width/2 + spacing
    switch (mainMenuSelection) {
      case 0: // GPS
        selectedTextX = 70;
        selectedIconX = 50;
        break;
      case 1: // Musica
        selectedTextX = 55;
        selectedIconX = 35;
        break;
      case 2: // Monitoreo
        selectedTextX = 40;
        selectedIconX = 20;
        break;
      case 3: // Info
        selectedTextX = 65;
        selectedIconX = 45;
        break;
      case 4: // Ejercicio
        selectedTextX = 30;
        selectedIconX = 50;
        break;
      case 5: // Emergencia
        selectedTextX = 23;
        selectedIconX = 43;
        break;
    }
     
    // Icon for selected (on white bg) - centered
    drawMenuIcon(selectedIconX, 34, mainMenuSelection, true, ST7735_WHITE);
    write_str_bold(selectedTextX, 38, selectedText.c_str(), Font_11x18, ST7735_BLACK, ST7735_WHITE);

    // Dibujar opción de abajo (si existe)
    if (mainMenuSelection < 5) {
      int belowIdx = mainMenuSelection + 1;
      String belowText = String(menuItems[belowIdx]);
      // Calculate centered positions
      int belowIconX = 35; // Center of screen
      int belowTextX = 55; // Icon center + icon width/2 + spacing
      switch (belowIdx) {
        case 1: // Musica
          belowTextX = 68;
          belowIconX = 48;
          break;
        case 2: // Monitoreo
          belowTextX = 57;
          belowIconX = 37;
          break;
        case 3: // Info
          belowTextX = 73;
          belowIconX = 53;
          break;
        case 4: // Ejercicio
          belowTextX = 53;
          belowIconX = 33;
          break;
        case 5: // Emergencia
          belowTextX = 50;
          belowIconX = 30;
          break;
      }
       
      // Icon for below (on black bg) - centered
      drawMenuIcon(belowIconX, 61, belowIdx, false, ST7735_BLACK);
      st7735.st7735_write_str(belowTextX, 65, belowText.c_str(), Font_7x10, ST7735_GRAY, ST7735_BLACK);
    }

    lastMainMenuIdx = mainMenuSelection;
  }
}

void drawFolderScreen(bool firstDraw) {
  static int lastFolderLocal = 0;
  if (firstDraw) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    drawMenuDots(SCREEN_MP3_FOLDER);
    drawRoundedRectangle(30, 20, 100, 30, 4, MORADO);
    st7735.st7735_write_str(45, 31, "PLAYLIST", Font_7x10, ST7735_WHITE, MORADO);
    String folderNum = String(currentFolder);
    st7735.st7735_write_str(110, 26, folderNum.c_str(), Font_11x18, ST7735_WHITE, MORADO);
    st7735.st7735_write_str(15, 58, "Select para iniciar", Font_7x10, ST7735_WHITE);
    lastFolderLocal = currentFolder;
  } else if (lastFolderLocal != currentFolder) {
    drawRoundedRectangle(110, 28, 18, 18, 4, MORADO);
    String folderNum = String(currentFolder);
    st7735.st7735_write_str(110, 26, folderNum.c_str(), Font_11x18, ST7735_WHITE, MORADO);
    lastFolderLocal = currentFolder;
  }
}

void drawGPSScreen(bool firstDraw, bool updateLatitude, bool updateLongitude) {
  
  // Verificar si hay coordenadas GPS válidas usando la función mejorada
  bool gpsValid = isGPSValid();
  
  if (gpsValid) {
    if(firstDraw) {        
      st7735.st7735_fill_screen(ST7735_BLACK);

      st7735.st7735_write_str(0, 0, "GPS Dispositivo", Font_7x10, ST7735_WHITE);
      // Mostrar coordenadas disponibles
      st7735.st7735_write_str(0, 20, "Latitud:", Font_7x10, ST7735_GREEN);
      st7735.st7735_write_str(0, 35, latitude.c_str(), Font_7x10, ST7735_WHITE);
      
      st7735.st7735_write_str(0, 50, "Longitud:", Font_7x10, ST7735_GREEN);
      st7735.st7735_write_str(0, 65, longitude.c_str(), Font_7x10, ST7735_WHITE);
    }
    else {
      if(updateLatitude) {
        // Actualizar solo la latitud 
        st7735.st7735_fill_rectangle(0, 35, 128, 15, ST7735_BLACK);
        st7735.st7735_write_str(0, 35, latitude.c_str(), Font_7x10, ST7735_WHITE);
      }
      if(updateLongitude) {
        // Actualizar solo la longitud
        st7735.st7735_fill_rectangle(0, 65, 128, 15, ST7735_BLACK);
        st7735.st7735_write_str(0, 65, longitude.c_str(), Font_7x10, ST7735_WHITE);
      }
    }

    // Indicador de estado
    //st7735.st7735_write_str(0, 80, "Estado: Conectado", Font_7x10, ST7735_GREEN);
  } else {
    st7735.st7735_fill_screen(ST7735_BLACK);      
    st7735.st7735_write_str(0, 0, "GPS Dispositivo", Font_7x10, ST7735_WHITE);

    // Mostrar mensaje de no disponible
    st7735.st7735_write_str(0, 30, "GPS no disponible", Font_7x10, ST7735_RED);
    st7735.st7735_write_str(0, 45, "Esperando senal...", Font_7x10, ST7735_YELLOW);
    st7735.st7735_write_str(0, 60, "Estado: Desconectado", Font_7x10, ST7735_RED);
  }
}

void updateGPSFieldsIfChanged(const String& newTime, const String& newLat, const String& newLon) {
  if (currentScreen != SCREEN_GPS) return;
  
  // Verificar si las coordenadas han cambiado
  bool coordsChanged = (newLat != latitude || newLon != longitude);
  Serial.println("Coords changed: " + String(coordsChanged));
  
  if (coordsChanged) {
    // Redibujar toda la pantalla GPS con los nuevos datos
    drawGPSScreen(false, newLat != latitude, newLon != longitude);
    time_str = newTime;
    latitude = newLat;
    longitude = newLon;
  }
}

void updateGPSBatteryIndicator() {
  if (currentScreen != SCREEN_GPS) return;
  static int lastPercent = -1;
  if (batteryPercent == lastPercent) return;
  char voltStr[20];
  snprintf(voltStr, sizeof(voltStr), "Bat: %d%%", batteryPercent);
  // Limpiar solo el área de la batería (arriba derecha)
  st7735.st7735_fill_rectangle(80, 0, 48, 10, ST7735_BLACK);
  st7735.st7735_write_str(80, 0, voltStr, Font_7x10, ST7735_YELLOW);
  lastPercent = batteryPercent;
}

// MP3 UI (idéntico a tu implementación, con variables globales desde AppState)
void drawMP3Screen(bool firstDraw) {
  static uint8_t lastVolume = 255;
  static MP3Option lastOption = MP3_OPTION_COUNT;
  static bool lastPlayingState = !isPlaying;
  static uint8_t lastSong = 0;
  int mp3base = 36;
  int volBaseX = 30;
  int baseYcontrols = 30;
  int volBaseY = baseYcontrols + 33;

  if (firstDraw) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    lastVolume = 255;
    lastOption = MP3_OPTION_COUNT;
    lastPlayingState = !isPlaying;
    lastSong = 0;
    lastFolder = 0;
    drawMenuDots(SCREEN_MP3_PLAYER);
  }

  if (firstDraw || lastSong != currentSong || lastFolder != currentFolder) {
    st7735.st7735_fill_rectangle(36, 5, 92, 10, ST7735_BLACK);
    String trackText = String(currentFolder) + ". Track " + String(currentSong);
    st7735.st7735_write_str(36, 5, trackText.c_str(), Font_7x10, NARANJA);
    st7735.st7735_write_str(37, 5, trackText.c_str(), Font_7x10, NARANJA);
    lastSong = currentSong;
    lastFolder = currentFolder;
  }

  if (currentVolume != lastVolume || firstDraw) {
    const int cursorRadius = 3;
    const int volBarEndX = 127;
    st7735.st7735_fill_rectangle(volBaseX, volBaseY - 7, 97, 11, AZUL_OSCURO);
    for (int i = 0; i < 97; i++) {
      uint16_t color = (i < currentVolume * 3.23) ? NARANJA : ST7735_WHITE;
      for (int h = 0; h < 2; h++) st7735.st7735_draw_pixel(volBaseX + i, volBaseY + h, color);
    }
    int cursorX = map(currentVolume, 0, 30, volBaseX + cursorRadius, volBarEndX - cursorRadius);
    cursorX = constrain(cursorX, volBaseX + cursorRadius, volBarEndX - cursorRadius);
    for (int dy = -cursorRadius; dy <= cursorRadius; dy++) {
      for (int dx = -cursorRadius; dx <= cursorRadius; dx++) {
        if (dx*dx + dy*dy <= cursorRadius*cursorRadius) st7735.st7735_draw_pixel(cursorX + dx, volBaseY + dy, NARANJA);
      }
    }
    lastVolume = currentVolume;
  }

  if (currentMP3Option != lastOption || isPlaying != lastPlayingState || firstDraw) {
    st7735.st7735_fill_rectangle(mp3base + 24, baseYcontrols - 5, 30, 30, ST7735_BLACK);
    uint16_t playPauseCircleColor = (currentMP3Option == MP3_PLAY_PAUSE) ? MORADO : ST7735_WHITE;
    if (isPlaying) drawPauseIcon(mp3base + 41, baseYcontrols + 10, ST7735_BLACK, 12, playPauseCircleColor, ST7735_BLACK);
    else drawPlayIcon(mp3base + 41, baseYcontrols + 10, ST7735_BLACK, 12, playPauseCircleColor, ST7735_BLACK);
    st7735.st7735_fill_rectangle(mp3base - 10, baseYcontrols + 5, 16, 16, ST7735_BLACK);
    drawPrevIcon(mp3base, baseYcontrols + 5, currentMP3Option == MP3_PREV ? MORADO : ST7735_WHITE);
    st7735.st7735_fill_rectangle(mp3base + 70, baseYcontrols + 5, 16, 16, ST7735_BLACK);
    drawNextIcon(mp3base + 76, baseYcontrols + 5, currentMP3Option == MP3_NEXT ? MORADO : ST7735_WHITE);
    st7735.st7735_fill_rectangle(mp3base - 22, volBaseY - 4, 16, 16, AZUL_OSCURO);
    drawMinusIcon(mp3base - 22, volBaseY - 4, currentMP3Option == MP3_VOL_DOWN ? MORADO : ST7735_WHITE, 8);
    st7735.st7735_fill_rectangle(mp3base + 104, volBaseY - 4, 16, 16, AZUL_OSCURO);
    drawPlusIcon(mp3base + 104, volBaseY - 4, currentMP3Option == MP3_VOL_UP ? MORADO : ST7735_WHITE, 8);
    lastOption = currentMP3Option;
    lastPlayingState = isPlaying;
  }
}

void drawMonitoringScreen() {
  st7735.st7735_fill_screen(ST7735_BLACK);
  st7735.st7735_write_str(0, 0, "Monitoreo:", Font_7x10, ST7735_WHITE);
  const char* statusText = isMonitoringActive ? "Activo" : "No Activo";
  st7735.st7735_write_str(70, 0, statusText, Font_7x10, isMonitoringActive ? ST7735_GREEN : ST7735_RED);
  int y_offset = 15;
  int current_y = 15;
  st7735.st7735_write_str(0, current_y, "Impacto:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, impacto ? "SI" : "NO", Font_7x10, impacto ? ST7735_GREEN : ST7735_RED);
  current_y += y_offset;
  st7735.st7735_write_str(0, current_y, "CaidaLibre:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, free_fall ? "SI" : "NO", Font_7x10, free_fall ? ST7735_GREEN : ST7735_RED);
  current_y += y_offset;
  st7735.st7735_write_str(0, current_y, "CaidaConf:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, segunda_condicion_caida ? "SI" : "NO", Font_7x10, segunda_condicion_caida ? ST7735_GREEN : ST7735_RED);
  current_y += y_offset;
  st7735.st7735_write_str(0, current_y, "Emergencia:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, emergencia ? "SI" : "NO", Font_7x10, emergencia ? ST7735_GREEN : ST7735_RED);
  drawMenuDots(SCREEN_MONITORING);
}

void drawInfoScreen() {
  st7735.st7735_fill_screen(ST7735_BLACK);
  st7735.st7735_write_str(0, 0, "SafeRun Tracker", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(0, 15, "Dispositivo Remoto", Font_7x10, ST7735_GREEN);
  
  // Información del sistema
  st7735.st7735_write_str(0, 35, "Bateria:", Font_7x10, ST7735_WHITE);
  char batStr[10];
  snprintf(batStr, sizeof(batStr), "%d%%", batteryPercent);
  st7735.st7735_write_str(60, 35, batStr, Font_7x10, ST7735_YELLOW);
  
  st7735.st7735_write_str(0, 50, "Estado:", Font_7x10, ST7735_WHITE);
  const char* statusText = isMonitoringActive ? "Activo" : "Inactivo";
  uint16_t statusColor = isMonitoringActive ? ST7735_GREEN : ST7735_RED;
  st7735.st7735_write_str(50, 50, statusText, Font_7x10, statusColor);
  
  // Instrucciones
  st7735.st7735_write_str(0, 70, "Long Sel: Menu", Font_7x10, ST7735_GRAY);
}

// ====== Nuevas pantallas ======
static unsigned long fallPopupEndMs = 0;
static bool fallPopupActive = false;
static unsigned long getExerciseElapsed(unsigned long now) {
  if (!isMonitoringActive) return exercisePausedAccumMs;
  return exercisePausedAccumMs + (now - exerciseStartMs);
}

void drawExerciseScreen(bool firstDraw) {
  if (firstDraw) {
    drawHeaderWithWiFi("EJERCICIO");
    st7735.st7735_fill_screen(ST7735_BLACK);
  
  }
  unsigned long now = millis();
  //st7735.st7735_write_str(0, 16, isMonitoringActive ? "Estado: Grabando" : "Estado: En pausa", Font_7x10, isMonitoringActive ? ST7735_GREEN : ST7735_GRAY);
  drawRoundedRectangle(30, 20, 100, 30, 4, isMonitoringActive ? ST7735_GREEN : ST7735_GRAY);
  unsigned long elapsed = getExerciseElapsed(now);
  unsigned long sec = elapsed / 1000; unsigned int hh = sec / 3600; sec %= 3600; unsigned int mm = sec / 60; unsigned int ss = sec % 60;
  char tbuf[24]; snprintf(tbuf, sizeof(tbuf), "%02u:%02u:%02u", hh, mm, ss);
  st7735.st7735_write_str(45, 31, tbuf, Font_7x10, ST7735_WHITE, isMonitoringActive ? ST7735_GREEN : ST7735_GRAY);
  
  //st7735.st7735_write_str(0, 28, tbuf, Font_7x10, ST7735_WHITE);
  char dbuf[24]; snprintf(dbuf, sizeof(dbuf), "Dist: %.1f m", exerciseDistanceMeters);
  st7735.st7735_write_str(40, 60, dbuf, Font_7x10, ST7735_WHITE);
  //st7735.st7735_write_str(0, 70, "Select: Iniciar/Detener", Font_7x10, NARANJA);
  if (fallPopupActive) {
    long remaining = (long)(fallPopupEndMs - now);
    if (remaining < 0) remaining = 0;
    int seconds = (int)((remaining + 999) / 1000);
    st7735.st7735_fill_rectangle(0, 18, 160, 52, ST7735_BLACK);
    st7735.st7735_write_str(0, 22, "Caida detectada", Font_7x10, ST7735_RED);
    st7735.st7735_write_str(0, 34, "Cancelar: Izquierda", Font_7x10, ST7735_WHITE);
    char buf[16]; snprintf(buf, sizeof(buf), "SOS en %ds", seconds);
    st7735.st7735_write_str(0, 46, buf, Font_7x10, ST7735_RED);
  }
}

void drawEmergencyScreen(bool firstDraw) {
  if (firstDraw) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    drawHeaderWithWiFi("EMERGENCIA");
    
  }
  if (!emergencia) {
    st7735.st7735_write_str(0, 20, "Mantener 3 botones 3s", Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(0, 32, "para enviar SOS", Font_7x10, ST7735_WHITE);
  } else {
    st7735.st7735_write_str(0, 20, "\xC2\xA1SOS Enviado!", Font_11x18, ST7735_RED);
    if (emergencyConfirmDeactivate) {
      st7735.st7735_write_str(0, 44, "Mantener 3 botones + Sel", Font_7x10, ST7735_WHITE);
      st7735.st7735_write_str(0, 56, "para apagar", Font_7x10, ST7735_WHITE);
    } else {
      st7735.st7735_write_str(0, 44, "Mantener 3 botones 3s", Font_7x10, ST7735_WHITE);
      st7735.st7735_write_str(0, 56, "para pedir apagar", Font_7x10, ST7735_WHITE);
    }
  }
}

void notifyFallDetected() {
  fallPopupActive = true;
  fallPopupEndMs = millis() + 10000;
  currentScreen = SCREEN_EXERCISE;
  drawExerciseScreen(true);
}

// Iconos (copiados de tu implementación)
void drawMinusIcon(uint16_t x, uint16_t y, uint16_t color, uint8_t size) {
  uint16_t lineLength = size;
  uint16_t thickness = (size > 6) ? 2 : 1;
  uint16_t startX = x;
  uint16_t centerY = y + size / 2;
  uint16_t startY = centerY - thickness / 2;
  for (uint8_t h = 0; h < thickness; ++h) {
    for (uint8_t w = 0; w < lineLength; ++w) {
      st7735.st7735_draw_pixel(startX + w, startY + h, color);
    }
  }
}

void drawPlusIcon(uint16_t x, uint16_t y, uint16_t color, uint8_t size) {
  uint16_t lineLength = size;
  uint16_t thickness = (size > 6) ? 2 : 1;
  uint16_t centerX = x + size / 2;
  uint16_t centerY = y + size / 2;
  uint16_t halfLength = lineLength / 2;
  uint16_t hStartX = centerX - halfLength;
  uint16_t hStartY = centerY - thickness / 2;
  for (uint8_t h = 0; h < thickness; ++h) {
    for (uint8_t w = 0; w < lineLength; ++w) st7735.st7735_draw_pixel(hStartX + w, hStartY + h, color);
  }
  uint16_t vStartX = centerX - thickness / 2;
  uint16_t vStartY = centerY - halfLength;
  for (uint8_t w = 0; w < thickness; ++w) {
    for (uint8_t h = 0; h < lineLength; ++h) st7735.st7735_draw_pixel(vStartX + w, vStartY + h, color);
  }
}

void drawPlayIcon(uint16_t x, uint16_t y, uint16_t borderColor, uint16_t radius, uint16_t circleColor, uint16_t fillColor) {
  for (int dy = -radius; dy <= radius; dy++)
    for (int dx = -radius; dx <= radius; dx++)
      if (dx*dx + dy*dy <= radius*radius) st7735.st7735_draw_pixel(x + dx, y + dy, circleColor);
  const int totalHeight = radius;
  const int iconHalfHeight = totalHeight/2;
  const int iconMaxWidth = radius/1.2;
  const int borderThickness = 1;
  const int max_i_val = iconHalfHeight - 1;
  int triX = x - iconMaxWidth/3;
  int triY = y - totalHeight/2;
  for (int i = 0; i < iconHalfHeight; i++) {
    int j_limit = (i * (iconMaxWidth - 1)) / max_i_val;
    for (int j = 0; j <= j_limit; j++) {
      bool isBorder = (j < borderThickness || j > j_limit - borderThickness);
      uint16_t currentPixelColor = isBorder ? borderColor : fillColor;
      st7735.st7735_draw_pixel(triX + j, triY + i, currentPixelColor);
      st7735.st7735_draw_pixel(triX + j, triY + (totalHeight - 1) - i, currentPixelColor);
    }
  }
}

void drawPauseIcon(uint16_t x, uint16_t y, uint16_t borderColor, uint16_t radius, uint16_t circleColor, uint16_t fillColor) {
  for (int dy = -radius; dy <= radius; dy++)
    for (int dx = -radius; dx <= radius; dx++)
      if (dx*dx + dy*dy <= radius*radius) st7735.st7735_draw_pixel(x + dx, y + dy, circleColor);
  const int barHeight = radius * 1.2;
  const int barWidth = radius / 3;
  const int barSpacing = radius / 3;
  const int totalWidth = barWidth * 2 + barSpacing;
  int topY = y - barHeight / 2;
  int leftBarCenterX = x - totalWidth / 2 + barWidth / 2;
  int rightBarCenterX = x + totalWidth / 2 - barWidth / 2;
  int leftBarStartX = leftBarCenterX - barWidth / 2;
  int rightBarStartX = rightBarCenterX - barWidth / 2;
  st7735.st7735_fill_rectangle(leftBarStartX, topY, barWidth, barHeight, fillColor);
  st7735.st7735_fill_rectangle(rightBarStartX, topY, barWidth, barHeight, fillColor);
}

void drawPrevIcon(uint16_t x, uint16_t y, uint16_t color) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j <= i; j++) {
      st7735.st7735_draw_pixel(x + 8 - j, y + i, color);
      st7735.st7735_draw_pixel(x + 8 - j, y + 15 - i, color);
    }
  }
  for (int i = 0; i < 1; i++)
    for (int j = 0; j < 16; j++) st7735.st7735_draw_pixel(x - i, y + j, color);
}

void drawNextIcon(uint16_t x, uint16_t y, uint16_t color) {
  for (int i = 0; i < 8; i++)
    for (int j = 0; j <= i; j++) {
      st7735.st7735_draw_pixel(x + j, y + i, color);
      st7735.st7735_draw_pixel(x + j, y + 15 - i, color);
    }
  for (int i = 0; i < 16; i++) st7735.st7735_draw_pixel(x + 8, y + i, color);
}

void drawVolDownIcon(uint16_t x, uint16_t y, uint16_t color) {
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8 - i; j++) st7735.st7735_draw_pixel(x + j + i / 2, y + i + 4, color);
}

void drawVolUpIcon(uint16_t x, uint16_t y, uint16_t color) {
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8 - i; j++) st7735.st7735_draw_pixel(x + j + i / 2, y + 8 - i, color);
}

void drawRoundedRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t radius, uint16_t color) {
  for (uint16_t i = radius; i < width - radius; i++)
    for (uint16_t j = 0; j < height; j++) st7735.st7735_draw_pixel(x + i, y + j, color);
  for (uint16_t i = radius; i < height - radius; i++) {
    for (uint16_t j = 0; j < radius; j++) st7735.st7735_draw_pixel(x + j, y + i, color);
    for (uint16_t j = 0; j < radius; j++) st7735.st7735_draw_pixel(x + width - j - 1, y + i, color);
  }
  for (uint16_t i = 0; i <= radius; i++)
    for (uint16_t j = 0; j <= radius; j++) if ((i * i + j * j) <= (radius * radius)) {
      st7735.st7735_draw_pixel(x + radius - i, y + radius - j, color);
      st7735.st7735_draw_pixel(x + width - radius + i - 1, y + radius - j, color);
      st7735.st7735_draw_pixel(x + radius - i, y + height - radius + j - 1, color);
      st7735.st7735_draw_pixel(x + width - radius + i - 1, y + height - radius + j - 1, color);
    }
}

// Funciones auxiliares para el menú principal (copiadas del dispositivo local)
void drawLine(int x1, int y1, int x2, int y2, uint16_t color) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;
  
  while (true) {
    st7735.st7735_draw_pixel(x1, y1, color);
    if (x1 == x2 && y1 == y2) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x1 += sx; }
    if (e2 < dx) { err += dx; y1 += sy; }
  }
}

void drawCircle(int x, int y, int radius, uint16_t color) {
  for (int dy = -radius; dy <= radius; dy++) {
    for (int dx = -radius; dx <= radius; dx++) {
      if (dx*dx + dy*dy <= radius*radius) {
        st7735.st7735_draw_pixel(x + dx, y + dy, color);
      }
    }
  }
}

void drawCircleOutline(int xc, int yc, int r, uint16_t color) {
    int x = r, y = 0;
    int P = 1 - r;
    while (x > y) {
        y++;
        if (P <= 0) P = P + 2*y + 1;
        else { x--; P = P + 2*y - 2*x + 1; }
        if (x < y) break;
        st7735.st7735_draw_pixel(xc + x, yc + y, color); st7735.st7735_draw_pixel(xc - x, yc + y, color);
        st7735.st7735_draw_pixel(xc + x, yc - y, color); st7735.st7735_draw_pixel(xc - x, yc - y, color);
        if (x != y) {
            st7735.st7735_draw_pixel(xc + y, yc + x, color); st7735.st7735_draw_pixel(xc - y, yc + x, color);
            st7735.st7735_draw_pixel(xc + y, yc - x, color); st7735.st7735_draw_pixel(xc - y, yc - x, color);
        }
    }
}

void fillRectPixels(int x, int y, int w, int h, uint16_t color) {
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      st7735.st7735_draw_pixel(x + xx, y + yy, color);
    }
  }
}

void drawHeaderWithWiFi(const String &title) {
  st7735.st7735_fill_screen(ST7735_BLACK);
  
  // Draw WiFi icon in top right as status bar
  bool wifiStatus = false; // No WiFi en dispositivo remoto
  drawWiFiIcon(140, 1, wifiStatus);
  
  // Draw title
  st7735.st7735_write_str(0, 0, title.c_str(), Font_7x10, ST7735_WHITE);
}

void drawWiFiIcon(int x, int y, bool connected) {
  if (connected) {
    // Connected WiFi icon (larger bars)
    for (int b = 0; b < 4; b++) {
      uint16_t barColor = ST7735_GREEN;
      int barHeight = (b + 1) * 2;
      int barY = y + 15 - barHeight;
      drawLine(x + b*3, barY, x + b*3, barY + barHeight, barColor);
    }
  } else {
    // Disconnected WiFi icon (larger red X)
    drawLine(x + 2, y + 2, x + 10, y + 10, ST7735_RED);
    drawLine(x + 10, y + 2, x + 2, y + 10, ST7735_RED);
  }
}

// Helper to draw slightly bolder text by overdrawing with 1px offset
void write_str_bold(uint16_t x, uint16_t y, const char* text, FontDef font, uint16_t color, uint16_t bgcolor) {
  st7735.st7735_write_str(x, y, text, font, color, bgcolor);
  st7735.st7735_write_str(x + 1, y, text, font, color, bgcolor);
}

// Función para dibujar iconos del menú principal
void drawMenuIcon(int x, int y, int itemIndex, bool selected, uint16_t bgcolor) {
  uint16_t fg = selected ? MORADO : ST7735_WHITE;
  // Clear icon area first to avoid artifacts - increased size to 18x18
  fillRectPixels(x, y, 18, 18, bgcolor);
  switch (itemIndex) {
    case 0: { // GPS: icono de satélite/ubicación
      // Círculo central
      drawCircle(x + 9, y + 9, 4, fg);
      // Líneas de señal
      drawLine(x + 9, y + 2, x + 9, y + 6, fg);
      drawLine(x + 9, y + 12, x + 9, y + 16, fg);
      drawLine(x + 2, y + 9, x + 6, y + 9, fg);
      drawLine(x + 12, y + 9, x + 16, y + 9, fg);
    } break;
    case 1: { // Musica: icono de altavoz/música
      // Base del altavoz
      drawLine(x + 3, y + 8, x + 3, y + 12, fg);
      drawLine(x + 3, y + 8, x + 8, y + 6, fg);
      drawLine(x + 3, y + 12, x + 8, y + 14, fg);
      // Ondas de sonido
      for (int i = 0; i < 3; i++) {
        int waveY = y + 6 + i * 2;
        int waveW = 4 + i * 2;
        drawLine(x + 8, waveY, x + 8 + waveW, waveY, fg);
      }
    } break;
    case 2: { // Monitoreo: icono de ojo/vigilancia
      // Contorno del ojo
      drawCircle(x + 9, y + 9, 6, fg);
      // Pupila
      drawCircle(x + 9, y + 9, 2, ST7735_BLACK);
      // Pestañas
      for (int i = 0; i < 3; i++) {
        drawLine(x + 6 + i*2, y + 3, x + 6 + i*2, y + 5, fg);
        drawLine(x + 6 + i*2, y + 13, x + 6 + i*2, y + 15, fg);
      }
    } break;
    case 3: { // Info: círculo con i
      drawCircle(x + 9, y + 9, 7, fg);
      drawLine(x + 9, y + 6, x + 9, y + 11, fg);
      st7735.st7735_draw_pixel(x + 9, y + 5, fg);
    } break;
  }
}


