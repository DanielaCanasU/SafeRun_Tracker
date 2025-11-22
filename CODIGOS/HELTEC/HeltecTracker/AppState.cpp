#include "AppState.h"
#include "Buttons.h"
#include "Accel.h"
#include "DFPlayerMod.h"
#include "Display.h"
#include <HT_st7735.h>
#include <HT_TinyGPS++.h>
#include <DFRobotDFPlayerMini.h>
#include <Adafruit_ADXL345_U.h>
#include <SX126XLT.h>
#include "LoRaComm.h"
#include "GPS.h"


// Objetos
HT_st7735 st7735;
TinyGPSPlus gps;
DFRobotDFPlayerMini dfPlayer;
Adafruit_ADXL345_Unified acelerometro = Adafruit_ADXL345_Unified(12345);
SX126XLT LT;
Botones botones;
DetectorCaida detectorCaida;
Musica musica;
Geolocation geolocation;
LoRa loRa;
Display display;

// Variables
std::vector<int> lista_canciones;
// Nuevo flag para indicar que el DFPlayer está listo
extern bool GPSLISTO;
bool GPSLISTO = false;
extern bool firstplay;
bool firstplay = false;
bool TRANSMISION_COMPLETADA = true;
bool TRANSMISION_FALLIDA = false;

uint32_t TXPacketCount = 0;
unsigned long lastSendTime_LoRa = 0;

unsigned long lastSendTime_GPS = 0;
String latitude = "";
String longitude = "";
String time_str = "";
bool gpsDataValid = false;
unsigned long lastGPSUpdate = 0;

float x = 0, y = 0, z = 0;
float time_dato = 0;
unsigned long lastReadTime_Acelerometro = 0;
//bool free_fall = false, segunda_condicion_caida = false, emergencia = false, impacto = false;
//unsigned long time_of_fall = 0, tiempo_de_choque_piso = 0, tiempoimpacto = 0;
unsigned long ventana_caida_a_choque = 500;
unsigned long ventana_choque_a_inactividad = 7000;
//float initialX = 0, initialY = 0, initialZ = 0;
bool isCalibrated = false, isMonitoringActive = false;
bool prev_impacto = false, prev_free_fall = false, prev_segunda_condicion_caida = false, prev_emergencia = false;

volatile bool menuNeedsUpdate = false;
volatile bool needProcessButton = false;
MenuScreen currentScreen = SCREEN_MAIN_MENU;
MenuScreen pastScreen = SCREEN_MP3_PLAYER;
MP3Option currentMP3Option = MP3_PLAY_PAUSE;
uint8_t currentVolume = 15;
bool isPlaying = false;
uint8_t currentFolder = 1, lastFolder = 0, maxFolders = 0, currentSong = 1;
bool folderSelected = false;

// Menú principal
int mainMenuSelection = 0;
const int MAIN_MENU_OPTIONS = 6; // base (GPS, Musica, Monitoreo, Info) — mantendremos 4 visibles; amplíaremos en Display
int lastMainMenuIdx = -1;
MenuScreen lastRenderedScreen = SCREEN_COUNT; // Inicializar con valor inválido

ButtonState leftButton = {0,0,false,false,0,0,0};
ButtonState rightButton = {0,0,false,false,0,0,0};
ButtonState selectButton = {0,0,false,false,0,0,0};

// Ejercicio/Emergencia estado UI
//bool exerciseRecording = false;
unsigned long exerciseStartMs = 0;
unsigned long exercisePausedAccumMs = 0;
float exerciseDistanceMeters = 0.0f;
bool lastExercisePosSet = false;
float lastExerciseLat = 0.0f;
float lastExerciseLon = 0.0f;

//bool emergencyActiveUI = false;
bool emergencyConfirmDeactivate = false;