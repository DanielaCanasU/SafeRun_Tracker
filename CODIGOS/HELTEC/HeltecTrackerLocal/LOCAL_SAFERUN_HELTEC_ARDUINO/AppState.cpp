#include "AppState.h"
#include "SensorData.h"
#include "LoraHandler.h"
#include "Buttons.h"



Data datos;
LoRa lora;
Botones botones;
Display display;
Geolocation geolocation;
BaseDatos dataBase;

bool TRANSMISION_COMPLETADA = true;
bool TRANSMISION_FALLIDA = false;
bool ACKenviado = true;

unsigned long lastSendTime_ACK = 0;