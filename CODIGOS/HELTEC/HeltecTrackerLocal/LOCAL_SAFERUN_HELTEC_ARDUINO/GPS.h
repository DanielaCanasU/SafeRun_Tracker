#pragma once
#include <Arduino.h>
#include <HT_TinyGPS++.h>

// Variables globales del GPS
extern TinyGPSPlus gps;
extern String latitude;
extern String longitude;
extern String time_str;
extern bool gpsDataValid;
extern unsigned long lastGPSUpdate;

// Funciones del GPS
void configureGPS();
void getGpsData();
bool isGPSValid();
float getLatitude();
float getLongitude();
String getTimeString();

class Geolocation {
    private:
        int ubi;
    public:
        Geolocation();
        void init();
        void  getGpsData();
};