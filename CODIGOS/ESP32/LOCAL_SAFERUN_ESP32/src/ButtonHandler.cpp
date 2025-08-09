#include "ButtonHandler.h"
#include "config.h"
#include "Utils.h"

// =============================================================================
// PRIVATE VARIABLES
// =============================================================================
static volatile int clickCount = 0;
static volatile unsigned long lastClickTime = 0;
static bool waitingForLongPress = false;
static unsigned long patternDetectedTime = 0;
static unsigned long pressStartTime = 0;

// =============================================================================
// BUTTON HANDLER IMPLEMENTATION
// =============================================================================

void initButtonHandler() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);
    debugPrint("Button handler initialized");
}

void IRAM_ATTR handleButtonInterrupt() {
    unsigned long now = millis();

    // Ignora rebotes
    if (now - lastClickTime < BUTTON_DEBOUNCE_TIME) return;

    if (now - lastClickTime > BUTTON_CLICK_TIMEOUT) {
        clickCount = 1; // Nuevo conteo
    } else {
        clickCount++;
    }

    lastClickTime = now;

    if (clickCount == REQUIRED_CLICKS) {
        waitingForLongPress = true;
        patternDetectedTime = now;
    }
    Serial.println(clickCount);
}

bool processButtonPattern(unsigned long currentTime) {
    if (waitingForLongPress) {
        if (digitalRead(BUTTON_PIN) == LOW) {
            if (pressStartTime == 0) {
                pressStartTime = currentTime;
            } else if (currentTime - pressStartTime >= LONG_PRESS_TIME) {
                debugPrint("Patrón completo detectado. Aceptando solicitud...");
                //onLinkRequestPatternComplete();
                resetButtonState();
                return true;
            }
            Serial.println("Long press");
        } else {
            pressStartTime = 0; // Se soltó antes del tiempo
            Serial.println("Short press");
        }

        // Tiempo límite para presionar después del patrón
        if (currentTime - patternDetectedTime > PATTERN_TIMEOUT) {
            debugPrint("Tiempo excedido. Reiniciando patrón.");
            resetButtonState();
        }
    }
    return false;
}

bool isWaitingForLongPress() {
    return waitingForLongPress;
}

void resetButtonState() {
    waitingForLongPress = false;
    pressStartTime = 0;
    clickCount = 0;
}

int getClickCount() {
    return clickCount;
} 