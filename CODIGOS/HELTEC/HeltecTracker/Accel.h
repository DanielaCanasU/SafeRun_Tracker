#pragma once
#include <Arduino.h>
#include <Adafruit_ADXL345_U.h>
#include "AppState.h"
#include "Config.h"

//void checkSetupAcelerometro();
//void setupAcelerometro();
float getCalibracionAcelerometro(char eje);
//void readAcelerometroData();
//void calibrateStandingPosition();
//bool isPersonStanding();
float calcularMagnitud(float x, float y, float z);
//void accelLoop();

class DetectorCaida {
    private:
        int estado;
        void calibrateStandingPosition();
        float initialX = 0, initialY = 0, initialZ = 0;
        unsigned long lastReadTime_Acelerometro = 0;
        unsigned long time_of_fall = 0, tiempo_de_choque_piso = 0, tiempoimpacto = 0;
    public:
        DetectorCaida();
        bool free_fall = false, segunda_condicion_caida = false, emergencia = false, impacto = false, isCalibrated = false;
        void init();
        void checkConfig();
        void checkStatus();
        void readAcelerometroData();
        bool isPersonStanding();
};


