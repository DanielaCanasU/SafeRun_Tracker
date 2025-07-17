#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize button handler
 */
void initButtonHandler();

/**
 * @brief Button interrupt handler (IRAM_ATTR for ESP32)
 */
void IRAM_ATTR handleButtonInterrupt();

/**
 * @brief Process button patterns and long press detection
 * @param currentTime Current time in milliseconds
 * @return true if pattern was completed, false otherwise
 */
bool processButtonPattern(unsigned long currentTime);

/**
 * @brief Check if waiting for long press
 * @return true if waiting for long press, false otherwise
 */
bool isWaitingForLongPress();

/**
 * @brief Reset button state
 */
void resetButtonState();

/**
 * @brief Get current click count
 * @return Number of clicks detected
 */
int getClickCount();

// =============================================================================
// EXTERNAL CALLBACKS
// =============================================================================
// These functions should be implemented in the main file or appropriate handler

/**
 * @brief Callback for when link request pattern is completed
 */
extern void onLinkRequestPatternComplete();

#endif // BUTTON_HANDLER_H 