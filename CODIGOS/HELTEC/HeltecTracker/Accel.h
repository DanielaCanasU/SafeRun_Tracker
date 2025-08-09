#pragma once
#include <Arduino.h>
#include <Adafruit_ADXL345_U.h>
#include "AppState.h"
#include "Config.h"

void checkSetupAcelerometro();
void setupAcelerometro();
float getCalibracionAcelerometro(char eje);
void readAcelerometroData();
void calibrateStandingPosition();
bool isPersonStanding();
float calcularMagnitud(float x, float y, float z);
void accelLoop();


