#include "Arduino.h"
#include "WiFi.h"
#include <Wire.h>  
#include "HT_st7735.h" //Pantalla
#include "HT_TinyGPS++.h"  //Gps
#include <HardwareSerial.h> //Puerto Serial
#include <SoftwareSerial.h> 
#include <DFRobotDFPlayerMini.h> // Librería para DFPlayer Mini 
#include <Adafruit_Sensor.h> //Acelerometro
#include <Adafruit_ADXL345_U.h>
#include <vector>
#include <SPI.h>                                         
#include <SX126XLT.h> 
#include "SX1262_Settings.h"    

#define ENVIAR_ACELEROMETRO

//----------------------------------------------------------PINES-----------------------------------------------------------------
//BOTONES
#define BUTTON_LEFT 4   // Pin para botón izquierdo
#define BUTTON_RIGHT 5   // Pin para botón derecho
#define BUTTON_SELECT 18  // Pin para botón de selección/menú


//DFPlayer Mini
#define HELTEC_RX2_PIN 6 // 
#define HELTEC_TX2_PIN 7 //

//Acelerometro
#define ADXL345_SCL_PIN 46
#define ADXL345_SDA_PIN 45

// ---Configuracion GPS ---
#define VGNSS_CTRL Vext

#define VBAT_ADC_PIN 1  // GPIO1 = ADC1_CH0


//---------------------------------------------------------PARAMETROS------------------------------------------------------------
// --- Configuración LoRa RYLR998 ---
#define LORA_BAUD_RATE 115200 // Baud rate para comunicar ESP32 <-> RYLR998 
#define LORA_NETWORK_ID 5   // ID de red (0-255, debe ser el mismo en ambos dispositivos)
#define LORA_ADDRESS_HELTEC 0 // Dirección de este dispositivo (1-65535)
#define LORA_ADDRESS_ESP32  1 // Dirección del dispositivo receptor (ESP32)
#define LORA_BAND 915000000  // Frecuencia en Hz 

// --- Colores ---
#define NARANJA ST7735_COLOR565(243, 91, 4)
#define AZUL_OSCURO ST7735_COLOR565(2, 48, 71)
#define MORADO ST7735_COLOR565(131, 56, 236)
#define ST7735_GRAY ST7735_COLOR565(128, 128, 128)

std::vector<int> lista_canciones;

float batteryVoltage = 0.0;
int batteryPercent = 0;

//---------------------------------------------------- INSTACIACIÓN DE OBJETOS -----------------------------------------------------
HT_st7735 st7735; //Pantalla
uint64_t chipid; //No se
TinyGPSPlus gps; //GPS
HardwareSerial SerialLoRa(2); //Serial Modulo LoRa RYLR998
Adafruit_ADXL345_Unified acelerometro = Adafruit_ADXL345_Unified(12345); //Acelerometro
EspSoftwareSerial::UART DFPlayerSerial; // Crear objeto de EspSoftwareSerial
DFRobotDFPlayerMini dfPlayer; //Reproductor
SX126XLT LT;  //LoRa

//------------------------------------------------VARIABLES-------------------------------------------------------
// ---LoRa
uint8_t TXPacketL;
uint32_t TXPacketCount, startmS, endmS;

//  --- Variables GPS---
unsigned long lastSendTime_LoRa = 0;
unsigned long lastSendTime_GPS = 0;
int counter = 0;
String latitude = "";
String longitude = "";  
String time_str = "";

//------------ Variables detector caida -------------
float time_dato = 0; //Tiempo de variable
unsigned long lastTime = 0;
bool toma_datos = true; //Bandera de toma de datos
float x,y,z = 0;
bool free_fall = false;
bool segunda_condicion_caida = false;
float z_offset = 0;
unsigned long time_of_fall = 0;
unsigned long tiempo_de_choque_piso = 0;
unsigned long tiempoimpacto = 0;
static unsigned long lastReadTime_Acelerometro = 0;
bool emergencia = false;
bool impacto = false;

// Variables para guardar las condiciones iniciales
float initialX = 0;
float initialY = 0;
float initialZ = 0;
bool isCalibrated = false;
bool isMonitoringActive = false;

// Variables para guardar el estado anterior para la actualización de SCREEN_MONITORING
bool prev_impacto = false;
bool prev_free_fall = false;
bool prev_segunda_condicion_caida = false;
bool prev_emergencia = false;

// ------------------------------------- Variables de menú ----------------------------------
enum MenuScreen {
  SCREEN_GPS,
  SCREEN_MP3_FOLDER,  // Nueva pantalla para selección de carpeta
  SCREEN_MP3_PLAYER,  // Renombrado del anterior SCREEN_MP3
  SCREEN_MONITORING, // Nueva pantalla para el estado de monitoreo
  SCREEN_COUNT
};

enum MP3Option {
  MP3_PLAY_PAUSE,
  MP3_NEXT,
  MP3_VOL_UP,
  MP3_VOL_DOWN,
  MP3_PREV,
  MP3_OPTION_COUNT
};


volatile bool menuNeedsUpdate = false;
volatile bool needProcessButton = false;

// Variables para el menú MP3
MenuScreen currentScreen = SCREEN_GPS;

MenuScreen pastScreen = SCREEN_MP3_PLAYER;
MP3Option currentMP3Option = MP3_PLAY_PAUSE;
uint8_t currentVolume = 15; // Volumen inicial MP3
bool isPlaying = false;
uint8_t currentFolder = 1;  // Carpeta actual seleccionada
uint8_t lastFolder = 0;
uint8_t maxFolders;     // Número máximo de carpetas disponibles
uint8_t currentSong = 1;
bool folderSelected = false; // Indica si ya se seleccionó una carpeta


//------ Variables para manejo de botones --------------
unsigned long lastButtonPressTime = 0;
unsigned long buttonPressStartTime = 0;
const unsigned long DOUBLE_CLICK_TIME = 250;  // Reducido a 250ms para doble click
const unsigned long LONG_PRESS_TIME = 800;    // Reducido a 800ms para long press
const unsigned long MENU_SWITCH_TIME = 500;   // Reducido a 500ms para cambio de menú
const unsigned long DEBOUNCE_TIME = 300;       // Tiempo de debounce para evitar rebotes
uint8_t buttonClickCount = 0;
bool buttonLongPressed = false;

struct ButtonState {
  volatile unsigned long lastPressTime;
  volatile unsigned long pressStartTime;
  volatile bool isPressed;
  volatile bool wasLongPress;
  volatile uint8_t clickCount;
  volatile unsigned long lastClickTime;
  volatile unsigned long lastVolumeUpdateTime; // Para control continuo de volumen
};

// Estados de los botones
ButtonState leftButton = {0, 0, false, false, 0, 0, 0};
ButtonState rightButton = {0, 0, false, false, 0, 0, 0};
ButtonState selectButton = {0, 0, false, false, 0, 0, 0};


// ---Constantes---
const unsigned long TRIPLE_CLICK_TIME = 600;   // Tiempo máximo entre clicks para triple click
const unsigned long CLICK_TIMEOUT = 400;       // Tiempo máximo para esperar más clicks
const unsigned long VOLUME_UPDATE_INTERVAL = 200; // Intervalo para actualizar volumen (200ms)

//---------Tiempos de actualizacion------------

const long sendInterval_LoRa = 1000; // Enviar cada 10 segundos
const long sendInterval_GPS = 1000; // Enviar cada 1 segundo
const long timerDelay_Acelerometro = 25; // Intervalo de 0.1 segundos
unsigned long ventana_caida_a_choque = 500;
unsigned long ventana_choque_a_inactividad = 7000;


// ------------------------------------------------------ PROTOTIPOS DE FUNCIONES ----------------------------------------------
// Interrupciones
void IRAM_ATTR handleLeftInterrupt();
void IRAM_ATTR handleRightInterrupt();
void IRAM_ATTR handleSelectInterrupt();

// Menu y Botones
void handleButtons();
void handleGPSScreen();
void handleMP3Screen();
void drawGPSScreen();
void drawMP3Screen(bool firstDraw = false);
void drawMonitoringScreen();  // Prototipo para la nueva pantalla
void handleButtonPress(uint8_t button, bool isLongPress, bool isDoubleClick, bool isTripleClick = false);
void drawFolderScreen(bool firstDraw = false);
void handleFolderScreen();

// Función para procesar los botones fuera de la interrupción
void processButtonPress();

// Modulo LoRa 
bool sendCommand(String command, unsigned long timeout = 1000, bool waitForOK = true);
void configureSX1262();
void sendMessage(const char* message, bool verbose = false);
void checkForIncomingMessage();

// GPS
void getGpsData();
void configureGPS();

// DFPlayer Mini
void configureDFPlayer();
void playSound(uint8_t folder, uint8_t file);
void adjustVolume(uint8_t volume);
void handleMonitoringScreen();  // Prototipo para la función de manejo de la pantalla de monitoreo

//Acelerometro
void checkSetupAcelerometro();
void setupAcelerometro();
float getCalibracionAcelerometro(char eje);
void readAcelerometroData();
void calibrateStandingPosition();
bool isPersonStanding();
float calcularMagnitud(float x, float y,float z);


//Iconos
void drawMinusIcon(uint16_t x, uint16_t y, uint16_t color, uint8_t size);
void drawPlusIcon(uint16_t x, uint16_t y, uint16_t color, uint8_t size);
void drawPlayIcon(uint16_t x, uint16_t y, uint16_t borderColor, uint16_t radius, uint16_t circleColor, uint16_t fillColor);
void drawPauseIcon(uint16_t x, uint16_t y, uint16_t borderColor, uint16_t radius, uint16_t circleColor, uint16_t fillColor);
void drawPrevIcon(uint16_t x, uint16_t y, uint16_t color);
void drawNextIcon(uint16_t x, uint16_t y, uint16_t color);
void drawVolDownIcon(uint16_t x, uint16_t y, uint16_t color);
void drawVolUpIcon(uint16_t x, uint16_t y, uint16_t color);
void drawRoundedRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t radius, uint16_t color);

