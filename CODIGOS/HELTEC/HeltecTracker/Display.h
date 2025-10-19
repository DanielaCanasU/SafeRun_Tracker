// Display.h
#pragma once
#include <Arduino.h>
#include <HT_st7735.h>
#include "AppState.h"

class Display {
    private:
        int menu;
    public:
        Display();
        void init();
        void drawMenu(MenuScreen currentMenu, bool firstDraw = true);
};

void displayInit();
void drawMenuDots(MenuScreen currentMenu);
void drawMainMenu();
void drawGPSScreen(bool firstDraw = true, bool updateLatitude = false, bool updateLongitude = false);
void updateGPSFieldsIfChanged(const String& newTime, const String& newLat, const String& newLon);
void updateGPSBatteryIndicator();
void drawFolderScreen(bool firstDraw = false);
void drawMP3Screen(bool firstDraw = false);
void drawMonitoringScreen();
void drawInfoScreen();
void drawExerciseScreen(bool firstDraw = true);
void drawEmergencyScreen(bool firstDraw = true);

// API para popup de caida
void notifyFallDetected();

// Funciones auxiliares para el menú principal
void drawHeaderWithWiFi(const String &title);
void drawMenuIcon(int x, int y, int itemIndex, bool selected, uint16_t bgcolor);
void write_str_bold(uint16_t x, uint16_t y, const char* text, FontDef font, uint16_t color, uint16_t bgcolor);
void drawLine(int x1, int y1, int x2, int y2, uint16_t color);
void drawCircle(int x, int y, int radius, uint16_t color);
void drawCircleOutline(int xc, int yc, int r, uint16_t color);
void fillRectPixels(int x, int y, int w, int h, uint16_t color);
void drawWiFiIcon(int x, int y, bool connected);

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

