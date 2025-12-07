#pragma once
#include <Arduino.h>

struct SensorData {
  float latitude;
  float longitude;
  int state;
  int sensor1;
  int sensor2;
  int sensor3;
  int sensor4;
  int8_t rssi;
  int8_t snr;
  unsigned long receivedAt;
  float accelX, accelY, accelZ;
};

struct Waypoint {
  float latitude;
  float longitude;
  String name;
  unsigned long timestamp;
  bool isValid;
};

const int MAX_WAYPOINTS = 30;
extern Waypoint waypoints[MAX_WAYPOINTS];
extern int waypointCount;

// Configuración de distancia para ejercicio
enum ExerciseDistance { DISTANCE_500M, DISTANCE_1KM, DISTANCE_1_5KM, DISTANCE_COUNT };
extern ExerciseDistance selectedExerciseDistance;

class Data {
  private:
    int numero;
    unsigned long lastCheckEnlace = 0;
  public:
    Data();
    void init();
    void initPaquete(SensorData &data);
    void setupTime();
    SensorData currentData;
    SensorData tempData;
    bool timeSynced = false;
    
};
void initSensorData(SensorData &data);
bool parseLoRaMessage(String message, SensorData &data);
void printSensorData(const SensorData &data);
extern void loadWaypointsFromStorage();
