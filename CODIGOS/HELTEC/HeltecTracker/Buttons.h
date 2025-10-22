#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Pins.h"

class Botones {
    private:  
      int pin;
      bool comboPressed = false;
      unsigned long comboStart = 0;
      unsigned long comboActivatedTime = 0;
      bool comboJustActivated = false;
      bool left;
      bool right;
      bool sel;
      unsigned long currentTime;

    public:
        Botones();
        void init();
        void checkUserEntry();
        void checkEmergencyCombo();
        void handleUserGestures(uint8_t button, bool isLongPress, bool isDoubleClick, bool isTripleClick);
};

void initButtonsState();
void IRAM_ATTR handleLeftInterrupt();
void IRAM_ATTR handleRightInterrupt();
void IRAM_ATTR handleSelectInterrupt();
void processButtonPress();
void handleButtonPress(uint8_t button, bool isLongPress, bool isDoubleClick, bool isTripleClick = false);


