#include "DFPlayerMod.h"
#include <SoftwareSerial.h> 
#include <HardwareSerial.h> //Puerto Serial
#include "AppState.h"
#include "Display.h"

// Única definición del objeto serial
SoftwareSerial DFPlayerSerial(HELTEC_RX2_PIN, HELTEC_TX2_PIN); // RX, TX

Musica::Musica(){

}

void Musica::init(){
  DFPlayerSerial.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, HELTEC_RX2_PIN, HELTEC_TX2_PIN, false, 256);
  delay(1000);
  bool ok = dfPlayer.begin(DFPlayerSerial, true, false);
  if (!ok) {
    Serial.println("--------------------------------");
    Serial.println("ERROR INICIANDO REPRODUCTOR");
    Serial.println("--------------------------------");
    return;
  }
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.setTimeOut(500);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  if(!firstplay){
  dfPlayer.volume(20);
  lista_canciones.clear();
  for (int i = 1; i <= 255; i++) {
    int count = dfPlayer.readFileCountsInFolder(i); delay(500);
    if (count > 0) lista_canciones.push_back(count);
    else if (count == 0) break;
  }
  maxFolders = lista_canciones.size();
  firstplay= true;
  }
}

void Musica::volumeDown(){
  if (currentVolume > 0) { currentVolume--; dfPlayer.volume(currentVolume); drawMP3Screen();}
}

void Musica::volumeUp(){
  if (currentVolume < 30) { currentVolume++; dfPlayer.volume(currentVolume); drawMP3Screen(); }
}


void playSound(uint8_t folder, uint8_t file) {
  Serial.printf("Reproduciendo carpeta %d, archivo %d\n", folder, file);
  dfPlayer.playFolder(folder, file);
}

void adjustVolume(uint8_t volume) {
  if (volume <= 30) { Serial.printf("Ajustando volumen a %d\n", volume); dfPlayer.volume(volume); }
}