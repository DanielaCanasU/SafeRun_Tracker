#pragma once
#include <Arduino.h>
#include <vector>

// Reenvío de tipos usados por varios módulos
class HT_st7735;
class TinyGPSPlus;
class DFRobotDFPlayerMini;
class Adafruit_ADXL345_Unified;
class SX126XLT;
class Botones;
class Display;
class Musica;
class Geolocation;
class DetectorCaida;
class LoRa;
class Bateria;

// Colores usados en Display
#define NARANJA ST7735_COLOR565(243, 91, 4)
#define AZUL_OSCURO ST7735_COLOR565(2, 48, 71)
#define MORADO ST7735_COLOR565(131, 56, 236)
#define ST7735_GRAY ST7735_COLOR565(128, 128, 128)

// Menú y opciones MP3
enum MenuScreen { SCREEN_MAIN_MENU, SCREEN_GPS, SCREEN_MP3_FOLDER, SCREEN_MP3_PLAYER, SCREEN_MONITORING, SCREEN_INFO, SCREEN_EXERCISE, SCREEN_EMERGENCY, SCREEN_DISCONNECTED, SCREEN_COUNT };
enum MP3Option { MP3_PLAY_PAUSE, MP3_NEXT, MP3_VOL_UP, MP3_VOL_DOWN, MP3_PREV, MP3_OPTION_COUNT };

// Estado botones
struct ButtonState {
  volatile unsigned long lastPressTime;
  volatile unsigned long pressStartTime;
  volatile bool isPressed;
  volatile bool wasLongPress;
  volatile uint8_t clickCount;
  volatile unsigned long lastClickTime;
  volatile unsigned long lastVolumeUpdateTime;
};

// Objetos globales (deben ser definidos en un .cpp)
extern HT_st7735 st7735;
extern TinyGPSPlus gps;
extern DFRobotDFPlayerMini dfPlayer;
extern Adafruit_ADXL345_Unified acelerometro;
extern SX126XLT LT;
extern Botones botones;
extern DetectorCaida detectorCaida;
extern Musica musica;
extern Display display;
extern Geolocation geolocation;
extern LoRa loRa;

// Estado global
extern std::vector<int> lista_canciones;

extern bool TRANSMISION_COMPLETADA;
extern bool TRANSMISION_FALLIDA;

// LoRa
extern uint32_t TXPacketCount;
extern unsigned long lastSendTime_LoRa;
extern bool waitingACK;
extern bool receivedACK;
extern int diferencia;

// GPS
extern unsigned long lastSendTime_GPS;
extern String latitude;
extern String longitude;
extern String time_str;
extern bool gpsDataValid;
extern unsigned long lastGPSUpdate;


// Acelerómetro y monitoreo
extern float x, y, z;
extern float time_dato;
extern unsigned long lastReadTime_Acelerometro;
//extern bool free_fall, segunda_condicion_caida, emergencia, impacto;
extern unsigned long time_of_fall, tiempo_de_choque_piso, tiempoimpacto;
extern unsigned long ventana_caida_a_choque, ventana_choque_a_inactividad;
//extern float initialX, initialY, initialZ;
extern bool isCalibrated, isMonitoringActive;
extern bool prev_impacto, prev_free_fall, prev_segunda_condicion_caida, prev_emergencia;

// Menú / MP3
extern volatile bool menuNeedsUpdate;
extern volatile bool needProcessButton;
extern MenuScreen currentScreen;
extern MenuScreen pastScreen;
extern MP3Option currentMP3Option;
extern uint8_t currentVolume;
extern bool isPlaying;
extern uint8_t currentFolder, lastFolder, maxFolders, currentSong;
extern bool folderSelected;

// Menú principal
extern int mainMenuSelection;
extern const int MAIN_MENU_OPTIONS;
extern int lastMainMenuIdx;
extern MenuScreen lastRenderedScreen;

// Botones
extern ButtonState leftButton, rightButton, selectButton;

// Ejercicio
//extern bool exerciseRecording;
extern unsigned long exerciseStartMs;
extern unsigned long exercisePausedAccumMs;
extern float exerciseDistanceMeters;
extern bool lastExercisePosSet;
extern float lastExerciseLat;
extern float lastExerciseLon;

// Emergencia UI
//extern bool emergencyActiveUI;
extern bool emergencyConfirmDeactivate;

extern bool GPSLISTO;  // Flag para indicar si el DFPlayer está listo
extern bool firstplay;
extern bool disconnectedScreenShown;  // Flag para indicar si ya se mostró la pantalla de desconexión