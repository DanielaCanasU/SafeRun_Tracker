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
  // Pines botones
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);

  analogReadResolution(12);

  // Interrupciones botones
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), handleLeftInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), handleRightInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_SELECT), handleSelectInterrupt, CHANGE);

  // Estado inicial de botones
  initButtonsState();

  Serial.begin(115200);

  // I2C acelerómetro
  Wire.begin(ADXL345_SDA_PIN, ADXL345_SCL_PIN);

  // Pantalla
  displayInit();
  st7735.st7735_fill_screen(ST7735_BLACK);
  drawGPSScreen();

  // LoRa SX1262
  SPI.begin();
  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, DIO2, DIO3, RX_EN, TX_EN, SW, LORA_DEVICE)) {
    Serial.println(F("LoRa Device found"));
    delay(1000);
  } else {
    Serial.println(F("No device responding"));
  }
  configureSX1262();

  // GPS
  Serial.println("Configurando GPS...");
  configureGPS();

  // DFPlayer (opcional)
  Serial.println("Configurando DFPlayer Mini...");
  //configureDFPlayer();

  // Acelerómetro
  Serial.println("Iniciando ADXL345");
  setupAcelerometro();
}

void loop() {
  const unsigned long currentTime = millis();

  // Botones
  processButtonPress();

  // Pantallas según menú
  if (currentScreen == SCREEN_GPS) {
    handleGPSScreen();
  } else if (currentScreen == SCREEN_MP3_FOLDER) {
    handleFolderScreen();
  } else if (currentScreen == SCREEN_MP3_PLAYER) {
    handleMP3Screen();
  } else if (currentScreen == SCREEN_MONITORING) {
    handleMonitoringScreen();
  }

  // Monitoreo acelerómetro
  accelLoop();

  // GPS feed
  getGpsData();

  // Envío periódico por LoRa
  if (currentTime - lastSendTime_LoRa >= sendInterval_LoRa) {
    lastSendTime_LoRa = currentTime;
    char message[128];
#ifdef ENVIAR_ACELEROMETRO
    const int len = snprintf(message, sizeof(message),
                             "GPS:%s,%s; ST:%d,%d,%d,%d,%d; ACC:%.2f,%.2f,%.2f",
                             latitude.c_str(), longitude.c_str(),
                             isMonitoringActive ? 1 : 0, impacto ? 1 : 0,
                             free_fall ? 1 : 0, segunda_condicion_caida ? 1 : 0,
                             emergencia ? 1 : 0,
                             x, y, z);
#else
    const int len = snprintf(message, sizeof(message),
                             "GPS:%s,%s; ST:%d,%d,%d,%d,%d",
                             latitude.c_str(), longitude.c_str(),
                             isMonitoringActive ? 1 : 0, impacto ? 1 : 0,
                             free_fall ? 1 : 0, segunda_condicion_caida ? 1 : 0,
                             emergencia ? 1 : 0);
#endif
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
  updateGPSBatteryIndicator();
}


