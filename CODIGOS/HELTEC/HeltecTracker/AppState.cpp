#include "AppState.h"
#include <HT_st7735.h>
#include <HT_TinyGPS++.h>
#include <DFRobotDFPlayerMini.h>
#include <Adafruit_ADXL345_U.h>
#include <SX126XLT.h>

// Objetos
HT_st7735 st7735;
TinyGPSPlus gps;
DFRobotDFPlayerMini dfPlayer;
Adafruit_ADXL345_Unified acelerometro = Adafruit_ADXL345_Unified(12345);
SX126XLT LT;

// Variables
std::vector<int> lista_canciones;
float batteryVoltage = 0.0f;
int batteryPercent = 0;

bool TRANSMISION_COMPLETADA = true;
bool TRANSMISION_FALLIDA = false;

uint32_t TXPacketCount = 0;
unsigned long lastSendTime_LoRa = 0;

unsigned long lastSendTime_GPS = 0;
String latitude = "";
String longitude = "";
String time_str = "";

float x = 0, y = 0, z = 0;
float time_dato = 0;
unsigned long lastReadTime_Acelerometro = 0;
bool free_fall = false, segunda_condicion_caida = false, emergencia = false, impacto = false;
unsigned long time_of_fall = 0, tiempo_de_choque_piso = 0, tiempoimpacto = 0;
unsigned long ventana_caida_a_choque = 500;
unsigned long ventana_choque_a_inactividad = 7000;
float initialX = 0, initialY = 0, initialZ = 0;
bool isCalibrated = false, isMonitoringActive = false;
bool prev_impacto = false, prev_free_fall = false, prev_segunda_condicion_caida = false, prev_emergencia = false;

volatile bool menuNeedsUpdate = false;
volatile bool needProcessButton = false;
MenuScreen currentScreen = SCREEN_GPS;
MenuScreen pastScreen = SCREEN_MP3_PLAYER;
MP3Option currentMP3Option = MP3_PLAY_PAUSE;
uint8_t currentVolume = 15;
bool isPlaying = false;
uint8_t currentFolder = 1, lastFolder = 0, maxFolders = 0, currentSong = 1;
bool folderSelected = false;

ButtonState leftButton = {0,0,false,false,0,0,0};
ButtonState rightButton = {0,0,false,false,0,0,0};
ButtonState selectButton = {0,0,false,false,0,0,0};


