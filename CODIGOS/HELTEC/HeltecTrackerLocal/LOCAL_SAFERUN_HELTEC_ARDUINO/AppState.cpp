#include "AppState.h"
#include "Buttons.h"
#include <HT_st7735.h>
#include <HT_TinyGPS++.h>
#include <SX126XLT.h>
#include "LoRaComm.h"
#include "GPS.h"


// Objetos
HT_st7735 st7735;
TinyGPSPlus gps;
SX126XLT LT;
Botones botones;
Geolocation geolocation;
LoRa loRa;
Display display;

// Variables
float batteryVoltage = 0.0f;
int batteryPercent = 0;
// Nuevo flag para indicar que el DFPlayer está listo
extern bool GPSLISTO;
bool GPSLISTO = false;
extern bool firstplay;
bool firstplay = false;

uint32_t TXPacketCount = 0;
unsigned long lastSendTime_LoRa = 0;

unsigned long lastSendTime_GPS = 0;
String latitude = "";
String longitude = "";
String time_str = "";
bool gpsDataValid = false;
unsigned long lastGPSUpdate = 0;

volatile bool menuNeedsUpdate = false;
volatile bool needProcessButton = false;
// Menú principal
int mainMenuSelection = 0;
const int MAIN_MENU_OPTIONS = 6; // base (GPS, Musica, Monitoreo, Info) — mantendremos 4 visibles; amplíaremos en Display
int lastMainMenuIdx = -1;
MenuScreen lastRenderedScreen = SCREEN_COUNT; // Inicializar con valor inválido

ButtonState leftButton = {0,0,false,false,0,0,0};
ButtonState rightButton = {0,0,false,false,0,0,0};
ButtonState selectButton = {0,0,false,false,0,0,0};

//bool emergencyActiveUI = false;
bool emergencyConfirmDeactivate = false;