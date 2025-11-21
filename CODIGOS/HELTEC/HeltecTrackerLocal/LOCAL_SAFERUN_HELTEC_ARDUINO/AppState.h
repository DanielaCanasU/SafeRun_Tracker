#pragma once
#include <HT_st7735.h>


class HT_st7735;
class SX126XLT;
class TinyGPSPlus;
class LoRa;
class Bateria;
class Geolocation;
class Display;
class Botones;


// Colores usados en Display
#define NARANJA ST7735_COLOR565(243, 91, 4)
#define AZUL_OSCURO ST7735_COLOR565(2, 48, 71)
#define MORADO ST7735_COLOR565(131, 56, 236)
#define ST7735_GRAY ST7735_COLOR565(128, 128, 128)

// Menú y opciones MP3
enum MenuScreen { SCREEN_MAIN_MENU, SCREEN_GPS, SCREEN_MP3_FOLDER, SCREEN_MP3_PLAYER, SCREEN_MONITORING, SCREEN_INFO, SCREEN_EXERCISE, SCREEN_EMERGENCY, SCREEN_COUNT };

// Objetos globales (deben ser definidos en un .cpp)
extern HT_st7735 st7735;
extern TinyGPSPlus gps;
extern Adafruit_ADXL345_Unified acelerometro;
extern SX126XLT LT;
extern Botones botones;
extern Display display;
extern Geolocation geolocation;
extern LoRa loRa;


// Estado global
extern std::vector<int> lista_canciones;
extern float batteryVoltage;
extern int batteryPercent;

// LoRa
extern uint32_t TXPacketCount;
extern unsigned long lastSendTime_LoRa;

// GPS
extern unsigned long lastSendTime_GPS;
extern String latitude;
extern String longitude;
extern String time_str;
extern bool gpsDataValid;
extern unsigned long lastGPSUpdate;

// Botones
extern ButtonState leftButton, rightButton, selectButton, backButton-; 

// Declare the global display object
extern HT_st7735 st7735;