void drawMenuDots(MenuScreen currentMenu) {
  const int dotRadius = 1;        // Radio del punto pequeño
  const int activeDotRadius = 2;  // Radio del punto activo
  const int dotSpacing = 12;      // Espacio entre puntos
  const int yPosition = 73;      // Posición Y de los puntos (cerca del fondo)
  const int xStart = 80 - dotSpacing/2;  // Posición X del primer punto (centrado)
  
  // Dibujar los puntos
  for(int i = 0; i < 3; i++) {
    int xPos = xStart + i * dotSpacing;
    bool isActive = false;


    
    // Determinar si este punto debe estar activo
    if (i == 0) { // Punto del GPS
      isActive = (currentMenu == SCREEN_GPS);
    } else { // Punto de Música (activo para ambas pantallas de música)
      isActive = (currentMenu == SCREEN_MP3_FOLDER || currentMenu == SCREEN_MP3_PLAYER);
    }
    if (i == 2) {
      isActive = (currentMenu == SCREEN_MONITORING);
    }
    
    // Dibujar el punto con el tamaño y color correspondiente
    int radius = isActive ? activeDotRadius : dotRadius;
    uint16_t color = isActive ? ST7735_WHITE : ST7735_GRAY;
    
    // Dibujar el punto como un círculo relleno
    for(int dy = -radius; dy <= radius; dy++) {
      for(int dx = -radius; dx <= radius; dx++) {
        if(dx*dx + dy*dy <= radius*radius) {
          st7735.st7735_draw_pixel(xPos + dx, yPosition + dy, color);
        }
      }
    }
  }
}

float readBatteryVoltage() {
  int raw = analogRead(VBAT_ADC_PIN);
  float voltage = ((raw / 4095.0) * 3.3) * 4.9;
  // Si hay divisor resistivo, multiplica aquí. Si no, deja así.
  return voltage;
}

int batteryVoltageToPercent(float voltage) {
  // 4.2V = 100%, 3.2V = 0%
  if (voltage >= 4.2) return 100;
  if (voltage <= 3.2) return 0;
  return (int)(((voltage - 3.2) / (4.2 - 3.2)) * 100.0);
}

void setup()
{
  // Configurar pines 
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(2,INPUT_PULLUP);

  analogReadResolution(12); // 12 bits para ESP32

  
  //pinMode(DFPLAYER_RX_PIN, INPUT);
  //pinMode(DFPLAYER_TX_PIN, OUTPUT);
  // Configurar interrupciones para los botones
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), handleLeftInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), handleRightInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_SELECT), handleSelectInterrupt, CHANGE);
  
  // Inicializar estados de los botones
  leftButton = {0, 0, false, false, 0, 0, 0};
  rightButton = {0, 0, false, false, 0, 0, 0};
  selectButton = {0, 0, false, false, 0, 0, 0};

  Serial.begin(115200); //Inicio serial computadora
  Wire.begin(ADXL345_SDA_PIN,ADXL345_SCL_PIN); // I2C Acelerometro
  //Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE); 
  st7735.st7735_init(); //Inicio pantalla
	st7735.st7735_fill_screen(ST7735_BLACK);
  drawGPSScreen(); // Dibuja la pantalla inicial (GPS)
  SPI.begin();
  //setup hardware pins used by device, then check if device is found
  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, DIO2, DIO3, RX_EN, TX_EN, SW, LORA_DEVICE))
  {
    Serial.println(F("LoRa Device found"));
    delay(1000);
  }
  else
  {
    Serial.println(F("No device responding"));
  }


  /*
  
  // Iniciar Serial2 para la comunicación con el módulo LoRa RYLR998
  //SerialLoRa.begin(LORA_BAUD_RATE, SERIAL_8N1, HELTEC_RX2_PIN, HELTEC_TX2_PIN);
  //Serial.println("Puerto Serial LoRa (Serial2) inicializado en pines RX: " + String(HELTEC_RX2_PIN) + ", TX: " + String(HELTEC_TX2_PIN));
  //delay(100); // Pequeña pausa

  // Configurar el módulo RYLR998
  //Serial.println("Configurando RYLR998...");
  if (configureRYLR998()) {
    Serial.println("RYLR998 configurado correctamente.");
  } else {
    Serial.println("Error al configurar RYLR998. Verifique conexiones y baud rate.");
    // Quedarse aquí o intentar reiniciar
    while(1);
  }
  */
  configureSX1262();
  //Configurar GPS
  Serial.println("Configurando GPS...");
  configureGPS();

  //Configurar DFPlayer Mini
  Serial.println("Configurando DFPlayer Mini...");
  //configureDFPlayer();
  Serial.println("Iniciando ADXL345");
  setupAcelerometro();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Procesar botones si es necesario
  processButtonPress();
  

  // Actualizar pantalla según el menú actual
  if (currentScreen == SCREEN_GPS) {
    handleGPSScreen();
    pastScreen == SCREEN_MP3_FOLDER;
  }
  else if (currentScreen == SCREEN_MP3_FOLDER) {
    handleFolderScreen();
    pastScreen = SCREEN_GPS;
  }
  else if (currentScreen == SCREEN_MP3_PLAYER) {
    handleMP3Screen();
    pastScreen = SCREEN_MP3_FOLDER;
  }
  else if (currentScreen == SCREEN_MONITORING) {
    handleMonitoringScreen();
  }

  if (isMonitoringActive){
    if(!isCalibrated) {
      calibrateStandingPosition();
      free_fall = false;
      segunda_condicion_caida = false;
      emergencia = false;
      isCalibrated = true;
      impacto = false;

      // Sincronizar prev_states después de la inicialización
      prev_impacto = impacto;
      prev_free_fall = free_fall;
      prev_segunda_condicion_caida = segunda_condicion_caida;
      prev_emergencia = emergencia;

            // Si estamos en la pantalla de monitoreo, dibujarla con el estado inicial
      if (currentScreen == SCREEN_MONITORING) {
          drawMonitoringScreen();
      }
    }
    else if (millis() - lastReadTime_Acelerometro >= timerDelay_Acelerometro) {
      lastReadTime_Acelerometro = millis();
      Activites activ = acelerometro.readActivites();  //(LEER CAIDA LIBRE, INACTIVIDAD, ACTIVIDAD)
      
      if (impacto)
      {
        tiempoimpacto = millis();
      }
      if ((millis() - time_of_fall >= ventana_caida_a_choque) && (free_fall)) {
        impacto = false;
        tiempoimpacto = 0;
      }

      
      if (activ.isFreeFall)
      {
        Serial.println("Free Fall Detected!");
        free_fall = true;
        time_of_fall = millis();
      }

      //SI MILLIS - TIEMPO DE CAIDA >= 500ms AND SI FREE FALL= TRUE
      if ((millis() - time_of_fall >= ventana_caida_a_choque) && (free_fall)) {
        free_fall = false;
        time_of_fall = 0;
      }

      //SI FREE FALL = TRUE Y ESTA EN ACTIVIDAD   (segunda condicion de caida es actividad y caida libre)
      if(activ.isActivity && free_fall) {
        segunda_condicion_caida = true;
        tiempo_de_choque_piso = millis();
      }

      if(activ.isActivity && impacto) {
        segunda_condicion_caida = true;
        tiempo_de_choque_piso = millis();
      }


      //SI MILLIS - TIEMPO DE CHOQUE >= 7000ms AND SI SEGUNDA CONDICION DE CAIDA = TRUE
      if ((millis() - tiempo_de_choque_piso >= ventana_choque_a_inactividad) && (segunda_condicion_caida)) {
        segunda_condicion_caida = false;
        tiempo_de_choque_piso = 0;
      }

      //Si la segunda condicion es true y esta inactivo entonces se activa la emergencia
      if(activ.isInactivity && segunda_condicion_caida && !isPersonStanding() or activ.isInactivity && impacto && !isPersonStanding()) {
        emergencia = true;
      }

      sensors_event_t event; 
      acelerometro.getEvent(&event);
      x = event.acceleration.x;
      y = event.acceleration.y;
      z = event.acceleration.z;
      time_dato = time_dato + 0.025;
      
      readAcelerometroData();
      // Verificar si algún estado monitorizado ha cambiado y redibujar si es necesario
      bool stateChanged = (impacto != prev_impacto ||
                           free_fall != prev_free_fall ||
                           segunda_condicion_caida != prev_segunda_condicion_caida ||
                           emergencia != prev_emergencia);

      if (stateChanged) {
          if (currentScreen == SCREEN_MONITORING) {
              drawMonitoringScreen();
          }
          // Actualizar los estados previos después de verificar y potencialmente dibujar
          prev_impacto = impacto;
          prev_free_fall = free_fall;
          prev_segunda_condicion_caida = segunda_condicion_caida;
          prev_emergencia = emergencia;
      }
    }
  } else { // if !isMonitoringActive
    // Cuando el monitoreo está desactivado, resetear los estados y actualizar la pantalla si es necesario
    bool actual_state_changed_to_false = false;
    if (impacto) { impacto = false; actual_state_changed_to_false = true; }
    if (free_fall) { free_fall = false; actual_state_changed_to_false = true; }
    if (segunda_condicion_caida) { segunda_condicion_caida = false; actual_state_changed_to_false = true; }
    if (emergencia) { emergencia = false; actual_state_changed_to_false = true; }
    if (isCalibrated) { isCalibrated = false; } // Resetear calibración

    bool prev_states_need_sync = (prev_impacto != impacto || prev_free_fall != free_fall ||
                                  prev_segunda_condicion_caida != segunda_condicion_caida || prev_emergencia != emergencia);

    if (actual_state_changed_to_false || prev_states_need_sync) {
        prev_impacto = impacto; prev_free_fall = free_fall;
        prev_segunda_condicion_caida = segunda_condicion_caida; prev_emergencia = emergencia;
        if (currentScreen == SCREEN_MONITORING) {
            drawMonitoringScreen();
        }
    }
  }


  
  // Procesar datos del GPS
  getGpsData();
  /*
  if(toma_datos) {   
    if (currentTime - lastReadTime_Acelerometro >= timerDelay_Acelerometro) {
      lastReadTime_Acelerometro = currentTime;
      readAcelerometroData();
    }
  }
  */
  
  // Enviar ubicación periódicamente
  if (currentTime - lastSendTime_LoRa >= sendInterval_LoRa) {
    lastSendTime_LoRa = currentTime;
    char message[128];
#ifdef ENVIAR_ACELEROMETRO
    int len = snprintf(message, sizeof(message), "GPS:%s,%s; ST:%d,%d,%d,%d,%d; ACC:%.2f,%.2f,%.2f",
                      latitude.c_str(), longitude.c_str(),
                      isMonitoringActive ? 1 : 0, impacto ? 1 : 0,
                      free_fall ? 1 : 0, segunda_condicion_caida ? 1 : 0,
                      emergencia ? 1 : 0,
                      x, y, z);
#else
    int len = snprintf(message, sizeof(message), "GPS:%s,%s; ST:%d,%d,%d,%d,%d",
                      latitude.c_str(), longitude.c_str(),
                      isMonitoringActive ? 1 : 0, impacto ? 1 : 0,
                      free_fall ? 1 : 0, segunda_condicion_caida ? 1 : 0,
                      emergencia ? 1 : 0);
#endif
    if (len > 0 && len < (sizeof(message) - 1)) {
        message[len] = '*';  // Añade terminador visual
        message[len + 1] = '\0'; // Mantén el string válido
        len++;  // Ahora longitud real incluye '*'
    }

    // Agregar terminador visible (opcional)
    

    
    sendMessage(message, true);
    //Serial.println("X: " + String(x) + ", Y: " + String(y) + ", Z: " + String(z) + ",");
  }
  
  // Escuchar respuestas
  //checkForIncomingMessage();
  batteryVoltage = readBatteryVoltage();
  batteryPercent = batteryVoltageToPercent(batteryVoltage);
  
}
// Implementación de las funciones de menú y botones

