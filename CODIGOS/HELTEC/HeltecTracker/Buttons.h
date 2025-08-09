#pragma once
#include <Arduino.h>
#include "AppState.h"
#include "Config.h"
#include "Pins.h"

void initButtonsState();
void IRAM_ATTR handleLeftInterrupt();
void IRAM_ATTR handleRightInterrupt();
void IRAM_ATTR handleSelectInterrupt();
void processButtonPress();
void handleButtonPress(uint8_t button, bool isLongPress, bool isDoubleClick, bool isTripleClick = false);


