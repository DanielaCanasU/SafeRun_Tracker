#include "Display.h"
#include "GPS.h"
#include "Accel.h"
#include "DFPlayerMod.h"
#include <SoftwareSerial.h>
#include <HardwareSerial.h>  //Puerto Serial
#include "AppState.h"
#include "Backtrack.h"

Display::Display() {
}

void Display::drawStaticWelcome() {
  // Pantalla de bienvenida (se llama solo la primera vez)
  st7735.st7735_fill_screen(ST7735_BLACK);

  // Logo principal (centrado arriba)
  st7735.st7735_write_str(40, 6, "SafeRun", Font_11x18, NARANJA);       // ancho aprox 80px
  st7735.st7735_write_str(55, 26, "Tracker", Font_7x10, ST7735_WHITE);  // subtítulo

  // Línea decorativa (centrada)
  drawLine(30, 38, 130, 38, NARANJA);

  // Mensaje de bienvenida (centrado)
  st7735.st7735_write_str(20, 48, "BIENVENIDOS", Font_11x18, NARANJA);

  // opcional: subtítulo o instrucción
  st7735.st7735_write_str(18, 68, "Iniciando sistema...", Font_7x10, ST7735_GRAY);

  // dejar marcado que la última pantalla no es el menú para forzar redraw al cambiar
  lastRenderedScreen = SCREEN_COUNT;
  lastMainMenuIdx = -1;
}

void Display::init() {
  st7735.st7735_init();
  // Si el MP3 no está listo dibujamos el splash; cuando se inicie el DFPlayer
  // Musica::init() llamará display.drawMenu(...) para la transición automática.
  if (!GPSLISTO) {
    drawStaticWelcome();
    Serial.println("--------------------------------");
    Serial.println("PANTALLA DE BIENVENIDA INICIADA");
    Serial.println("--------------------------------");
  } else {
    // Si ya está listo, dibujar menú completo
    this->drawMenu(SCREEN_MAIN_MENU, true);
    Serial.println("--------------------------------");
    Serial.println("PANTALLA INICIADO CORRECTAMENTE");
    Serial.println("--------------------------------");
  }
}

void Display::drawMenu(MenuScreen currentMenu, bool firstDraw) {
  if (currentMenu == SCREEN_MAIN_MENU) {
    if (firstDraw || lastRenderedScreen != SCREEN_MAIN_MENU) {
      // limpiar toda la pantalla antes de dibujar fondo/header del menú
      st7735.st7735_fill_screen(ST7735_BLACK);
      drawHeaderWithWiFi("Inicio");
      // Dibujar la barra blanca de selección UNA SOLA VEZ
      fillRectPixels(0, 34, 160, 26, ST7735_WHITE);
      lastRenderedScreen = SCREEN_MAIN_MENU;
      lastMainMenuIdx = -1;  // Forzar un redibujado completo de los items del menú
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
      fillRectPixels(20, 34, 120, 26, ST7735_WHITE);  // Limpia el centro de la barra blanca

      // --- Redibujado de items ---
      const char* menuItems[] = { "GPS", "Musica", "Monitoreo", "Info", "Ejercicio", "Emergencia", "Backtrack" };

      // Dibujar opción de arriba (si existe)
      if (mainMenuSelection > 0) {
        int aboveIdx = mainMenuSelection - 1;
        String aboveText = String(menuItems[aboveIdx]);
        // Calculate centered positions
        int iconX = 35;  // Center of screen (128/2 - 18/2 = 55)
        int textX = 55;  // Icon center + icon width/2 + spacing
        switch (aboveIdx) {
          case 0:  // GPS
            textX = 80;
            iconX = 55;
            break;
          case 1:  // Musica
            textX = 68;
            iconX = 48;
            break;
          case 2:  // Monitoreo
            textX = 57;
            iconX = 37;
            break;
          case 3:  // Info
            textX = 75;
            iconX = 55;
            break;
          case 4:  // Ejercicio
            textX = 53;
            iconX = 33;
            break;
          case 5:  // Emergencia
            textX = 50;
            iconX = 30;
            break;
        }

        // Icon for above (on black bg) - centered
        drawMenuIcon(iconX, 15, aboveIdx, false, ST7735_BLACK);
        st7735.st7735_write_str(textX, 20, aboveText.c_str(), Font_7x10, ST7735_GRAY, ST7735_BLACK);
      }

      // Dibujar opción seleccionada (sobre la barra blanca ya existente)
      String selectedText = String(menuItems[mainMenuSelection]);
      // Calculate centered positions for selected item
      int selectedIconX = 35;  // Center of screen
      int selectedTextX = 55;  // Icon center + icon width/2 + spacing
      switch (mainMenuSelection) {
        case 0:  // GPS
          selectedTextX = 70;
          selectedIconX = 50;
          break;
        case 1:  // Musica
          selectedTextX = 55;
          selectedIconX = 35;
          break;
        case 2:  // Monitoreo
          selectedTextX = 40;
          selectedIconX = 20;
          break;
        case 3:  // Info
          selectedTextX = 65;
          selectedIconX = 45;
          break;
        case 4:  // Ejercicio
          selectedTextX = 30;
          selectedIconX = 50;
          break;
        case 5:  // Emergencia
          selectedTextX = 23;
          selectedIconX = 43;
          break;
          case 6:  // Backtrack
            selectedTextX = 35;
            selectedIconX = 15;
            break;
          case 7:  // Config Dist
            selectedTextX = 20;
            selectedIconX = 0;
            break;
      }

      // Icon for selected (on white bg) - centered
      drawMenuIcon(selectedIconX, 34, mainMenuSelection, true, ST7735_WHITE);
      write_str_bold(selectedTextX, 38, selectedText.c_str(), Font_11x18, ST7735_BLACK, ST7735_WHITE);

      // Dibujar opción de abajo (si existe)
      if (mainMenuSelection < 7) {
        int belowIdx = mainMenuSelection + 1;
        String belowText = String(menuItems[belowIdx]);
        // Calculate centered positions
        int belowIconX = 35;  // Center of screen
        int belowTextX = 55;  // Icon center + icon width/2 + spacing
        switch (belowIdx) {
          case 1:  // Musica
            belowTextX = 68;
            belowIconX = 48;
            break;
          case 2:  // Monitoreo
            belowTextX = 57;
            belowIconX = 37;
            break;
          case 3:  // Info
            belowTextX = 73;
            belowIconX = 53;
            break;
          case 4:  // Ejercicio
            belowTextX = 53;
            belowIconX = 33;
            break;
          case 5:  // Emergencia
            belowTextX = 50;
            belowIconX = 30;
            break;
          case 6:  // Backtrack
            belowTextX = 40;
            belowIconX = 20;
            break;
          case 7:  // Config Dist
            belowTextX = 25;
            belowIconX = 5;
            break;
        }

        // Icon for below (on black bg) - centered
        drawMenuIcon(belowIconX, 61, belowIdx, false, ST7735_BLACK);
        st7735.st7735_write_str(belowTextX, 65, belowText.c_str(), Font_7x10, ST7735_GRAY, ST7735_BLACK);
      }

      lastMainMenuIdx = mainMenuSelection;
    }
  }
}



