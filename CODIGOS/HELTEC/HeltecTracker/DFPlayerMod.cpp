#include "DFPlayerMod.h"
#include <SoftwareSerial.h> 
#include <HardwareSerial.h> //Puerto Serial
#include "AppState.h"


EspSoftwareSerial::UART DFPlayerSerial;

Musica::Musica(){

}

void Musica::init(){
  DFPlayerSerial.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, HELTEC_RX2_PIN, HELTEC_TX2_PIN, false, 256);
  delay(1000);
  if (!dfPlayer.begin(DFPlayerSerial, true, false)) {
    Serial.println("Error iniciando DFPlayer Mini:");
    Serial.println("1. Verifique conexiones");
    Serial.println("2. Inserte la tarjeta SD");
    while (true) {}
  }
  Serial.println("DFPlayer Mini en línea.");
  dfPlayer.setTimeOut(500);
  dfPlayer.volume(20);
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  lista_canciones.clear();
  for (int i = 1; i <= 255; i++) {
    int count = dfPlayer.readFileCountsInFolder(i); delay(500);
    if (count > 0) lista_canciones.push_back(count);
    else if (count == 0) break;
  }
  maxFolders = lista_canciones.size();
}

void configureDFPlayer() {
  DFPlayerSerial.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, HELTEC_RX2_PIN, HELTEC_TX2_PIN, false, 256);
  delay(1000);
  if (!dfPlayer.begin(DFPlayerSerial, true, false)) {
    Serial.println("Error iniciando DFPlayer Mini:");
    Serial.println("1. Verifique conexiones");
    Serial.println("2. Inserte la tarjeta SD");
    while (true) {}
  }
  Serial.println("DFPlayer Mini en línea.");
  dfPlayer.setTimeOut(500);
  dfPlayer.volume(20);
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  lista_canciones.clear();
  for (int i = 1; i <= 255; i++) {
    int count = dfPlayer.readFileCountsInFolder(i); delay(500);
    if (count > 0) lista_canciones.push_back(count);
    else if (count == 0) break;
  }
  maxFolders = lista_canciones.size();
}

void playSound(uint8_t folder, uint8_t file) {
  Serial.printf("Reproduciendo carpeta %d, archivo %d\n", folder, file);
  dfPlayer.playFolder(folder, file);
}

void adjustVolume(uint8_t volume) {
  if (volume <= 30) { Serial.printf("Ajustando volumen a %d\n", volume); dfPlayer.volume(volume); }
}