void handleButtonPress(uint8_t button, bool isLongPress, bool isDoubleClick, bool isTripleClick) {
  if (currentScreen == SCREEN_MP3_FOLDER) {
    if (button == BUTTON_SELECT && !isLongPress) {
      // Confirmar selección de carpeta y pasar al reproductor
      folderSelected = true;
      currentScreen = SCREEN_MP3_PLAYER;
      if (!isPlaying || currentFolder != lastFolder){
        dfPlayer.playFolder(currentFolder, 1); // Comenzar reproducción desde la primera canción
        isPlaying = true;
        currentSong = 1;
      }
      drawMP3Screen(true);
      Serial.print("Cancion:");
      Serial.println(currentSong);
      Serial.print("Playlist:");
      Serial.println(currentFolder);
      return;
    } 
    else if (button == BUTTON_LEFT && !isLongPress) {
      // Carpeta anterior
      if (currentFolder > 1) {
        //lastFolder = currentFolder;
        currentFolder--;
        drawFolderScreen();
      }
    }
    else if (button == BUTTON_RIGHT && !isLongPress) {
      // Siguiente carpeta
      if (currentFolder < maxFolders) {
        currentFolder++;
        drawFolderScreen();
      }
      else {
        currentFolder = 1;
        drawFolderScreen();
      }
    }
    //return;
  }
  /*
  if (currentScreen == SCREEN_MONITORING) {
    if ((button == BUTTON_SELECT) && (!isLongPress)){
      isMonitoringActive = !isMonitoringActive;
      drawMonitoringScreen();
    }
    return;
  }
  */
  
  if (button == BUTTON_SELECT) {
    if (isLongPress) {
      // Cambiar pantalla
      if (currentScreen == SCREEN_MP3_FOLDER) {
        currentScreen = (MenuScreen)((currentScreen + 2) % SCREEN_COUNT);
      }
      
      else if(currentScreen == SCREEN_MP3_PLAYER){
        currentScreen = SCREEN_MP3_FOLDER;  
      }
      else {
        currentScreen = (MenuScreen)((currentScreen + 1) % SCREEN_COUNT);
        Serial.println(currentScreen);
      }

      if (currentScreen == SCREEN_GPS) {
        drawGPSScreen();
      } 
      else if (currentScreen == SCREEN_MP3_FOLDER) {
        // Si volvemos al menú MP3, empezar por la selección de carpeta
        folderSelected = false;
        drawFolderScreen(true);
      }
      else if (currentScreen == SCREEN_MONITORING) {
                // Sincronizar prev_states con los estados actuales antes de dibujar por primera vez
        prev_impacto = impacto;
        prev_free_fall = free_fall;
        prev_segunda_condicion_caida = segunda_condicion_caida;
        prev_emergencia = emergencia;
        drawMonitoringScreen(); // Dibuja la pantalla de monitoreo
      }
      Serial.println(currentScreen);
    } 
    else if (isTripleClick) {
      if (currentScreen == SCREEN_MP3_PLAYER) {
        dfPlayer.reset();
        drawMP3Screen();
      }
    }
    else if (isDoubleClick) {
      if (currentScreen == SCREEN_MP3_PLAYER) {
        dfPlayer.enableLoop();
        drawMP3Screen();
      }
    }
    else { // Single click
      if (currentScreen == SCREEN_MP3_PLAYER) {
        switch (currentMP3Option) {
          case MP3_PLAY_PAUSE:
            isPlaying = !isPlaying;
            if (isPlaying) {
              dfPlayer.start();
            } else {
              dfPlayer.pause();
            }
            break;
          case MP3_NEXT:
            //dfPlayer.next();
            isPlaying = true;
            currentSong++;
            Serial.print("Cancion:");
            Serial.println(currentSong);
            if(currentSong > lista_canciones[currentFolder - 1]) {
              currentSong = 1;
              dfPlayer.playFolder(currentFolder, 1); // Comenzar reproducción desde la primera canción

              break;
              isPlaying = true;
            }
            dfPlayer.next();
            Serial.print("Cancion:");
            Serial.println(currentSong);
            Serial.print("Playlist:");
            Serial.println(currentFolder);
            break;
          case MP3_PREV:
            currentSong--;
            isPlaying = true;
            if(currentSong < 1) {
              currentSong = lista_canciones[currentFolder - 1];
              dfPlayer.playFolder(currentFolder, lista_canciones[currentFolder - 1]); // Comenzar reproducción desde la última canción
              break;
            }
            dfPlayer.previous();
            Serial.print("Cancion:");
            Serial.println(currentSong);
            Serial.print("Playlist:");
            Serial.println(currentFolder);
            break;
          case MP3_VOL_UP:
            if (currentVolume < 30) {
              currentVolume++;
              dfPlayer.volume(currentVolume);
            }
            break;
          case MP3_VOL_DOWN:
            if (currentVolume > 0) {
              currentVolume--;
              dfPlayer.volume(currentVolume);
            }
            break;
        }
       drawMP3Screen();
      }
      if (currentScreen == SCREEN_MONITORING) {
        if (!isMonitoringActive) { // Si se acaba de desactivar
            impacto = false;
            free_fall = false;
            segunda_condicion_caida = false;
            emergencia = false;
            isCalibrated = false; // Resetear calibración también
        }
        // Sincronizar prev_states antes de este redibujo inmediato
        prev_impacto = impacto;
        prev_free_fall = free_fall;
        prev_segunda_condicion_caida = segunda_condicion_caida;
        prev_emergencia = emergencia;
        isMonitoringActive = !isMonitoringActive;
        drawMonitoringScreen();
      }
    }
  } 
  //else if (currentScreen == SCREEN_MP3_PLAYER) {
  else {
    if (isTripleClick) {
      if (button == BUTTON_LEFT) {
        //dfPlayer.previous();
        //isPlaying = true;
        //drawMP3Screen();
        currentSong--;
        isPlaying = true;
        if(currentSong < 1) {
          currentSong = lista_canciones[currentFolder - 1];
          dfPlayer.playFolder(currentFolder, lista_canciones[currentFolder - 1]); // Comenzar reproducción desde la última canción
        }
        else {
        dfPlayer.previous();
        Serial.print("Cancion:");
        Serial.println(currentSong);
        Serial.print("Playlist:");
        Serial.println(currentFolder);
        }
      } else if (button == BUTTON_RIGHT) {
        currentSong++;
        if(currentSong > lista_canciones[currentFolder - 1]) {
          currentSong = 1;
          dfPlayer.playFolder(currentFolder, 1); // Comenzar reproducción desde la primera canción
          isPlaying = true;
        }
        else {
          isPlaying = true;
          dfPlayer.next();
        }
        Serial.print("Cancion:");
        Serial.println(currentSong);
        Serial.print("Playlist:");
        Serial.println(currentFolder);
        //dfPlayer.next();
        //isPlaying = true;
        //drawMP3Screen();
      }
    }
    else if (isDoubleClick) {
      if (button == BUTTON_LEFT) {
        dfPlayer.pause();
        isPlaying = false;
        //drawMP3Screen();
      } else if (button == BUTTON_RIGHT) {
        dfPlayer.start();
        isPlaying = true;
        //drawMP3Screen();
      }
    }
    else if (!isLongPress) { // Single click
      if(currentScreen == SCREEN_MP3_PLAYER){
        if (button == BUTTON_RIGHT) {
          currentMP3Option = (MP3Option)((currentMP3Option + 1) % MP3_OPTION_COUNT);
          //drawMP3Screen();
        } else if (button == BUTTON_LEFT) {
          currentMP3Option = (MP3Option)((currentMP3Option + MP3_OPTION_COUNT - 1) % MP3_OPTION_COUNT);
          //drawMP3Screen();
        }
      }
    }
    if (currentScreen == SCREEN_MP3_PLAYER) {
      drawMP3Screen();
    }
  }
}