void drawMenuDots(MenuScreen currentMenu) {
  const int dotRadius = 1;
  const int activeDotRadius = 2;
  const int dotSpacing = 12;
  const int yPosition = 73;
  const int xStart = 80 - dotSpacing / 2;
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
        if (dx * dx + dy * dy <= radius * radius) st7735.st7735_draw_pixel(xPos + dx, yPosition + dy, color);
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
    lastMainMenuIdx = -1;  // Forzar un redibujado completo de los items del menú
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
    fillRectPixels(20, 34, 120, 26, ST7735_WHITE);  // Limpia el centro de la barra blanca

      // --- Redibujado de items ---
      const char* menuItems[] = { "GPS", "Musica", "Monitoreo", "Info", "Ejercicio", "Emergencia", "Backtrack", "Config Dist" };

    // Dibujar opción de arriba (si existe)
    if (mainMenuSelection > 0) {
      int aboveIdx = mainMenuSelection - 1;
      String aboveText = String(menuItems[aboveIdx]);
      // Calculate centered positions
      int iconX = 35;  // Center of screen (128/2 - 18/2 = 55)
      int textX = 55;  // Icon center + icon width/2 + spacing
      switch (aboveIdx) {
        case 0:  // GPS
          textX = 80;
          iconX = 55;
          break;
        case 1:  // Musica
          textX = 68;
          iconX = 48;
          break;
        case 2:  // Monitoreo
          textX = 57;
          iconX = 37;
          break;
        case 3:  // Info
          textX = 75;
          iconX = 55;
          break;
        case 4:  // Ejercicio
          textX = 53;
          iconX = 33;
          break;
        case 5:  // Emergencia
          textX = 50;
          iconX = 30;
          break;
        case 6:  // Backtrack
          textX = 40;
          iconX = 20;
          break;
        case 7:  // Config Dist
          textX = 25;
          iconX = 5;
          break;
      }

      // Icon for above (on black bg) - centered
      drawMenuIcon(iconX, 15, aboveIdx, false, ST7735_BLACK);
      st7735.st7735_write_str(textX, 20, aboveText.c_str(), Font_7x10, ST7735_GRAY, ST7735_BLACK);
    }

    // Dibujar opción seleccionada (sobre la barra blanca ya existente)
    String selectedText = String(menuItems[mainMenuSelection]);
    // Calculate centered positions for selected item
    int selectedIconX = 35;  // Center of screen
    int selectedTextX = 55;  // Icon center + icon width/2 + spacing
    switch (mainMenuSelection) {
      case 0:  // GPS
        selectedTextX = 70;
        selectedIconX = 50;
        break;
      case 1:  // Musica
        selectedTextX = 55;
        selectedIconX = 35;
        break;
      case 2:  // Monitoreo
        selectedTextX = 40;
        selectedIconX = 20;
        break;
      case 3:  // Info
        selectedTextX = 65;
        selectedIconX = 45;
        break;
      case 4:  // Ejercicio
        selectedTextX = 30;
        selectedIconX = 50;
        break;
      case 5:  // Emergencia
        selectedTextX = 23;
        selectedIconX = 43;
        break;
      case 6:  // Backtrack
        selectedTextX = 35;
        selectedIconX = 15;
        break;
      case 7:  // Config Dist
        selectedTextX = 20;
        selectedIconX = 0;
        break;
    }

    // Icon for selected (on white bg) - centered
    drawMenuIcon(selectedIconX, 34, mainMenuSelection, true, ST7735_WHITE);
    write_str_bold(selectedTextX, 38, selectedText.c_str(), Font_11x18, ST7735_BLACK, ST7735_WHITE);

    // Dibujar opción de abajo (si existe)
    if (mainMenuSelection < 7) {
      int belowIdx = mainMenuSelection + 1;
      String belowText = String(menuItems[belowIdx]);
      // Calculate centered positions
      int belowIconX = 35;  // Center of screen
      int belowTextX = 55;  // Icon center + icon width/2 + spacing
      switch (belowIdx) {
        case 1:  // Musica
          belowTextX = 68;
          belowIconX = 48;
          break;
        case 2:  // Monitoreo
          belowTextX = 57;
          belowIconX = 37;
          break;
        case 3:  // Info
          belowTextX = 73;
          belowIconX = 53;
          break;
        case 4:  // Ejercicio
          belowTextX = 53;
          belowIconX = 33;
          break;
        case 5:  // Emergencia
          belowTextX = 50;
          belowIconX = 30;
          break;
        case 6:  // Backtrack
          belowTextX = 40;
          belowIconX = 20;
          break;
        case 7:  // Config Dist
          belowTextX = 25;
          belowIconX = 5;
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
  musica.init();
  static int lastFolderLocal = 0;
  
  if (firstDraw) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    drawMenuDots(SCREEN_MP3_FOLDER);
    // Mostrar símbolo de desconexión si es necesario
    if (diferencia > 10 && disconnectedScreenShown) {
      drawDisconnectedIcon(120, 1);
    }
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
  static bool firstValidGps = false;

  if (gpsValid) {
    if (firstDraw || !firstValidGps) {
      st7735.st7735_fill_screen(ST7735_BLACK);

      st7735.st7735_write_str(0, 0, "GPS Dispositivo", Font_7x10, ST7735_WHITE);
      // Mostrar símbolo de desconexión si es necesario
      if (diferencia > 10 && disconnectedScreenShown) {
        drawDisconnectedIcon(120, 1);
      }
      // Mostrar coordenadas disponibles
      st7735.st7735_write_str(0, 20, "Latitud:", Font_7x10, ST7735_GREEN);
      st7735.st7735_write_str(0, 35, latitude.c_str(), Font_7x10, ST7735_WHITE);

      st7735.st7735_write_str(0, 50, "Longitud:", Font_7x10, ST7735_GREEN);
      st7735.st7735_write_str(0, 65, longitude.c_str(), Font_7x10, ST7735_WHITE);
    } else {
      firstValidGps = true;
      if (updateLatitude) {
        // Actualizar solo la latitud
        st7735.st7735_fill_rectangle(0, 35, 128, 15, ST7735_BLACK);
        st7735.st7735_write_str(0, 35, latitude.c_str(), Font_7x10, ST7735_WHITE);
      }
      if (updateLongitude) {
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
    // Mostrar símbolo de desconexión si es necesario
    if (diferencia > 10 && disconnectedScreenShown) {
      drawDisconnectedIcon(120, 1);
    }
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
        if (dx * dx + dy * dy <= cursorRadius * cursorRadius) st7735.st7735_draw_pixel(cursorX + dx, volBaseY + dy, NARANJA);
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
  // Mostrar símbolo de desconexión si es necesario
  if (diferencia > 10 && disconnectedScreenShown) {
    drawDisconnectedIcon(120, 1);
  }
  const char* statusText = isMonitoringActive ? "Activo" : "No Activo";
  st7735.st7735_write_str(70, 0, statusText, Font_7x10, isMonitoringActive ? ST7735_GREEN : ST7735_RED);
  int y_offset = 15;
  int current_y = 15;
  st7735.st7735_write_str(0, current_y, "Impacto:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, detectorCaida.impacto ? "SI" : "NO", Font_7x10, detectorCaida.impacto ? ST7735_GREEN : ST7735_RED);
  current_y += y_offset;
  st7735.st7735_write_str(0, current_y, "CaidaLibre:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, detectorCaida.free_fall ? "SI" : "NO", Font_7x10, detectorCaida.free_fall ? ST7735_GREEN : ST7735_RED);
  current_y += y_offset;
  st7735.st7735_write_str(0, current_y, "CaidaConf:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, detectorCaida.segunda_condicion_caida ? "SI" : "NO", Font_7x10, detectorCaida.segunda_condicion_caida ? ST7735_GREEN : ST7735_RED);
  current_y += y_offset;
  st7735.st7735_write_str(0, current_y, "Emergencia:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, detectorCaida.emergencia ? "SI" : "NO", Font_7x10, detectorCaida.emergencia ? ST7735_GREEN : ST7735_RED);
  drawMenuDots(SCREEN_MONITORING);
}

void drawInfoScreen(bool firstDraw) {
  static int lastDiferencia = -1;
  static bool lastShouldShowIcon = false;
  static MenuScreen lastScreen = SCREEN_COUNT;
  
  // Verificar si es la primera vez que se dibuja esta pantalla o si se cambió de pantalla
  // Usar currentScreen para detectar cuando se entra a esta pantalla desde otra
  bool screenChanged = (lastScreen != SCREEN_INFO && currentScreen == SCREEN_INFO);
  bool isFirstDraw = firstDraw || screenChanged;
  
  // Verificar si diferencia cambió
  bool diferenciaChanged = (diferencia != lastDiferencia);
  
  // Verificar si el estado del símbolo de desconexión cambió
  bool shouldShowIcon = (diferencia > 3 && disconnectedScreenShown);
  bool disconnectedIconChanged = (shouldShowIcon != lastShouldShowIcon);
  
  // Solo procesar si estamos en la pantalla de info
  if (currentScreen == SCREEN_INFO) {
    if (isFirstDraw || diferenciaChanged || disconnectedIconChanged) {
      if (isFirstDraw) {
        st7735.st7735_fill_screen(ST7735_BLACK);
        st7735.st7735_write_str(0, 0, "SafeRun Tracker", Font_7x10, ST7735_WHITE);
        st7735.st7735_write_str(0, 15, "Dispositivo Remoto", Font_7x10, ST7735_GREEN);
        st7735.st7735_write_str(0, 30, "No delivered", Font_7x10, ST7735_WHITE);
        st7735.st7735_write_str(0, 50, "Estado:", Font_7x10, ST7735_WHITE);
        st7735.st7735_write_str(0, 70, "Long Sel: Menu", Font_7x10, ST7735_GRAY);
      }
      
      // Actualizar símbolo de desconexión si cambió o es primera vez
      if (isFirstDraw || disconnectedIconChanged) {
        // Limpiar área del icono
        st7735.st7735_fill_rectangle(120, 1, 15, 15, ST7735_BLACK);
        if (shouldShowIcon) {
          drawDisconnectedIcon(120, 1);
        }
        lastShouldShowIcon = shouldShowIcon;
      }
      
      // Actualizar valor de diferencia si cambió o es primera vez
      if (isFirstDraw || diferenciaChanged) {
        // Limpiar área del valor de diferencia
        st7735.st7735_fill_rectangle(90, 30, 40, 10, ST7735_BLACK);
        String stringDiferencia = String(diferencia);
        st7735.st7735_write_str(90, 30, stringDiferencia.c_str(), Font_7x10, ST7735_RED);
        lastDiferencia = diferencia;
      }
      
      // Actualizar estado de monitoreo (siempre en primera vez)
      if (isFirstDraw) {
        const char* statusText = isMonitoringActive ? "Activo" : "Inactivo";
        uint16_t statusColor = isMonitoringActive ? ST7735_GREEN : ST7735_RED;
        st7735.st7735_write_str(50, 50, statusText, Font_7x10, statusColor);
      }
    }
    
    // Actualizar lastScreen cuando estamos en esta pantalla
    lastScreen = SCREEN_INFO;
  } else {
    // Si salimos de la pantalla de info, actualizar lastScreen
    if (lastScreen == SCREEN_INFO) {
      lastScreen = currentScreen;
    }
  }
}

// ====== Nuevas pantallas ======
static unsigned long fallPopupEndMs = 0;
static bool fallPopupActive = false;

void drawExerciseScreen(bool firstDraw) {
  if (firstDraw) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    //drawHeaderWithWiFi("EJERCICIO");
    st7735.st7735_write_str(45, 5, "EJERCICIO", Font_7x10, ST7735_WHITE);
    // Mostrar símbolo de desconexión si es necesario
    if (diferencia > 10 && disconnectedScreenShown) {
      drawDisconnectedIcon(120, 1);
    }
  }
  unsigned long now = millis();
  //st7735.st7735_write_str(0, 16, isMonitoringActive ? "Estado: Grabando" : "Estado: En pausa", Font_7x10, isMonitoringActive ? ST7735_GREEN : ST7735_GRAY);
  drawRoundedRectangle(20, 20, 115, 35, 4, isMonitoringActive ? ST7735_GREEN : ST7735_GRAY);
  unsigned long elapsed = getExerciseElapsed(now);
  unsigned long sec = elapsed / 1000;
  unsigned int hh = sec / 3600;
  sec %= 3600;
  unsigned int mm = sec / 60;
  unsigned int ss = sec % 60;
  char tbuf[24];
  snprintf(tbuf, sizeof(tbuf), "%02u:%02u:%02u", hh, mm, ss);
  st7735.st7735_write_str(35, 28, tbuf, Font_11x18, isMonitoringActive ? ST7735_BLACK : ST7735_WHITE, isMonitoringActive ? ST7735_GREEN : ST7735_GRAY);

  //st7735.st7735_write_str(0, 28, tbuf, Font_7x10, ST7735_WHITE);
  char dbuf[24];
  snprintf(dbuf, sizeof(dbuf), "Dist: %.1f m", exerciseDistanceMeters);
  st7735.st7735_write_str(40, 60, dbuf, Font_7x10, ST7735_WHITE);
  //st7735.st7735_write_str(0, 70, "Select: Iniciar/Detener", Font_7x10, NARANJA);
  if (fallPopupActive) {
    long remaining = (long)(fallPopupEndMs - now);
    if (remaining < 0) remaining = 0;
    int seconds = (int)((remaining + 999) / 1000);
    st7735.st7735_fill_rectangle(0, 18, 160, 52, ST7735_BLACK);
    st7735.st7735_write_str(0, 22, "Caida detectada", Font_7x10, ST7735_RED);
    st7735.st7735_write_str(0, 34, "Cancelar: Izquierda", Font_7x10, ST7735_WHITE);
    char buf[16];
    snprintf(buf, sizeof(buf), "SOS en %ds", seconds);
    st7735.st7735_write_str(0, 46, buf, Font_7x10, ST7735_RED);
  }
}

void drawEmergencyScreen(bool firstDraw) {
  if (firstDraw) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    st7735.st7735_write_str(45, 5, "EMERGENCIA", Font_7x10, ST7735_WHITE);
  }
  if (!detectorCaida.emergencia) {
    drawRoundedRectangle(22, 20, 115, 30, 4, ST7735_GREEN);
    st7735.st7735_write_str(40, 28, "A SALVO", Font_11x18, ST7735_BLACK, ST7735_GREEN);
    st7735.st7735_write_str(5, 60, "Mantener 3 botones 3s", Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(25, 70, "para enviar SOS", Font_7x10, ST7735_WHITE);
  } else {
    drawRoundedRectangle(22, 20, 115, 30, 4, ST7735_RED);
    st7735.st7735_write_str(65, 28, "SOS", Font_11x18, ST7735_BLACK, ST7735_RED);
    if (emergencyConfirmDeactivate) {
      st7735.st7735_write_str(35, 60, "Presionar Sel", Font_7x10, ST7735_WHITE);
      st7735.st7735_write_str(40, 70, "para apagar", Font_7x10, ST7735_WHITE);
    } else {
      st7735.st7735_write_str(5, 60, "Mantener 3 botones 3s", Font_7x10, ST7735_WHITE);
      st7735.st7735_write_str(25, 70, "para pedir apagar", Font_7x10, ST7735_WHITE);
    }
  }
}

void drawDisconnectedScreen(bool firstDraw) {
  if (firstDraw) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    st7735.st7735_write_str(40, 5, "DESCONECTADO", Font_7x10, ST7735_RED);
    
    // Dibujar un rectángulo con borde rojo para indicar alerta
    drawRoundedRectangle(10, 20, 140, 50, 4, ST7735_RED);
    
    // Mensaje principal
    st7735.st7735_write_str(41, 30, "Dispositivo", Font_7x10, ST7735_WHITE,ST7735_RED);
    st7735.st7735_write_str(39, 40, "desconectado", Font_7x10, ST7735_WHITE,ST7735_RED);
    st7735.st7735_write_str(39, 50, "del receptor", Font_7x10, ST7735_WHITE,ST7735_RED);
    

  }
}

void drawDistanceConfigScreen(bool firstDraw) {
  static MenuScreen lastScreen = SCREEN_COUNT;
  
  if (firstDraw || lastScreen != SCREEN_DISTANCE_CONFIG) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    drawHeaderWithWiFi("Config Distancia");
    lastScreen = SCREEN_DISTANCE_CONFIG;
  }
  
  // Título
  st7735.st7735_write_str(20, 20, "Distancia ejercicio:", Font_7x10, ST7735_WHITE);
  
  // Opciones con indicador de selección
  const char* options[] = {"500 metros", "1 kilometro", "1.5 kilometros"};
  int yPos = 35;
  
  for (int i = 0; i < DISTANCE_COUNT; i++) {
    uint16_t bgColor = (i == selectedExerciseDistance) ? MORADO : ST7735_BLACK;
    uint16_t textColor = (i == selectedExerciseDistance) ? ST7735_WHITE : ST7735_GRAY;
    
    // Dibujar fondo si está seleccionado
    if (i == selectedExerciseDistance) {
      fillRectPixels(10, yPos - 2, 140, 12, MORADO);
    }
    
    // Indicador de selección
    if (i == selectedExerciseDistance) {
      st7735.st7735_write_str(12, yPos, ">", Font_7x10, ST7735_WHITE, MORADO);
    } else {
      st7735.st7735_write_str(12, yPos, " ", Font_7x10, ST7735_BLACK);
    }
    
    // Texto de la opción
    st7735.st7735_write_str(20, yPos, options[i], Font_7x10, textColor, bgColor);
    
    yPos += 15;
  }
  
  // Información de configuración LoRa
  st7735.st7735_write_str(5, 75, "Select: Confirmar", Font_7x10, NARANJA);
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
      if (dx * dx + dy * dy <= radius * radius) st7735.st7735_draw_pixel(x + dx, y + dy, circleColor);
  const int totalHeight = radius;
  const int iconHalfHeight = totalHeight / 2;
  const int iconMaxWidth = radius / 1.2;
  const int borderThickness = 1;
  const int max_i_val = iconHalfHeight - 1;
  int triX = x - iconMaxWidth / 3;
  int triY = y - totalHeight / 2;
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
      if (dx * dx + dy * dy <= radius * radius) st7735.st7735_draw_pixel(x + dx, y + dy, circleColor);
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
    for (uint16_t j = 0; j <= radius; j++)
      if ((i * i + j * j) <= (radius * radius)) {
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
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}

void drawCircle(int x, int y, int radius, uint16_t color) {
  for (int dy = -radius; dy <= radius; dy++) {
    for (int dx = -radius; dx <= radius; dx++) {
      if (dx * dx + dy * dy <= radius * radius) {
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
    if (P <= 0) P = P + 2 * y + 1;
    else {
      x--;
      P = P + 2 * y - 2 * x + 1;
    }
    if (x < y) break;
    st7735.st7735_draw_pixel(xc + x, yc + y, color);
    st7735.st7735_draw_pixel(xc - x, yc + y, color);
    st7735.st7735_draw_pixel(xc + x, yc - y, color);
    st7735.st7735_draw_pixel(xc - x, yc - y, color);
    if (x != y) {
      st7735.st7735_draw_pixel(xc + y, yc + x, color);
      st7735.st7735_draw_pixel(xc - y, yc + x, color);
      st7735.st7735_draw_pixel(xc + y, yc - x, color);
      st7735.st7735_draw_pixel(xc - y, yc - x, color);
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

void drawHeaderWithWiFi(const String& title) {
  st7735.st7735_fill_screen(ST7735_BLACK);

  // Draw WiFi icon in top right as status bar
  bool wifiStatus = false;  // No WiFi en dispositivo remoto
  //drawWiFiIcon(140, 1, wifiStatus);

  // Draw disconnected icon if diferencia > 10 and screen was already shown
  if (diferencia > 10 && disconnectedScreenShown) {
    drawDisconnectedIcon(120, 1);
  }

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
      drawLine(x + b * 3, barY, x + b * 3, barY + barHeight, barColor);
    }
  } else {
    // Disconnected WiFi icon (larger red X)
    drawLine(x + 2, y + 2, x + 10, y + 10, ST7735_RED);
    drawLine(x + 10, y + 2, x + 2, y + 10, ST7735_RED);
  }
}

void drawDisconnectedIcon(int x, int y) {
  // Dibujar un símbolo de desconexión (X roja con círculo)
  // Círculo exterior
  drawCircleOutline(x + 6, y + 6, 5, ST7735_RED);
  // X en el centro 
  drawLine(x + 3, y + 3, x + 9, y + 9, ST7735_RED);
  drawLine(x + 9, y + 3, x + 3, y + 9, ST7735_RED);
}

// Helper to draw slightly bolder text by overdrawing with 1px offset
void write_str_bold(uint16_t x, uint16_t y, const char* text, FontDef font, uint16_t color, uint16_t bgcolor) {
  st7735.st7735_write_str(x, y, text, font, color, bgcolor);
  st7735.st7735_write_str(x + 1, y, text, font, color, bgcolor);
}

// Función para dibujar una brújula simple
void drawCompass(int centerX, int centerY, int radius, float bearing, uint16_t color) {
  static float lastBearing = -1.0f;
  static int lastArrowX = -1, lastArrowY = -1;
  static int lastTip1X = -1, lastTip1Y = -1, lastTip2X = -1, lastTip2Y = -1;
  
  // Limpiar la flecha anterior si cambió la dirección
  if (lastBearing >= 0.0f && lastBearing != bearing) {
    int arrowClearX = min(lastArrowX, min(lastTip1X, lastTip2X)) - 2;
    int arrowClearY = min(lastArrowY, min(lastTip1Y, lastTip2Y)) - 2;
    int arrowClearW = max(lastArrowX, max(lastTip1X, lastTip2X)) - arrowClearX + 4;
    int arrowClearH = max(lastArrowY, max(lastTip1Y, lastTip2Y)) - arrowClearY + 4;
    
    for (int y = arrowClearY; y < arrowClearY + arrowClearH; y++) {
      for (int x = arrowClearX; x < arrowClearX + arrowClearW; x++) {
        if (x >= 0 && x < 160 && y >= 0 && y < 128) {
          st7735.st7735_draw_pixel(x, y, ST7735_BLACK);
        }
      }
    }
  }
  
  // Dibujar círculo exterior
  for (int angle = 0; angle < 360; angle += 5) {
    float rad = angle * 3.14159265359 / 180.0;
    int x1 = centerX + (radius - 2) * cos(rad);
    int y1 = centerY + (radius - 2) * sin(rad);
    int x2 = centerX + radius * cos(rad);
    int y2 = centerY + radius * sin(rad);
    drawLine(x1, y1, x2, y2, color);
  }
  
  // Dibujar flecha de dirección
  float adjustedBearing = bearing - 90.0;
  if (adjustedBearing < 0) adjustedBearing += 360.0;
  
  float arrowRad = adjustedBearing * 3.14159265359 / 180.0;
  int arrowX = centerX + (radius - 5) * cos(arrowRad);
  int arrowY = centerY + (radius - 5) * sin(arrowRad);
  
  drawLine(centerX, centerY, arrowX, arrowY, color);
  
  float tip1Rad = arrowRad - 0.3;
  float tip2Rad = arrowRad + 0.3;
  int tip1X = arrowX - 8 * cos(tip1Rad);
  int tip1Y = arrowY - 8 * sin(tip1Rad);
  int tip2X = arrowX - 8 * cos(tip2Rad);
  int tip2Y = arrowY - 8 * sin(tip2Rad);
  
  drawLine(arrowX, arrowY, tip1X, tip1Y, color);
  drawLine(arrowX, arrowY, tip2X, tip2Y, color);
  
  lastBearing = bearing;
  lastArrowX = arrowX;
  lastArrowY = arrowY;
  lastTip1X = tip1X;
  lastTip1Y = tip1Y;
  lastTip2X = tip2X;
  lastTip2Y = tip2Y;
  
  // Marcas cardinales
  st7735.st7735_write_str(centerX - 3, centerY - radius - 8, "N", Font_7x10, color);
  st7735.st7735_write_str(centerX - 3, centerY + radius + 2, "S", Font_7x10, color);
  st7735.st7735_write_str(centerX + radius + 2, centerY - 3, "E", Font_7x10, color);
  st7735.st7735_write_str(centerX - radius - 8, centerY - 3, "W", Font_7x10, color);
}

// Función para mapeo flotante
int mapf(float value, float in_min, float in_max, int out_min, int out_max) {
  if (fabs(in_max - in_min) < 1e-8) return (out_min + out_max) / 2;
  return out_min + (int)(((value - in_min) * (out_max - out_min)) / (in_max - in_min));
}

// Pantalla de gestión de waypoints
void drawWaypointManagerScreen(bool firstDraw) {
  static MenuScreen lastScreen = SCREEN_COUNT;
  loadWaypointsFromStorage();
  
  // Inicializar selectedWaypointIndex si es necesario
  if (waypointCount > 0 && selectedWaypointIndex < 0) {
    selectedWaypointIndex = 0;
  } else if (waypointCount == 0) {
    selectedWaypointIndex = -1;
  }
  
  if (firstDraw || lastScreen != SCREEN_WAYPOINT_MANAGER) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    drawHeaderWithWiFi("Gestionar Puntos");
    lastScreen = SCREEN_WAYPOINT_MANAGER;
  }
  
  // Sistema de scroll para waypoints (mostrar solo 3 por pantalla)
  int maxScrollOffset = max(0, waypointCount - 3);
  
  // Mostrar solo 3 waypoints por pantalla
  int yPos = 16;
  for (int i = 0; i < 3 && (i + waypointScrollOffset) < waypointCount; i++) {
    int actualIndex = i + waypointScrollOffset;
    String waypointText = String(actualIndex + 1) + ". " + waypoints[actualIndex].name;
    if (waypointText.length() > 18) {
      waypointText = waypointText.substring(0, 15) + "...";
    }
    
    uint16_t textColor = (actualIndex == selectedWaypointIndex) ? MORADO : ST7735_WHITE;
    st7735.st7735_write_str(0, yPos, waypointText.c_str(), Font_7x10, textColor);
    
    // Mostrar coordenadas abreviadas
    String coords = String(waypoints[actualIndex].latitude, 4) + "," + String(waypoints[actualIndex].longitude, 4);
    st7735.st7735_write_str(0, yPos + 10, coords.c_str(), Font_7x10, ST7735_GRAY);
    
    yPos += 20;
  }
  
  // Indicadores de scroll
  if (waypointCount > 3) {
    if (waypointScrollOffset < maxScrollOffset) {
      // Triángulo abajo
      for (int dy = 0; dy < 8; dy++) {
        int startX = 140 + dy;
        int endX = 148 - dy;
        for (int x = startX; x <= endX; x++) {
          st7735.st7735_draw_pixel(x, 70 + dy, ST7735_GRAY);
        }
      }
    }
    if (waypointScrollOffset > 0) {
      // Triángulo arriba
      for (int dy = 0; dy < 8; dy++) {
        int startX = 140 + dy;
        int endX = 148 - dy;
        for (int x = startX; x <= endX; x++) {
          st7735.st7735_draw_pixel(x, 16 - dy, ST7735_GRAY);
        }
      }
    }
  }
  
  if (waypointCount == 0) {
    st7735.st7735_write_str(0, 40, "No hay puntos guardados", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 52, "Izq+Der: Agregar punto", Font_7x10, NARANJA);
  }
}

// Pantalla de navegación hacia waypoint (backtrack)
void drawBacktrackScreen(bool firstDraw) {
  static MenuScreen lastScreen = SCREEN_COUNT;
  
  if (firstDraw || lastScreen != SCREEN_BACKTRACK) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    drawHeaderWithWiFi("Backtrack");
    lastScreen = SCREEN_BACKTRACK;
  }
  
  // PROTECCIÓN CRÍTICA: Cargar waypoints y verificar índice ANTES de cualquier acceso
  loadWaypointsFromStorage();
  if (waypointCount == 0 || selectedWaypointIndex < 0 || selectedWaypointIndex >= waypointCount) {
    st7735.st7735_write_str(0, 20, "Selecciona un punto", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 32, "desde Gestionar Puntos", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 44, "para comenzar navegacion", Font_7x10, ST7735_WHITE);
    return; // Salir para no dibujar el resto de la pantalla
  }
  
  if (!localPositionSet) {
    st7735.st7735_write_str(0, 20, "GPS Local no disponible", Font_7x10, ST7735_RED);
    st7735.st7735_write_str(0, 32, "Esperando señal GPS...", Font_7x10, ST7735_GRAY);
    return;
  }
  
  // Actualizar navegación
  updateWaypointNavigation();
  
  // PROTECCIÓN ADICIONAL antes de acceder al array (por si acaso cambió durante updateWaypointNavigation)
  if (selectedWaypointIndex < 0 || selectedWaypointIndex >= waypointCount) {
    return;
  }
  
  // Mostrar información del waypoint objetivo
  String targetName = waypoints[selectedWaypointIndex].name;
  st7735.st7735_write_str(0, 16, "Hacia: " + targetName, Font_7x10, NARANJA);
  
  // Mostrar información de navegación
  String distanceStr = String("Dist: ") + String(waypointNavigation.distance, 1) + "m";
  String bearingStr = String("Dir: ") + String(waypointNavigation.bearing, 0) + "°";
  String directionStr = String("Hacia: ") + waypointNavigation.direction;
  
  // Dibujar brújula a la derecha
  drawCompass(120, 40, 20, waypointNavigation.bearing, MORADO);
  
  // Información de navegación a la izquierda
  st7735.st7735_write_str(0, 28, bearingStr.c_str(), Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(0, 40, directionStr.c_str(), Font_7x10, NARANJA);
  
  // Distancia centrada abajo
  int distanceX = (160 - distanceStr.length() * 7) / 2;
  st7735.st7735_write_str(distanceX, 68, distanceStr.c_str(), Font_7x10, ST7735_WHITE);
  
  // Indicador de GPS local
  st7735.st7735_write_str(100, 0, "GPS", Font_7x10, MORADO);
}

// Pantalla de minimapa
void drawBacktrackMapScreen(bool firstDraw) {
  static MenuScreen lastScreen = SCREEN_COUNT;
  static float lastMapLat = -999.0f;
  static float lastMapLon = -999.0f;
  static int lastMapWaypointCount = -1;
  static int lastMapSelectedIdx = -2;
  
  loadWaypointsFromStorage();
  
  if (firstDraw || lastScreen != SCREEN_BACKTRACK_MAP) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    st7735.st7735_write_str(10, 0, "Mapa de ruta", Font_7x10, MORADO);
    lastScreen = SCREEN_BACKTRACK_MAP;
  }
  
  // PROTECCIÓN CRÍTICA: Verificar índice antes de cualquier acceso
  if (waypointCount == 0) {
    st7735.st7735_write_str(10, 40, "No hay puntos", Font_7x10, ST7735_GRAY);
    return;
  }
  
  // Verificar que el índice seleccionado sea válido
  if (selectedWaypointIndex < 0 || selectedWaypointIndex >= waypointCount) {
    st7735.st7735_write_str(10, 40, "Indice invalido", Font_7x10, ST7735_RED);
    return;
  }
  
  // Verificar si algo cambió
  bool needsRedraw = false;
  if (fabs(localLatitude - lastMapLat) > 0.00001f || 
      fabs(localLongitude - lastMapLon) > 0.00001f ||
      waypointCount != lastMapWaypointCount ||
      selectedWaypointIndex != lastMapSelectedIdx) {
    needsRedraw = true;
    lastMapLat = localLatitude;
    lastMapLon = localLongitude;
    lastMapWaypointCount = waypointCount;
    lastMapSelectedIdx = selectedWaypointIndex;
  }
  
  if (!needsRedraw) {
    return;
  }
  
  // Limpiar solo el área del mapa
  fillRectPixels(10, 10, 140, 60, ST7735_BLACK);
  
  // Filtrar puntos cercanos
  const float MAX_DIST_KM = 5.0f;
  int nearbyIdx[MAX_WAYPOINTS];
  int nearbyCount = 0;
  float refLat = localLatitude;
  float refLon = localLongitude;
  bool useNearby = localPositionSet;
  
  if (useNearby) {
    for (int i = 0; i < waypointCount; i++) {
      // PROTECCIÓN: Verificar que el índice i sea válido
      if (i < 0 || i >= waypointCount) continue;
      float dLat = (waypoints[i].latitude - refLat) * 3.14159265359 / 180.0;
      float dLon = (waypoints[i].longitude - refLon) * 3.14159265359 / 180.0;
      float a = sin(dLat/2)*sin(dLat/2) + cos(refLat * 3.14159265359 / 180.0)*cos(waypoints[i].latitude * 3.14159265359 / 180.0)*sin(dLon/2)*sin(dLon/2);
      float c = 2 * atan2(sqrt(a), sqrt(1-a));
      float dist = 6371.0f * c;
      if (dist <= MAX_DIST_KM) {
        nearbyIdx[nearbyCount++] = i;
      }
    }
  }
  
  if (useNearby && nearbyCount == 0) {
    st7735.st7735_write_str(10, 40, "No hay puntos cercanos", Font_7x10, ST7735_GRAY);
    return;
  }
  
  // Calcular bounding box
  float minLat, maxLat, minLon, maxLon;
  if (useNearby && nearbyCount > 0) {
    // PROTECCIÓN: Verificar que nearbyIdx[0] sea válido
    if (nearbyIdx[0] < 0 || nearbyIdx[0] >= waypointCount) {
      st7735.st7735_write_str(10, 40, "Error indice", Font_7x10, ST7735_RED);
      return;
    }
    minLat = maxLat = waypoints[nearbyIdx[0]].latitude;
    minLon = maxLon = waypoints[nearbyIdx[0]].longitude;
    for (int i = 1; i < nearbyCount; i++) {
      // PROTECCIÓN: Verificar que nearbyIdx[i] sea válido
      if (nearbyIdx[i] < 0 || nearbyIdx[i] >= waypointCount) continue;
      float lat = waypoints[nearbyIdx[i]].latitude;
      float lon = waypoints[nearbyIdx[i]].longitude;
      if (lat < minLat) minLat = lat;
      if (lat > maxLat) maxLat = lat;
      if (lon < minLon) minLon = lon;
      if (lon > maxLon) maxLon = lon;
    }
    if (localPositionSet) {
      if (localLatitude < minLat) minLat = localLatitude;
      if (localLatitude > maxLat) maxLat = localLatitude;
      if (localLongitude < minLon) minLon = localLongitude;
      if (localLongitude > maxLon) maxLon = localLongitude;
    }
  } else {
    // PROTECCIÓN: Verificar que haya al menos un waypoint
    if (waypointCount > 0) {
      minLat = maxLat = waypoints[0].latitude;
      minLon = maxLon = waypoints[0].longitude;
      for (int i = 1; i < waypointCount; i++) {
        // PROTECCIÓN: Verificar que i sea válido
        if (i < 0 || i >= waypointCount) continue;
        if (waypoints[i].latitude < minLat) minLat = waypoints[i].latitude;
        if (waypoints[i].latitude > maxLat) maxLat = waypoints[i].latitude;
        if (waypoints[i].longitude < minLon) minLon = waypoints[i].longitude;
        if (waypoints[i].longitude > maxLon) maxLon = waypoints[i].longitude;
      }
    } else {
      return;
    }
    if (localPositionSet) {
      if (localLatitude < minLat) minLat = localLatitude;
      if (localLatitude > maxLat) maxLat = localLatitude;
      if (localLongitude < minLon) minLon = localLongitude;
      if (localLongitude > maxLon) maxLon = localLongitude;
    }
  }
  
  float latMargin = (maxLat - minLat) * 0.1f + 0.0001f;
  float lonMargin = (maxLon - minLon) * 0.1f + 0.0001f;
  minLat -= latMargin; maxLat += latMargin;
  minLon -= lonMargin; maxLon += lonMargin;
  int mapX0 = 10, mapY0 = 10, mapW = 140, mapH = 60;
  
  if (fabs(maxLat - minLat) < 0.00005f) { maxLat += 0.000025f; minLat -= 0.000025f; }
  if (fabs(maxLon - minLon) < 0.00005f) { maxLon += 0.000025f; minLon -= 0.000025f; }
  
  // Dibujar líneas entre puntos
  if (useNearby && nearbyCount > 1) {
    for (int i = 1; i < nearbyCount; i++) {
      // PROTECCIÓN: Verificar índices antes de acceder
      if (nearbyIdx[i-1] < 0 || nearbyIdx[i-1] >= waypointCount ||
          nearbyIdx[i] < 0 || nearbyIdx[i] >= waypointCount) continue;
      int x0 = mapf(waypoints[nearbyIdx[i-1]].longitude, minLon, maxLon, mapX0, mapX0+mapW);
      int y0 = mapf(waypoints[nearbyIdx[i-1]].latitude,  maxLat, minLat, mapY0, mapY0+mapH);
      int x1 = mapf(waypoints[nearbyIdx[i]].longitude,   minLon, maxLon, mapX0, mapX0+mapW);
      int y1 = mapf(waypoints[nearbyIdx[i]].latitude,    maxLat, minLat, mapY0, mapY0+mapH);
      drawLine(x0, y0, x1, y1, ST7735_GRAY);
    }
  } else if (!useNearby && waypointCount > 1) {
    for (int i = 1; i < waypointCount; i++) {
      // PROTECCIÓN: Verificar índices antes de acceder
      if (i-1 < 0 || i-1 >= waypointCount || i < 0 || i >= waypointCount) continue;
      int x0 = mapf(waypoints[i-1].longitude, minLon, maxLon, mapX0, mapX0+mapW);
      int y0 = mapf(waypoints[i-1].latitude,  maxLat, minLat, mapY0, mapY0+mapH);
      int x1 = mapf(waypoints[i].longitude,   minLon, maxLon, mapX0, mapX0+mapW);
      int y1 = mapf(waypoints[i].latitude,    maxLat, minLat, mapY0, mapY0+mapH);
      drawLine(x0, y0, x1, y1, ST7735_GRAY);
    }
  }
  
  // Dibujar los puntos
  if (useNearby && nearbyCount > 0) {
    for (int i = 0; i < nearbyCount; i++) {
      int idx = nearbyIdx[i];
      // PROTECCIÓN: Verificar índice antes de acceder
      if (idx < 0 || idx >= waypointCount) continue;
      int x = mapf(waypoints[idx].longitude, minLon, maxLon, mapX0, mapX0+mapW);
      int y = mapf(waypoints[idx].latitude,  maxLat, minLat, mapY0, mapY0+mapH);
      uint16_t color = (idx == selectedWaypointIndex) ? MORADO : NARANJA;
      drawCircle(x, y, 3, color);
    }
  } else {
    for (int i = 0; i < waypointCount; i++) {
      // PROTECCIÓN: Verificar índice antes de acceder
      if (i < 0 || i >= waypointCount) continue;
      int x = mapf(waypoints[i].longitude, minLon, maxLon, mapX0, mapX0+mapW);
      int y = mapf(waypoints[i].latitude,  maxLat, minLat, mapY0, mapY0+mapH);
      uint16_t color = (i == selectedWaypointIndex) ? MORADO : NARANJA;
      drawCircle(x, y, 3, color);
    }
  }
  
  // Dibuja la posición actual
  if (localPositionSet) {
    int x = mapf(localLongitude, minLon, maxLon, mapX0, mapX0+mapW);
    int y = mapf(localLatitude,  maxLat, minLat, mapY0, mapY0+mapH);
    drawCircle(x, y, 5, ST7735_WHITE);
    st7735.st7735_write_str(x+6, y-4, "Tu", Font_7x10, ST7735_WHITE);
  }
}

// Función para dibujar iconos del menú principal
void drawMenuIcon(int x, int y, int itemIndex, bool selected, uint16_t bgcolor) {
  uint16_t fg = selected ? MORADO : ST7735_WHITE;
  // Clear icon area first to avoid artifacts - increased size to 18x18
  fillRectPixels(x, y, 18, 18, bgcolor);
  switch (itemIndex) {
    case 0:
      {  // GPS: icono de satélite/ubicación
        // Círculo central
        drawCircle(x + 9, y + 9, 4, fg);
        // Líneas de señal
        drawLine(x + 9, y + 2, x + 9, y + 6, fg);
        drawLine(x + 9, y + 12, x + 9, y + 16, fg);
        drawLine(x + 2, y + 9, x + 6, y + 9, fg);
        drawLine(x + 12, y + 9, x + 16, y + 9, fg);
      }
      break;
    case 1:
      {  // Musica: icono de altavoz/música
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
      }
      break;
    case 2:
      {  // Monitoreo: icono de ojo/vigilancia
        // Contorno del ojo
        drawCircle(x + 9, y + 9, 6, fg);
        // Pupila
        drawCircle(x + 9, y + 9, 2, ST7735_BLACK);
        // Pestañas
        for (int i = 0; i < 3; i++) {
          drawLine(x + 6 + i * 2, y + 3, x + 6 + i * 2, y + 5, fg);
          drawLine(x + 6 + i * 2, y + 13, x + 6 + i * 2, y + 15, fg);
        }
      }
      break;
        case 3:
      {  // Info: círculo con i
        drawCircle(x + 9, y + 9, 7, fg);
        drawLine(x + 9, y + 6, x + 9, y + 11, fg);
        st7735.st7735_draw_pixel(x + 9, y + 5, fg);
      }
      break;
    case 4:
      {  // Ejercicio: icono de cronómetro
        drawCircle(x + 9, y + 9, 6, fg);
        drawLine(x + 9, y + 9, x + 9, y + 5, fg);
        drawLine(x + 9, y + 9, x + 12, y + 9, fg);
      }
      break;
    case 5:
      {  // Emergencia: icono de alerta
        drawCircle(x + 9, y + 9, 6, fg);
        drawLine(x + 9, y + 3, x + 9, y + 6, fg);
        st7735.st7735_draw_pixel(x + 9, y + 12, fg);
      }
      break;
    case 6:
      {  // Backtrack: flecha de retorno
        drawLine(x + 4, y + 2, x + 4, y + 16, fg);
        drawLine(x + 4, y + 16, x + 14, y + 16, fg);
        drawLine(x + 14, y + 16, x + 10, y + 12, fg);
        drawLine(x + 14, y + 16, x + 10, y + 20, fg);
      }
      break;
    }
}