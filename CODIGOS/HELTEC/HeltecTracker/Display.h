// Display.h
#pragma once
#include <Arduino.h>
#include <HT_st7735.h>
#include "AppState.h"

void displayInit();
void drawMenuDots(MenuScreen currentMenu);
void drawGPSScreen();
void updateGPSFieldsIfChanged(const String& newTime, const String& newLat, const String& newLon);
void updateGPSBatteryIndicator();
void drawMP3Screen(bool firstDraw = false);
void drawMonitoringScreen();

// Iconos
void drawMinusIcon(uint16_t x, uint16_t y, uint16_t color, uint8_t size);
void drawPlusIcon(uint16_t x, uint16_t y, uint16_t color, uint8_t size);
void drawPlayIcon(uint16_t x, uint16_t y, uint16_t borderColor, uint16_t radius, uint16_t circleColor, uint16_t fillColor);
void drawPauseIcon(uint16_t x, uint16_t y, uint16_t borderColor, uint16_t radius, uint16_t circleColor, uint16_t fillColor);
void drawPrevIcon(uint16_t x, uint16_t y, uint16_t color);
void drawNextIcon(uint16_t x, uint16_t y, uint16_t color);
void drawVolDownIcon(uint16_t x, uint16_t y, uint16_t color);
void drawVolUpIcon(uint16_t x, uint16_t y, uint16_t color);
void drawRoundedRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t radius, uint16_t color);