void drawGPSScreen() {
  st7735.st7735_fill_screen(ST7735_BLACK);
  st7735.st7735_write_str(0, 0, "GPS Screen", Font_7x10, ST7735_GREEN);
  st7735.st7735_write_str(0, 20, time_str, Font_7x10, ST7735_GREEN);
  st7735.st7735_write_str(0, 40, latitude, Font_7x10, ST7735_GREEN);
  st7735.st7735_write_str(0, 60, longitude, Font_7x10, ST7735_GREEN);
  // Mostrar porcentaje de batería
  char voltStr[20];
  snprintf(voltStr, sizeof(voltStr), "Bat: %d%%", batteryPercent);
  st7735.st7735_write_str(80, 0, voltStr, Font_7x10, ST7735_YELLOW);
  drawMenuDots(SCREEN_GPS);
  drawMenuDots(SCREEN_GPS);
}

void drawMP3Screen(bool firstDraw) {
  // --- Variables estáticas para optimización (no redibujar si no cambia) ---
  static uint8_t lastVolume = 255; // Inicializar a un valor inválido
  static MP3Option lastOption = MP3_OPTION_COUNT;
  static bool lastPlayingState = !isPlaying; // Inicializar al estado opuesto
  static uint8_t lastSong = 0; // Para detectar cambios de canción
  //static uint8_t lastFolder = 0; // Para detectar cambios de carpeta
  int mp3base = 36; // Posición base X para los iconos 
  int volBaseX = 30; // posición X inicial barra volumen
  int baseYcontrols = 30; 
  int volBaseY = baseYcontrols + 33; // posición Y base barra volumen
  

  // --- Dibujo inicial ---
  if (firstDraw) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    lastVolume = 255; // Forzar redibujo del volumen en el primer draw
    lastOption = MP3_OPTION_COUNT; // Forzar redibujo de iconos
    lastPlayingState = !isPlaying; // Forzar redibujo play/pause
    lastSong = 0; // Forzar redibujo del track
    lastFolder = 0; // Forzar redibujo de la carpeta
    drawMenuDots(SCREEN_MP3_PLAYER);
  }

  // Actualizar texto del track si cambió la canción o la carpeta
  if (firstDraw || lastSong != currentSong || lastFolder != currentFolder) {
    st7735.st7735_fill_rectangle(36, 5, 92, 10, ST7735_BLACK); // Limpiar área del texto
    String trackText = String(currentFolder) + ". Track " + String(currentSong);
    // Dibujar el texto múltiples veces con pequeños desplazamientos para crear efecto negrita
    st7735.st7735_write_str(36, 5, trackText.c_str(), Font_7x10, NARANJA);
    st7735.st7735_write_str(37, 5, trackText.c_str(), Font_7x10, NARANJA);
    lastSong = currentSong;
    lastFolder = currentFolder;
  }

  // --- BARRA DE VOLUMEN ---
  // Solo redibujar si el volumen cambió
  if (currentVolume != lastVolume || firstDraw) {
     // Dibuja el volumen como barras verticales tipo ecualizador
    // --- Constantes de la barra de volumen ---
  const int cursorRadius = 3;     // Radio del círculo indicador
  const uint16_t filledColor = ST7735_GREEN; // Color a la izquierda del cursor
  const uint16_t emptyColor = ST7735_WHITE;  // Color a la derecha del cursor
  const uint16_t cursorColor = NARANJA; // Color del círculo cursor
  int barHeight = 2; // altura de cada barra
  int volBarEndX = 127;
  const int volBarWidth = volBarEndX - volBaseX + 1;


  // Limpia el área del volumen
  st7735.st7735_fill_rectangle(volBaseX, volBaseY - 7, 97, 11, AZUL_OSCURO);

  // Dibuja una barra por cada nivel de volumen 
  for (int i = 0; i < 97; i++) {
      uint16_t color = (i < currentVolume*3.23) ? NARANJA : ST7735_WHITE;
      for (int h = 0; h < barHeight; h++) {
          st7735.st7735_draw_pixel(volBaseX + i, volBaseY + h, color);
      }
  }

    // --- Calcular posición X del centro del cursor ---
    // Mapea el volumen (0-30) al rango de píxeles de la barra
    // Ajustamos el rango para que el *centro* del cursor quede entre el inicio+radio y fin-radio
    int cursorX = map(currentVolume, 0, 30, volBaseX + cursorRadius, volBarEndX - cursorRadius);
    // Asegurarse de que el cursor no se salga por errores de redondeo
    cursorX = constrain(cursorX, volBaseX + cursorRadius, volBarEndX - cursorRadius);
    // --- Dibujar el cursor (círculo) ---
    for (int dy = -cursorRadius; dy <= cursorRadius; dy++) {
        for (int dx = -cursorRadius; dx <= cursorRadius; dx++) {
            if (dx*dx + dy*dy <= cursorRadius*cursorRadius) {
                st7735.st7735_draw_pixel(cursorX + dx, volBaseY + dy, cursorColor);
            }
        }
    }
    lastVolume = currentVolume; // Actualizar el último volumen dibujado
  }
  // --- FIN BARRA DE VOLUMEN ---


  // --- Redibujar Iconos (solo si la opción o estado de reproducción cambió) ---

  if (currentMP3Option != lastOption || isPlaying != lastPlayingState || firstDraw) {
    // Play/Pause en el centro
    st7735.st7735_fill_rectangle(mp3base + 24, baseYcontrols - 5, 30, 30, ST7735_BLACK); // Limpiar área
    uint16_t playPauseCircleColor = (currentMP3Option == MP3_PLAY_PAUSE) ? MORADO : ST7735_WHITE; //Color si esta seleccionado o no
    if (isPlaying) {
      drawPauseIcon(mp3base + 41, baseYcontrols + 10, ST7735_BLACK, 12, playPauseCircleColor, ST7735_BLACK); 
    } else {
      drawPlayIcon(mp3base + 41, baseYcontrols + 10, ST7735_BLACK, 12, playPauseCircleColor, ST7735_BLACK); 
    }

    // Anterior a la izquierda
    st7735.st7735_fill_rectangle(mp3base - 10, baseYcontrols + 5, 16, 16, ST7735_BLACK); // Limpiar área 
    drawPrevIcon(mp3base, baseYcontrols + 5, currentMP3Option == MP3_PREV ? MORADO : ST7735_WHITE);

    // Siguiente a la derecha
    st7735.st7735_fill_rectangle(mp3base + 70, baseYcontrols + 5, 16, 16, ST7735_BLACK); // Limpiar área
    drawNextIcon(mp3base + 76, baseYcontrols + 5, currentMP3Option == MP3_NEXT ? MORADO : ST7735_WHITE);

    // Vol Down abajo izquierda
    st7735.st7735_fill_rectangle(mp3base - 22, volBaseY - 4, 16, 16, AZUL_OSCURO); // Limpiar área
    drawMinusIcon(mp3base - 22, volBaseY - 4, currentMP3Option == MP3_VOL_DOWN ? MORADO : ST7735_WHITE, 8);


    // Vol Up abajo derecha
    st7735.st7735_fill_rectangle(mp3base + 104, volBaseY - 4, 16, 16, AZUL_OSCURO); // Limpiar área
    drawPlusIcon(mp3base + 104, volBaseY - 4, currentMP3Option == MP3_VOL_UP ? MORADO : ST7735_WHITE, 8);

    lastOption = currentMP3Option; // Actualizar última opción dibujada
    lastPlayingState = isPlaying; // Actualizar último estado de reproducción
  }
}

// --- Funciones para la pantalla de monitoreo ---

