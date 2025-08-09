#include "Display.h"

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

void drawGPSScreen() {
  st7735.st7735_fill_screen(ST7735_BLACK);
  st7735.st7735_write_str(0, 0, "GPS Screen", Font_7x10, ST7735_GREEN);
  // Pinta valores actuales como base
  st7735.st7735_write_str(0, 20, time_str, Font_7x10, ST7735_GREEN);
  st7735.st7735_write_str(0, 40, latitude, Font_7x10, ST7735_GREEN);
  st7735.st7735_write_str(0, 60, longitude, Font_7x10, ST7735_GREEN);
  updateGPSBatteryIndicator();
  drawMenuDots(SCREEN_GPS);
}

void updateGPSFieldsIfChanged(const String& newTime, const String& newLat, const String& newLon) {
  if (currentScreen != SCREEN_GPS) return;
  if (newTime != time_str) {
    st7735.st7735_fill_rectangle(0, 20, 128, 10, ST7735_BLACK);
    st7735.st7735_write_str(0, 20, newTime, Font_7x10, ST7735_GREEN);
  }
  if (newLat != latitude) {
    st7735.st7735_fill_rectangle(0, 40, 128, 10, ST7735_BLACK);
    st7735.st7735_write_str(0, 40, newLat, Font_7x10, ST7735_GREEN);
  }
  if (newLon != longitude) {
    st7735.st7735_fill_rectangle(0, 60, 128, 10, ST7735_BLACK);
    st7735.st7735_write_str(0, 60, newLon, Font_7x10, ST7735_GREEN);
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


