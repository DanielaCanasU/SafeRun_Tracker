#pragma once
#include <Arduino.h>



// Temporizadores (ms)
inline constexpr unsigned long sendInterval_LoRa_receive_ack = 10000;   // 30s
inline constexpr unsigned long sendInterval_LoRa_send_data = 1000;   // 1s
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

extern const unsigned long LORA_TIMEOUT;


// IDs de dispositivo LoRa
#define DEVICE_ID "001"  // ID del dispositivo remoto (HeltecTracker)
#define TARGET_DEVICE_ID "456"  // ID del dispositivo destino (LOCAL_SAFERUN)

// ACK timeout en milisegundos
inline constexpr unsigned long ACK_TIMEOUT = 1000;  // 1 segundo para esperar ACK