void drawMonitoringScreen() {
  st7735.st7735_fill_screen(ST7735_BLACK);
  st7735.st7735_write_str(0, 0, "Monitoreo:", Font_7x10, ST7735_WHITE);
  
  const char* statusText = isMonitoringActive ? "Activo" : "No Activo";
  st7735.st7735_write_str(70, 0, statusText, Font_7x10, isMonitoringActive ? ST7735_GREEN : ST7735_RED);

  int y_offset = 15; // Espacio vertical entre líneas
  int current_y = 15;

  // Mostrar estado de 'impacto'
  st7735.st7735_write_str(0, current_y, "Impacto:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, impacto ? "SI" : "NO", Font_7x10, impacto ? ST7735_GREEN : ST7735_RED);
  current_y += y_offset;

  // Mostrar estado de 'free_fall'
  st7735.st7735_write_str(0, current_y, "CaidaLibre:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, free_fall ? "SI" : "NO", Font_7x10, free_fall ? ST7735_GREEN : ST7735_RED);
  current_y += y_offset;

  // Mostrar estado de 'segunda_condicion_caida'
  st7735.st7735_write_str(0, current_y, "CaidaConf:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, segunda_condicion_caida ? "SI" : "NO", Font_7x10, segunda_condicion_caida ? ST7735_GREEN : ST7735_RED);
  current_y += y_offset;

  // Mostrar estado de 'emergencia'
  st7735.st7735_write_str(0, current_y, "Emergencia:", Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(70, current_y, emergencia ? "SI" : "NO", Font_7x10, emergencia ? ST7735_GREEN : ST7735_RED);

  drawMenuDots(SCREEN_MONITORING);
}

//****************************************************************************************************************************************************************************************************


void handleGPSScreen() {
  // Actualizar datos GPS en pantalla
  if (currentScreen != pastScreen) {
    pastScreen = currentScreen;
    drawGPSScreen();
  }
}

void handleMP3Screen() {
  // La actualización de la pantalla MP3 se maneja en los eventos de botón
}

void handleMonitoringScreen() {

  // La actualización se maneja en los eventos de botón, específicamente en handleButtonPress
  // No hay necesidad de lógica adicional aquí por ahora, pero podría agregarse si es necesario

}

void drawFolderScreen(bool firstDraw) {
  static int lastFolder = 0;
  
  if (firstDraw) {

    st7735.st7735_fill_screen(ST7735_BLACK);
    drawMenuDots(SCREEN_MP3_FOLDER);

    
    // Dibujar un rectángulo morado grande para el título "PLAYLIST"
    drawRoundedRectangle(30, 20, 100, 30, 4, MORADO);
    
    // Escribir "PLAYLIST" centrado en el rectángulo morado
    st7735.st7735_write_str(45, 31, "PLAYLIST", Font_7x10, ST7735_WHITE, MORADO);
    
    // Escribir el número de playlist grande en la parte superior derecha
    String folderNum = String(currentFolder);
    st7735.st7735_write_str(110, 26, folderNum.c_str(), Font_11x18, ST7735_WHITE, MORADO);
    
    // Escribir el mensaje "PRESIONE ENTER" en la parte inferior
    st7735.st7735_write_str(15, 58, "Select para iniciar", Font_7x10, ST7735_WHITE);
    
    lastFolder = currentFolder;
  }
  else if (lastFolder != currentFolder)
  {
    // Solo actualizar el número de la playlist cuando cambia
    drawRoundedRectangle(110, 28, 18, 18, 4, MORADO); // Limpiar área del número
    String folderNum = String(currentFolder);
    st7735.st7735_write_str(110, 26, folderNum.c_str(), Font_11x18, ST7735_WHITE, MORADO);
    lastFolder = currentFolder;
  }
}

void handleFolderScreen() {
  // La actualización se maneja en los eventos de botón
}

// --- Funciones Auxiliares para RYLR998 ---



bool sendCommand(String command, unsigned long timeout, bool waitForOK) {
  Serial.print("Enviando Comando: ");
  Serial.println(command);
  SerialLoRa.println(command);

  unsigned long startTime = millis();
  String response = "";
  bool okReceived = false;

  while (millis() - startTime < timeout) {
    if (SerialLoRa.available()) {
      char c = SerialLoRa.read();
      response += c;
      if (response.endsWith("\r\n")) {
        Serial.print("Respuesta Modulo: ");
        Serial.print(response); // Imprime la línea completa recibida
        if (waitForOK && response.startsWith("+OK")) {
          okReceived = true;
          // Podríamos salir aquí si solo esperamos OK, pero leamos la respuesta completa
        }
        if (!waitForOK && response.length() > 0) {
            // Si no esperamos OK, cualquier respuesta es suficiente para salir pronto
            // O podríamos analizar la respuesta específica si fuera necesario
        }
        response = ""; // Limpiar para la siguiente línea si hay más
      }
    }
  }

  if (waitForOK) {
      if (okReceived) {
          Serial.println("Comando OK recibido.");
          return true;
      } else {
          Serial.println("Timeout o error esperando OK.");
          return false;
      }
  }
  return true; // Si no esperábamos OK, asumimos éxito si el comando se envió
}


void configureSX1262() {
  //***************************************************************************************************
  //Setup LoRa device
  //***************************************************************************************************
  LT.setMode(MODE_STDBY_RC);
  LT.setRegulatorMode(USE_DCDC);
  LT.setPaConfig(0x04, PAAUTO, LORA_DEVICE);
  LT.setDIO3AsTCXOCtrl(TCXO_CTRL_3_3V);
  LT.calibrateDevice(ALLDevices);                //is required after setting TCXO
  LT.calibrateImage(Frequency);
  LT.setDIO2AsRfSwitchCtrl();
  LT.setPacketType(PACKET_TYPE_LORA);
  LT.setRfFrequency(Frequency, Offset);
  LT.setModulationParams(SpreadingFactor, Bandwidth, CodeRate, Optimisation);
  LT.setBufferBaseAddress(0, 0);
  LT.setPacketParams(8, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_NORMAL);
  LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_TX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);   //set for IRQ on TX done and timeout on DIO1
  LT.setHighSensitivity();  //set for maximum gain
  LT.setSyncWord(LORA_MAC_PRIVATE_SYNCWORD);

}

void sendMessage(const char* message, bool verbose) {
  int TXPacketL = 0;
  TXPacketL = strlen(message);
  if(verbose) {
    Serial.print("Contenido de message: ");
    Serial.println(message);
    Serial.print("Bytes como int: ");
    for (int i = 0; i < strlen(message); i++) {
      Serial.print((int)message[i]);
      Serial.print(" ");
    }
    Serial.println();


    Serial.print("Longitud real del mensaje: ");
    Serial.println(TXPacketL);
    Serial.print(TXpower);                                       //print the transmit power defined
    Serial.print(F("dBm "));
    Serial.print(F("Packet> "));
    Serial.flush();
  }
  startmS =  millis();   
  //LT.setMode(MODE_STDBY_RC);  // Asegura que esté listo
  //LT.setMode(MODE_TX);
  delay(10);                                 //start transmit timer
  if (LT.transmit((uint8_t *)message, TXPacketL, 10000, TXpower, WAIT_TX))   // 0 = no bloqueante
  {
    endmS = millis();                                          //packet sent, note end time
    //if here packet has been sent OK
    uint16_t localCRC;
    if(verbose){
      Serial.print(F("  BytesSent,"));
      Serial.print(TXPacketL);                             //print transmitted packet length
      localCRC = LT.CRCCCITT((uint8_t *)message, TXPacketL, 0xFFFF);
      Serial.print(F("  CRC,"));
      Serial.print(localCRC, HEX);                         //print CRC of transmitted packet
      Serial.print(F("  TransmitTime,"));
      Serial.print(endmS - startmS);                       //print transmit time of packet
      Serial.print(F("mS"));
      Serial.print(F("  PacketsSent,"));
      Serial.print(TXPacketCount); 
    }
  }
  else
  {
    //if here there was an error transmitting packet
    uint16_t IRQStatus;
    IRQStatus = LT.readIrqStatus(); 
    if(verbose){                     //read the the interrupt register
      Serial.print(F(" SendError,"));
      Serial.print(F("Length,"));
      Serial.print(TXPacketL);                             //print transmitted packet length
      Serial.print(F(",IRQreg,"));
      Serial.print(IRQStatus, HEX);
    }                        //print IRQ status
    LT.printIrqStatus();                                    //transmit packet returned 0, there was an error
  }
   // Nota: AT+SEND devuelve +OK casi inmediatamente. La transmisión real puede tardar.
   // El módulo devuelve +SENT cuando el paquete se ha enviado por aire.
   // Podrías añadir lógica para esperar el +SENT si es necesario.
 }

void checkForIncomingMessage() {
  String receivedData = "";
  while (SerialLoRa.available()) {
    char c = SerialLoRa.read();
    receivedData += c;
    // Los mensajes del RYLR998 terminan en \r\n
    if (receivedData.endsWith("\r\n")) {
       Serial.print("Datos Raw Recibidos LoRa: ");
       Serial.print(receivedData);
       // Parsear el mensaje +RCV=addr,len,data,rssi,snr
       if (receivedData.startsWith("+RCV=")) {
         // Remover el prefijo "+RCV="
         receivedData.remove(0, 5);
         // Separar los campos por coma
         int firstComma = receivedData.indexOf(',');
         int secondComma = receivedData.indexOf(',', firstComma + 1);
         int thirdComma = receivedData.indexOf(',', secondComma + 1);
         int fourthComma = receivedData.indexOf(',', thirdComma + 1);

         if (firstComma != -1 && secondComma != -1 && thirdComma != -1 && fourthComma != -1) {
           String addrStr = receivedData.substring(0, firstComma);
           String lenStr = receivedData.substring(firstComma + 1, secondComma);
           String msg = receivedData.substring(secondComma + 1, thirdComma);
           String rssiStr = receivedData.substring(thirdComma + 1, fourthComma);
           String snrStr = receivedData.substring(fourthComma + 1);
           snrStr.trim(); // Quitar \r\n

           Serial.println("--- Mensaje LoRa Recibido ---");
           Serial.println("De Addr: " + addrStr);
           Serial.println("Longitud: " + lenStr);
           Serial.println("Mensaje: " + msg);
           Serial.println("RSSI: " + rssiStr);
           Serial.println("SNR: " + snrStr);
           Serial.println("-----------------------------");

           // Aquí podrías procesar el mensaje recibido, por ejemplo, enviar una confirmación
         } else {
            Serial.println("Error parseando mensaje RCV.");
         }
       } else if (receivedData.startsWith("+OK")) {
           // Ignorar OK de comandos previos
       } else if (receivedData.startsWith("+SENT")) {
           Serial.println("Confirmacion de envio (+SENT) recibida.");
       } else if (receivedData.startsWith("+ERR")) { 
           Serial.println("Error reportado por el modulo RYLR998.");
       }
       // Resetear buffer para la próxima línea
       receivedData = "";
    }
     delay(1); // Pequeña pausa para no saturar
  }
}

//--- GPS ---

void configureGPS() {
	pinMode(VGNSS_CTRL,OUTPUT);
	digitalWrite(VGNSS_CTRL,HIGH);
	Serial1.begin(115200,SERIAL_8N1,33,34);  
  delay(100);
}

void getGpsData() {
  while(Serial1.available()>0)
    {
      if(Serial1.peek()!='\n')
      {
        gps.encode(Serial1.read());
      }
      else
      {
        Serial1.read();

        if(gps.time.second()==0) {
          continue;
        }
        String new_time_str = String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) + ":" + String(gps.time.centisecond());
        String new_latitude = "LAT: " + String(gps.location.lat(), 6);
        String new_longitude = "LON: " + String(gps.location.lng(), 6);

        // Solo actualizar si los valores han cambiado
        if (new_time_str != time_str) {
            if (currentScreen == SCREEN_GPS) {
              st7735.st7735_fill_rectangle(0, 20, 128, 10, ST7735_BLACK); // Limpiar solo el área del tiempo
              st7735.st7735_write_str(0, 20, new_time_str, Font_7x10, ST7735_GREEN);
            }
          time_str = new_time_str;
        }
        
        if (new_latitude != latitude) {
          if (currentScreen == SCREEN_GPS) {
            st7735.st7735_fill_rectangle(0, 40, 128, 10, ST7735_BLACK); // Limpiar solo el área de latitud
            st7735.st7735_write_str(0, 40, new_latitude, Font_7x10, ST7735_GREEN);
          }
          latitude = new_latitude;
        }
        
        if (new_longitude != longitude) {
          if (currentScreen == SCREEN_GPS) {
            st7735.st7735_fill_rectangle(0, 60, 128, 10, ST7735_BLACK); // Limpiar solo el área de longitud
            st7735.st7735_write_str(0, 60, new_longitude, Font_7x10, ST7735_GREEN);
          }
          longitude = new_longitude;
        }


        while(Serial1.read()>0);
      }
    }
  }

//--- DFPlayer Mini ---
void configureDFPlayer() {

  // Inicializar EspSoftwareSerial para DFPlayer
  DFPlayerSerial.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, HELTEC_RX2_PIN, HELTEC_TX2_PIN, false, 256);
  //SerialMP3.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
  delay(1000); // Dar tiempo al DFPlayer para inicializarse
  
  if (!dfPlayer.begin(DFPlayerSerial, true, false)) { // Use true para modo debug
    Serial.println("Error iniciando DFPlayer Mini:");
    Serial.println("1. Por favor revise las conexiones!");
    Serial.println("2. Por favor inserte la tarjeta SD!");
    while(true);
  }
  Serial.println("DFPlayer Mini en línea.");
  
  dfPlayer.setTimeOut(500); //Establecer tiempo de espera a 500ms
  dfPlayer.volume(20);  //Establecer volumen inicial (0-30)
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL); //Ecualizador normal
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD); //Usar tarjeta SD como dispositivo

  for (int i = 1; i <= 255; i++) {
      Serial.print("Carpeta:");
      Serial.println(i);
      int a = dfPlayer.readFileCountsInFolder(i);
      delay(500);
      Serial.print("Canciones:");
      Serial.println(a);
      delay(100);
      if(a > 0) {
        lista_canciones.push_back(a);
      }

      else if(a == 0) {
        break;
      }
    }
  maxFolders = lista_canciones.size();
}

