#include "GPS.h"
#include "Display.h"
#include <HT_TinyGPS++.h>

void handleGPSScreen() {
  if (currentScreen != pastScreen) {
    pastScreen = currentScreen;
    drawGPSScreen();
  }
}

void handleMP3Screen() {
  // La actualización de la pantalla MP3 se maneja desde los eventos de botón
}

void handleMonitoringScreen() {
  // Se actualiza desde los eventos de botón
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

void handleFolderScreen() {
  // Lógica se maneja en eventos de botón
}

void configureGPS() {
  pinMode(VGNSS_CTRL, OUTPUT);
  digitalWrite(VGNSS_CTRL, HIGH);
  Serial1.begin(115200, SERIAL_8N1, 33, 34);
  delay(100);
}

void getGpsData() {
  while (Serial1.available() > 0) {
    if (Serial1.peek() != '\n') gps.encode(Serial1.read());
    else {
      Serial1.read();
      if (gps.time.second() == 0) continue;
      String new_time_str = String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) + ":" + String(gps.time.centisecond());
      String new_latitude = String("LAT: ") + String(gps.location.lat(), 6);
      String new_longitude = String("LON: ") + String(gps.location.lng(), 6);

      // Redibujo parcial solo si cambian
      updateGPSFieldsIfChanged(new_time_str, new_latitude, new_longitude);
      // Actualiza estado en memoria
      time_str = new_time_str;
      latitude = new_latitude;
      longitude = new_longitude;
      while (Serial1.read() > 0) {}
    }
  }
}


