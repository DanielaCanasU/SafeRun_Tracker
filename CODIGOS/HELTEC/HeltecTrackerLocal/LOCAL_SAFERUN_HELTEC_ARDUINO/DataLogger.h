#pragma once
#include <Arduino.h>
#include "SensorData.h"

#define MAX_RECORDS 800
struct DataRecord {
    unsigned long timestamp;
    float lat_local, lon_local, lat_remote, lon_remote;
    uint8_t state;
    uint8_t sensor1, sensor2, sensor3, sensor4;
    int8_t rssi, snr;
    uint8_t type;
    float accelX, accelY, accelZ;
};

extern DataRecord dataBuffer[MAX_RECORDS];
extern size_t dataCount;
extern size_t dataIndex;

void clearAllRecords();
bool saveDataRecord(const SensorData& loraData, float localLat, float localLon, uint8_t type = 0);
bool saveManualRecord(const SensorData& loraData, float localLat, float localLon);
uint16_t getRecordCount();
String generateCSV();
uint16_t getMaxRecords();