void playSound(uint8_t folder, uint8_t file) {
  Serial.printf("Reproduciendo carpeta %d, archivo %d\n", folder, file);
  dfPlayer.playFolder(folder, file);
}

void adjustVolume(uint8_t volume) {
  if(volume <= 30) {
    Serial.printf("Ajustando volumen a %d\n", volume);
    dfPlayer.volume(volume);
  }
}

//Acelerometro

void checkSetupAcelerometro()
{
  Serial.print("Free Fall Threshold = "); Serial.println(acelerometro.getFreeFallThreshold());
  Serial.print("Free Fall Duration = "); Serial.println(acelerometro.getFreeFallDuration());
  Serial.println('.');
  Serial.print("Data Rate:    "); 
 
  switch(acelerometro.getDataRate())
  {
    case ADXL345_DATARATE_3200_HZ:
      Serial.print  ("3200 "); 
      break;
    case ADXL345_DATARATE_1600_HZ:
      Serial.print  ("1600 "); 
      break;
    case ADXL345_DATARATE_800_HZ:
      Serial.print  ("800 "); 
      break;
    case ADXL345_DATARATE_400_HZ:
      Serial.print  ("400 "); 
      break;
    case ADXL345_DATARATE_200_HZ:
      Serial.print  ("200 "); 
      break;
    case ADXL345_DATARATE_100_HZ:
      Serial.print  ("100 "); 
      break;
    case ADXL345_DATARATE_50_HZ:
      Serial.print  ("50 "); 
      break;
    case ADXL345_DATARATE_25_HZ:
      Serial.print  ("25 "); 
      break;
    case ADXL345_DATARATE_12_5_HZ:
      Serial.print  ("12.5 "); 
      break;
    case ADXL345_DATARATE_6_25HZ:
      Serial.print  ("6.25 "); 
      break;
    case ADXL345_DATARATE_3_13_HZ:
      Serial.print  ("3.13 "); 
      break;
    case ADXL345_DATARATE_1_56_HZ:
      Serial.print  ("1.56 "); 
      break;
    case ADXL345_DATARATE_0_78_HZ:
      Serial.print  ("0.78 "); 
      break;
    case ADXL345_DATARATE_0_39_HZ:
      Serial.print  ("0.39 "); 
      break;
    case ADXL345_DATARATE_0_20_HZ:
      Serial.print  ("0.20 "); 
      break;
    case ADXL345_DATARATE_0_10_HZ:
      Serial.print  ("0.10 "); 
      break;
    default:
      Serial.print  ("???? "); 
      break;
  }  
  Serial.println(" Hz");  

  Serial.print ("Range:         +/- ");
 
  switch(acelerometro.getRange())
  {
    case ADXL345_RANGE_16_G:
      Serial.print  ("16 "); 
      break;
    case ADXL345_RANGE_8_G:
      Serial.print  ("8 "); 
      break;
    case ADXL345_RANGE_4_G:
      Serial.print  ("4 "); 
      break;
    case ADXL345_RANGE_2_G:
      Serial.print  ("2 "); 
      break;
    default:
      Serial.print  ("?? "); 
      break;
  } 
  Serial.println(" g"); 

}

void setupAcelerometro() {
  if (!acelerometro.begin())
  {
    Serial.println("No se detectó el sensor ADXL345");
    delay(500);
  }
  else {
    // Values for Free Fall detection
    acelerometro.setFreeFallThreshold(0.38); // Recommended 0.3 -0.6 g
    acelerometro.setFreeFallDuration(0.06);  //Recomendado 2 g/0.5 g
    acelerometro.setActivityXYZ(1,0);
    acelerometro.setInactivityThreshold(0.1875);
    acelerometro.setTimeInactivity(5);
    acelerometro.setInactivityXYZ(1,1);
    acelerometro.setDataRate(ADXL345_DATARATE_100_HZ);// Recommended 0.1 s
    acelerometro.setActivityThreshold(1.7); //

    // Select INT 1 for get activities
    acelerometro.useInterrupt(ADXL345_INT1);

    checkSetupAcelerometro();

  }
}

float getCalibracionAcelerometro(char eje){
  float numReadings = 500;
  int Z_out;
  float Z_offset;
  if (eje == 'z') {
      float zSum = 0;
      Serial.print("Beginning Calibration");
      Serial.println();

      for (int i = 0; i < numReadings; i++) {
        Serial.print(i);
        Serial.println();
        Z_out = acelerometro.getZ();
        zSum += Z_out;
      }

      Z_offset = (256 - (zSum / numReadings)) / 4;
      Serial.print("Z_offset= " );
      Serial.print(Z_offset);
      Serial.println();
      delay(1000);
      return Z_offset;
  }

  else if (eje == 'x') {
    return 0;
  }
  else if (eje == 'y') {
    return 0;
  }

  else {
    return 0;
  }
}
/*
void readAcelerometroData() {
  Activites activ = acelerometro.readActivites();
      if (activ.isFreeFall)
      {
        Serial.println("Free Fall Detected!");
        free_fall = true;
        time_of_fall = millis();
      }

      if ((millis() - time_of_fall >= ventana_caida_a_choque) && (free_fall)) {
        free_fall = 0;
        time_of_fall = 0;
      }

      if(activ.isActivity && free_fall) {
        segunda_condicion_caida = true;
        tiempo_de_choque_piso = millis();
      }

      if ((millis() - tiempo_de_choque_piso >= ventana_choque_a_inactividad) && (segunda_condicion_caida)) {
        segunda_condicion_caida = 0;
        tiempo_de_choque_piso = 0;
      }

      if(activ.isInactivity && segunda_condicion_caida) {
        emergencia = true;
      }

    
      sensors_event_t event; 
      acelerometro.getEvent(&event);
      x = event.acceleration.x;
      y = event.acceleration.y;
      z = event.acceleration.z;
      time_dato = time_dato + 0.025;
}
*/

