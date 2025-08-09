#pragma once
#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include "AppState.h"
#include "Pins.h"

void configureDFPlayer();
void playSound(uint8_t folder, uint8_t file);
void adjustVolume(uint8_t volume);


