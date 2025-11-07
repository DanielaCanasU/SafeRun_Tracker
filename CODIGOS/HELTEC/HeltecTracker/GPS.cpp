#include "GPS.h"
#include "Display.h"
#include <HT_TinyGPS++.h>

Geolocation::Geolocation(){}


void Geolocation::init(){
  
  // Configurar pin de alimentación del GPS
  pinMode(VGNSS_CTRL, OUTPUT);
  digitalWrite(VGNSS_CTRL, HIGH);
  delay(500); // Dar tiempo para que el GPS se encienda
  
  // Inicializar comunicación serial con GPS
  Serial1.begin(115200, SERIAL_8N1, 33, 34);
  delay(1000); // Dar más tiempo para estabilización
  
  
  Serial.println("Esperando datos GPS válidos...");
  unsigned long startTime = millis();
  int dots = 0;
  
  // Inicializar variables de estado
  gpsDataValid = false;
  lastGPSUpdate = 0;
  while (!gpsDataValid) {
    getGpsData();
    
    if (gps.location.isValid() && 
        gps.location.lat() != 0.0 && 
        gps.location.lng() != 0.0) {
      
      unsigned long timeToFix = (millis() - startTime) / 1000;
      Serial.println("\nFix GPS obtenido!");
      Serial.printf("Tiempo para fix: %lu segundos\n", timeToFix);
      Serial.printf("LAT: %.6f\n", gps.location.lat());
      Serial.printf("LON: %.6f\n", gps.location.lng());
      Serial.printf("Satélites: %d\n", gps.satellites.value());
      return;
    }

    // Feedback visual cada 500ms
    if (millis() - startTime > dots * 500) {
      Serial.print(".");
      dots++;
    }
  }

  // Si inició correctamente, marcar y forzar transición a menú
  GPSLISTO = true;
  
  // Transición automática: dibujar menú principal completo
  display.drawMenu(SCREEN_MAIN_MENU, true);
}


void getGpsData() {
  // Procesar datos GPS solo si hay datos disponibles
  if (Serial1.available() == 0) return;
  
  while (Serial1.available() > 0) {
    if (Serial1.peek() != '\n') {
      gps.encode(Serial1.read());
    } else {
      Serial1.read();
      
      // Solo procesar si tenemos datos de tiempo válidos
      if (gps.time.second() == 0) continue;
      
      // Crear strings de tiempo y coordenadas
      String new_time_str = String(gps.time.hour()) + ":" + 
                           String(gps.time.minute()) + ":" + 
                           String(gps.time.second()) + ":" + 
                           String(gps.time.centisecond());
      
      String new_latitude = String("LAT: ") + String(gps.location.lat(), 6);
      String new_longitude = String("LON: ") + String(gps.location.lng(), 6);

      // Validar si las coordenadas son válidas
      bool coordsValid = gps.location.isValid() && 
                        gps.location.lat() != 0.0 && 
                        gps.location.lng() != 0.0;
      
      // Actualizar estado del GPS
      if (coordsValid) {
        gpsDataValid = true;
        lastGPSUpdate = millis();
        Serial.println("GPS: Coordenadas válidas obtenidas");
      } else {
        // Si no hay coordenadas válidas por más de 30 segundos, marcar como inválido
        if (millis() - lastGPSUpdate > 30000) {
          gpsDataValid = false;
        }
      }

      // Actualizar variables globales
      /*
      time_str = new_time_str;
      latitude = new_latitude;
      longitude = new_longitude;
      */
      Serial.println("GPS: Datos actualizados");
      Serial.println(new_latitude);

      // Redibujar pantalla GPS si está activa
      updateGPSFieldsIfChanged(new_time_str, new_latitude, new_longitude);

      // Acumular distancia de ejercicio si está grabando (usando TinyGPS++)
      if (isMonitoringActive && gps.location.isValid()) {
        double latf = gps.location.lat();
        double lonf = gps.location.lng();
        if (lastExercisePosSet) {
          double dist = TinyGPSPlus::distanceBetween(lastExerciseLat, lastExerciseLon, latf, lonf);
          if (dist > 0.1 && dist < 1000.0) exerciseDistanceMeters += (float)dist;
        }
        lastExerciseLat = (float)latf; lastExerciseLon = (float)lonf; lastExercisePosSet = true;
      }
      
      // Limpiar buffer
      while (Serial1.read() > 0) {}
    }
  }
}

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

unsigned long getExerciseElapsed(unsigned long now) {
  if (!isMonitoringActive) return exercisePausedAccumMs;
  return exercisePausedAccumMs + (now - exerciseStartMs);
}

void handleExerciseScreen() {
  if(isMonitoringActive) {
    unsigned long now = millis();
    unsigned long elapsed = getExerciseElapsed(now);
    unsigned long sec = elapsed / 1000; unsigned int hh = sec / 3600; sec %= 3600; unsigned int mm = sec / 60; unsigned int ss = sec % 60;
    char tbuf[24]; snprintf(tbuf, sizeof(tbuf), "%02u:%02u:%02u", hh, mm, ss);
    st7735.st7735_write_str(35, 28, tbuf, Font_11x18, isMonitoringActive ? ST7735_BLACK : ST7735_WHITE, isMonitoringActive ? ST7735_GREEN : ST7735_GRAY);
    
    //st7735.st7735_write_str(0, 28, tbuf, Font_7x10, ST7735_WHITE);
    char dbuf[24]; snprintf(dbuf, sizeof(dbuf), "Dist: %.1f m", exerciseDistanceMeters);
    st7735.st7735_write_str(40, 60, dbuf, Font_7x10, ST7735_WHITE);
  }
}
/*
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
*/

void handleFolderScreen() {
  // Lógica se maneja en eventos de botón
}



bool isGPSValid() {
  return gpsDataValid && gps.location.isValid() && 
         (millis() - lastGPSUpdate) < 1000; // 1 minuto de timeout
}

float getLatitude() {
  return gps.location.lat();
}

float getLongitude() {
  return gps.location.lng();
}

String getTimeString() {
  return time_str;
}
