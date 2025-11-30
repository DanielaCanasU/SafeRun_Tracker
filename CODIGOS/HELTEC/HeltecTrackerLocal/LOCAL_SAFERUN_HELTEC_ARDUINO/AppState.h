#pragma once
#include <HT_st7735.h>
#include "LoraHandler.h"
#include "SensorData.h"
#include  "Buttons.h"
#include "UI.h"
#include "GPS.h"
#include "FirebaseHandler.h"

extern bool TRANSMISION_COMPLETADA;
extern bool TRANSMISION_FALLIDA;

extern bool ACKenviado;
extern unsigned long lastSendTime_ACK;

//#include "Buttons.h"
// Declare the global display object
extern HT_st7735 st7735;
class Botones;
class Data;
class LoRa;
class Display;
class Geolocation;
class BaseDatos;

extern Botones botones;
extern Data datos;
extern LoRa lora;
extern Display display;
extern Geolocation geolocation;
extern BaseDatos dataBase;
