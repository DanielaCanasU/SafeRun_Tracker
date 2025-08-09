#pragma once
#include <Arduino.h>

// Habilitar envío de datos de acelerómetro en el payload LoRa
#define ENVIAR_ACELEROMETRO

// Temporizadores (ms)
inline constexpr unsigned long sendInterval_LoRa = 1000;   // 1s
inline constexpr unsigned long sendInterval_GPS = 1000;    // 1s
inline constexpr unsigned long timerDelay_Acelerometro = 25; // 25ms

// Botones
inline constexpr unsigned long DOUBLE_CLICK_TIME = 250;  
inline constexpr unsigned long LONG_PRESS_TIME  = 800;   
inline constexpr unsigned long MENU_SWITCH_TIME = 500;   
inline constexpr unsigned long DEBOUNCE_TIME    = 300;   
inline constexpr unsigned long TRIPLE_CLICK_TIME = 600;  
inline constexpr unsigned long CLICK_TIMEOUT     = 400;  
inline constexpr unsigned long VOLUME_UPDATE_INTERVAL = 200;