void readAcelerometroData() { 


  // Determinar si hubo un impacto
  float magnitud = calcularMagnitud(x, y, z);
  if (magnitud >= 14) { // Umbral de impacto
    impacto = true;
    Serial.println("¡Impacto detectado!");
    Serial.print("Magnitud: ");
    Serial.println(magnitud);
  }
  else {
    impacto = false;
  }
  


  if(free_fall){
    Serial.println("¡Alerta! Caida.");
  }
  if(impacto) {
    Serial.println("¡Alerta! Impacto.");
  }
  if(segunda_condicion_caida){
    Serial.println("¡Alerta! Caida segundo.");
  }

  if(emergencia){
    Serial.println("¡Alerta! EMERGENCIA");
  }

  // Limitar el número de datos almacenados (opcional)
  //if (dataLog.size() > 6000) {
  //  dataLog.erase(dataLog.begin()); // Eliminar el dato más antiguo
  //}

}
// Funciones de interrupción para los botones
void IRAM_ATTR handleLeftInterrupt() {
  unsigned long currentTime = millis();
  bool buttonState = digitalRead(BUTTON_LEFT);
  
  if (currentTime - leftButton.lastPressTime >= DEBOUNCE_TIME) {
    if (buttonState == LOW && !leftButton.isPressed) { // Botón presionado
      leftButton.pressStartTime = currentTime;
      leftButton.isPressed = true;
      leftButton.lastPressTime = currentTime;
      leftButton.lastVolumeUpdateTime = currentTime; // Inicializar tiempo de actualización de volumen
    } 
  }
  if (buttonState == HIGH && leftButton.isPressed) { // Botón liberado
    leftButton.isPressed = false;
    leftButton.wasLongPress = (currentTime - leftButton.pressStartTime >= LONG_PRESS_TIME);
    
    if (!leftButton.wasLongPress) {
      if (currentTime - leftButton.lastClickTime <= TRIPLE_CLICK_TIME) {
        leftButton.clickCount++;
      } else {
        leftButton.clickCount = 1;
      }
      leftButton.lastClickTime = currentTime;
    }
    needProcessButton = true;
  }
}

void IRAM_ATTR handleRightInterrupt() {
  unsigned long currentTime = millis();
  bool buttonState = digitalRead(BUTTON_RIGHT);
  
  if (currentTime - rightButton.lastPressTime >= DEBOUNCE_TIME) {
    if (buttonState == LOW && !rightButton.isPressed) {
      rightButton.pressStartTime = currentTime;
      rightButton.isPressed = true;
      rightButton.lastPressTime = currentTime;
      rightButton.lastVolumeUpdateTime = currentTime; // Inicializar tiempo de actualización de volumen
    } 
  }
  if (buttonState == HIGH && rightButton.isPressed) {
    rightButton.isPressed = false;
    rightButton.wasLongPress = (currentTime - rightButton.pressStartTime >= LONG_PRESS_TIME);
    
    if (!rightButton.wasLongPress) {
      if (currentTime - rightButton.lastClickTime <= TRIPLE_CLICK_TIME) {
        rightButton.clickCount++;
      } else {
        rightButton.clickCount = 1;
      }
      rightButton.lastClickTime = currentTime;
    }
    
    needProcessButton = true;
  }
}

void IRAM_ATTR handleSelectInterrupt() {
  unsigned long currentTime = millis();
  bool buttonState = digitalRead(BUTTON_SELECT);
  
  if (currentTime - selectButton.lastPressTime >= DEBOUNCE_TIME) {
    if (buttonState == LOW && !selectButton.isPressed) {
      selectButton.pressStartTime = currentTime;
      selectButton.isPressed = true;
      selectButton.lastPressTime = currentTime;
    }
  }
  if (buttonState == HIGH && selectButton.isPressed) {
    selectButton.isPressed = false;
    selectButton.wasLongPress = (currentTime - selectButton.pressStartTime >= LONG_PRESS_TIME);
    
    if (!selectButton.wasLongPress) {
      if (currentTime - selectButton.lastClickTime <= TRIPLE_CLICK_TIME) {
        selectButton.clickCount++;
      } else {
        selectButton.clickCount = 1;
      }
      selectButton.lastClickTime = currentTime;
    }
    
    needProcessButton = true;
  }
}

void processButtonPress() {
  if (!needProcessButton && !leftButton.isPressed && !rightButton.isPressed) return;
  
  unsigned long currentTime = millis();
  
  // Manejar presión continua para volumen
  if (currentScreen == SCREEN_MP3_PLAYER) {
    if (leftButton.isPressed && (currentTime - leftButton.pressStartTime >= LONG_PRESS_TIME)) {
      // Verificar si es tiempo de actualizar el volumen
      if (currentTime - leftButton.lastVolumeUpdateTime >= VOLUME_UPDATE_INTERVAL) {
        if (currentVolume > 0) {
          currentVolume--;
          dfPlayer.volume(currentVolume);
          drawMP3Screen();
        }
        leftButton.lastVolumeUpdateTime = currentTime;
      }
      return; // No procesar otros eventos mientras se ajusta el volumen
    }
    
    if (rightButton.isPressed && (currentTime - rightButton.pressStartTime >= LONG_PRESS_TIME)) {
      // Verificar si es tiempo de actualizar el volumen
      if (currentTime - rightButton.lastVolumeUpdateTime >= VOLUME_UPDATE_INTERVAL) {
        if (currentVolume < 30) {
          currentVolume++;
          dfPlayer.volume(currentVolume);
          drawMP3Screen();
        }
        rightButton.lastVolumeUpdateTime = currentTime;
      }
      return; // No procesar otros eventos mientras se ajusta el volumen
    }
  }
  
  // Procesar otros eventos de botones solo si no están presionados
  if (!needProcessButton || leftButton.isPressed || rightButton.isPressed || selectButton.isPressed) return;
  
  ButtonState* activeButton = nullptr;
  uint8_t buttonType = 0;
  
  // Determinar qué botón fue el último en ser presionado
  if (leftButton.lastPressTime > rightButton.lastPressTime && 
      leftButton.lastPressTime > selectButton.lastPressTime) {
    activeButton = &leftButton;
    buttonType = BUTTON_LEFT;
  }
  else if (rightButton.lastPressTime > leftButton.lastPressTime && 
           rightButton.lastPressTime > selectButton.lastPressTime) {
    activeButton = &rightButton;
    buttonType = BUTTON_RIGHT;
  }
  else if (selectButton.lastPressTime > leftButton.lastPressTime && 
           selectButton.lastPressTime > rightButton.lastPressTime) {
    activeButton = &selectButton;
    buttonType = BUTTON_SELECT;
  }
  
  if (activeButton != nullptr) {
    // Si ha pasado el tiempo de espera o tenemos 3 clicks, procesar el evento
    if ((currentTime - activeButton->lastClickTime >= CLICK_TIMEOUT) || 
        activeButton->clickCount >= 3) {
      if (activeButton->wasLongPress) {
        handleButtonPress(buttonType, true, false, false);
      } else {
        // Determinar tipo de click basado en el contador
        bool isDoubleClick = (activeButton->clickCount == 2);
        bool isTripleClick = (activeButton->clickCount == 3);
        handleButtonPress(buttonType, false, isDoubleClick, isTripleClick);
      }
      
      // Resetear el contador de clicks
      activeButton->clickCount = 0;
      needProcessButton = false;
    }
  }
}

// --  FUNCIONES ICONOS --
void drawMinusIcon(uint16_t x, uint16_t y, uint16_t color, uint8_t size) {
  // Dibuja una línea horizontal simple pixel por pixel
  uint16_t lineLength = size; // Ancho del icono
  uint16_t thickness = (size > 6) ? 2 : 1; // Grosor de 1 o 2 píxeles
  uint16_t startX = x;
  uint16_t centerY = y + size / 2;
  uint16_t startY = centerY - thickness / 2; // Y inicial para el grosor

  // Bucle para el grosor vertical
  for (uint8_t h = 0; h < thickness; ++h) {
    // Bucle para la longitud horizontal
    for (uint8_t w = 0; w < lineLength; ++w) {
      st7735.st7735_draw_pixel(startX + w, startY + h, color);
    }
  }
}

void drawPlusIcon(uint16_t x, uint16_t y, uint16_t color, uint8_t size) {
  // Dibuja una cruz (+) pixel por pixel
  uint16_t lineLength = size; // Ancho/Alto del icono
  uint16_t thickness = (size > 6) ? 2 : 1; // Grosor de 1 o 2 píxeles
  uint16_t centerX = x + size / 2;
  uint16_t centerY = y + size / 2;
  uint16_t halfLength = lineLength / 2;

  // --- Dibuja la línea horizontal ---
  uint16_t hStartX = centerX - halfLength;
  uint16_t hStartY = centerY - thickness / 2;
  // Bucle para el grosor vertical
  for (uint8_t h = 0; h < thickness; ++h) {
    // Bucle para la longitud horizontal
    for (uint8_t w = 0; w < lineLength; ++w) {
      st7735.st7735_draw_pixel(hStartX + w, hStartY + h, color);
    }
  }

  // --- Dibuja la línea vertical ---
  uint16_t vStartX = centerX - thickness / 2;
  uint16_t vStartY = centerY - halfLength;
   // Bucle para el grosor horizontal
  for (uint8_t w = 0; w < thickness; ++w) {
    // Bucle para la longitud vertical
    for (uint8_t h = 0; h < lineLength; ++h) {
      st7735.st7735_draw_pixel(vStartX + w, vStartY + h, color);
    }
  }
}

