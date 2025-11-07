#pragma once
#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include "AppState.h"
#include "Pins.h"
#include <SoftwareSerial.h> // <-- usar SoftwareSerial

void configureDFPlayer();
void playSound(uint8_t folder, uint8_t file);
void adjustVolume(uint8_t volume);

class Musica {
    private:
        int cancion;
    public:
        Musica();
        void init();
        void volumeUp();
        void volumeDown();

        unsigned long lastVolumeUpdateTime; 
};

// declarar el objeto serial usado por el DFPlayer (definido en DFPlayerMod.cpp)
extern SoftwareSerial DFPlayerSerial;