void drawPlayIcon(uint16_t x, uint16_t y, uint16_t borderColor, uint16_t radius, uint16_t circleColor, uint16_t fillColor) {
    // Dibuja el círculo sólido
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= radius*radius) {
                st7735.st7735_draw_pixel(x + dx, y + dy, circleColor);
            }
        }
    }
    // --- Dimensiones del Icono ---
  const int totalHeight = radius; // Altura total (ej: 16)

  const int iconHalfHeight = totalHeight/2;   // Mitad de la altura total (para 16 total)
  const int iconMaxWidth = radius/1.2;  // Ancho máximo deseado en la base (ajustado a 12)
  const int borderThickness = 1;  // Grosor del borde en píxeles (AHORA 2)
  Serial.println(iconHalfHeight);

  // ---------------------------

  const int max_i_val = iconHalfHeight - 1; // Valor máximo que alcanzará 'i' (ej: 7)
  //const int totalHeight = iconHalfHeight * 2; // Altura total (ej: 16)
  int triX = x - iconMaxWidth/3;     // desplaza un poco a la izquierda para centrar visualmente
  int triY = y - totalHeight/2;
  // Asegurarse de que el grosor del borde no sea demasiado grande
  if (borderThickness * 2 > iconMaxWidth || borderThickness > iconHalfHeight) {
     Serial.println("Advertencia: Grosor de borde demasiado grande para el icono Play.");
     // Podrías forzar borderThickness = 1 aquí si lo prefieres
     // borderThickness = 1; 
  }

  // Bucle externo: Itera verticalmente por la mitad de la altura
  for (int i = 0; i < iconHalfHeight; i++) {
    // Calcula la extensión horizontal (límite de j) para esta fila 'i'
    int j_limit = (i * (iconMaxWidth - 1)) / max_i_val; 

    // Bucle interno: Itera horizontalmente a lo largo del ancho calculado para la fila
    for (int j = 0; j <= j_limit; j++) {
      uint16_t currentPixelColor;

      // Determinar si el píxel actual está en el borde grueso
      // Es borde si: está cerca del borde izquierdo, cerca del borde derecho,
      // o cerca de la base horizontal.
      bool isBorder = (j < borderThickness ||                     // Cerca del borde izquierdo
                       j > j_limit - borderThickness         // Cerca del borde derecho
                       ); // Cerca de la base (últimas 'borderThickness' filas)

      currentPixelColor = isBorder ? borderColor : fillColor;

      // Dibuja el píxel de la mitad superior
      st7735.st7735_draw_pixel(triX + j, triY + i, currentPixelColor);

      // Dibuja el píxel de la mitad inferior (reflejado verticalmente)
      // PERO: Evita dibujar la fila central (i == max_i_val) dos veces.
      // Solo dibuja el reflejo si NO estamos en la última fila del bucle 'i'.
      st7735.st7735_draw_pixel(triX + j, triY + (totalHeight - 1) - i, currentPixelColor);
    }
  }
}

void drawPauseIcon(uint16_t x, uint16_t y, uint16_t borderColor, uint16_t radius, uint16_t circleColor, uint16_t fillColor) {
    // 1. Dibuja el círculo sólido de fondo (igual que en drawPlayIcon)
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            // Usar <= radius*radius para un círculo relleno
            if (dx*dx + dy*dy <= radius*radius) {
                st7735.st7735_draw_pixel(x + dx, y + dy, circleColor);
            }
        }
    }

    // 2. Dibuja las dos barras de pausa dentro del círculo

    // --- Dimensiones de las barras (relativas al radio) ---
    const int barHeight = radius * 1.2; // Altura similar al triángulo de play
    const int barWidth = radius / 3;    // Ancho de cada barra (ajustar si es necesario)
    const int barSpacing = radius / 3;  // Espacio entre las barras (ajustar si es necesario)
    const int totalWidth = barWidth * 2 + barSpacing; // Ancho total ocupado por las barras

    // --- Posición de las barras (centradas en x, y) ---
    int topY = y - barHeight / 2;
    int bottomY = topY + barHeight -1; // -1 porque la altura incluye el pixel inicial
    // La coordenada X de la barra izquierda es: centro - mitad_ancho_total + mitad_ancho_barra
    int leftBarCenterX = x - totalWidth / 2 + barWidth / 2;
    // La coordenada X de la barra derecha es: centro + mitad_ancho_total - mitad_ancho_barra
    int rightBarCenterX = x + totalWidth / 2 - barWidth / 2;

    int leftBarStartX = leftBarCenterX - barWidth / 2;
    int rightBarStartX = rightBarCenterX - barWidth / 2;
    // -----------------------------------------------------

    // Barra izquierda
    st7735.st7735_fill_rectangle(leftBarStartX, topY, barWidth, barHeight, fillColor);
    // Barra derecha
    st7735.st7735_fill_rectangle(rightBarStartX, topY, barWidth, barHeight, fillColor);
}

void drawPrevIcon(uint16_t x, uint16_t y, uint16_t color) {
  // Triángulo hacia la izquierda
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j <= i; j++) {
      st7735.st7735_draw_pixel(x + 8 - j, y + i, color);
      st7735.st7735_draw_pixel(x + 8 - j, y + 15 - i, color);
    }
  }
  // Línea vertical
  for(int i = 0; i < 1; i++){
    for(int j = 0; j < 16; j++) {
      st7735.st7735_draw_pixel(x - i, y + j, color);
    }
  }
}

void drawNextIcon(uint16_t x, uint16_t y, uint16_t color) {
  // Triángulo hacia la derecha
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j <= i; j++) {
      st7735.st7735_draw_pixel(x + j, y + i, color);
      st7735.st7735_draw_pixel(x + j, y + 15 - i, color);
    }
  }
  // Línea vertical
  for(int i = 0; i < 16; i++) {
    st7735.st7735_draw_pixel(x + 8, y + i, color);
  }
}

void drawVolDownIcon(uint16_t x, uint16_t y, uint16_t color) {
  // Triángulo hacia abajo
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8-i; j++) {
      st7735.st7735_draw_pixel(x + j + i/2, y + i + 4, color);
    }
  }
}

void drawVolUpIcon(uint16_t x, uint16_t y, uint16_t color) {
  // Triángulo hacia arriba
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8-i; j++) {
      st7735.st7735_draw_pixel(x + j + i/2, y + 8 - i, color);
    }
  }
}

void drawRoundedRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t radius, uint16_t color) {
    // Dibujar la parte central (entre los radios)
    for(uint16_t i = radius; i < width - radius; i++) {
        for(uint16_t j = 0; j < height; j++) {
            st7735.st7735_draw_pixel(x + i, y + j, color);
        }
    }

    // Dibujar los bordes verticales (conectando las esquinas)
    for(uint16_t i = radius; i < height - radius; i++) {
        // Borde izquierdo
        for(uint16_t j = 0; j < radius; j++) {
            st7735.st7735_draw_pixel(x + j, y + i, color);
        }
        // Borde derecho
        for(uint16_t j = 0; j < radius; j++) {
            st7735.st7735_draw_pixel(x + width - j - 1, y + i, color);
        }
    }

    // Dibujar las esquinas redondeadas
    for(uint16_t i = 0; i <= radius; i++) {
        for(uint16_t j = 0; j <= radius; j++) {
            if((i * i + j * j) <= (radius * radius)) {
                // Esquina superior izquierda
                st7735.st7735_draw_pixel(x + radius - i, y + radius - j, color);
                // Esquina superior derecha
                st7735.st7735_draw_pixel(x + width - radius + i - 1, y + radius - j, color);
                // Esquina inferior izquierda
                st7735.st7735_draw_pixel(x + radius - i, y + height - radius + j - 1, color);
                // Esquina inferior derecha
                st7735.st7735_draw_pixel(x + width - radius + i - 1, y + height - radius + j - 1, color);
            }
        }
    }
}

// Función para calibrar las condiciones iniciales (posición de pie)
void calibrateStandingPosition() {
  sensors_event_t event;
  acelerometro.getEvent(&event);

  // Guardar los valores iniciales
  initialX = event.acceleration.x;
  initialY = event.acceleration.y;
  initialZ = event.acceleration.z;

  isCalibrated = true;
  Serial.println("Calibración completada. Condiciones iniciales guardadas:");
  Serial.print("X: "); Serial.println(initialX);
  Serial.print("Y: "); Serial.println(initialY);
  Serial.print("Z: "); Serial.println(initialZ);
}


// Función para verificar si la persona está de pie o en el piso
bool isPersonStanding() {
  if (!isCalibrated) {
    Serial.println("Error: No se ha realizado la calibración.");
    return false;
  }
  sensors_event_t event;
  acelerometro.getEvent(&event);

  // Comparar los valores actuales con las condiciones iniciales
  float deltaX = abs(event.acceleration.x - initialX);
  float deltaY = abs(event.acceleration.y - initialY);
  float deltaZ = abs(event.acceleration.z - initialZ);
  Serial.print("deltaX: "); Serial.println(deltaX);
  Serial.print("deltaY: "); Serial.println(deltaY);
  Serial.print("deltaZ: "); Serial.println(deltaZ);
  // Umbral para determinar si la persona sigue de pie
  float threshold = 5.0; // Ajusta este valor según sea necesario

  if (deltaX <= threshold && deltaY <= threshold && deltaZ <= threshold) {
    return true; // La persona sigue de pie
  } else {
    return false; // La persona ya no está de pie
  }
}

float calcularMagnitud(float x, float y, float z) {
  return sqrt(x * x + y * y + z * z);
}