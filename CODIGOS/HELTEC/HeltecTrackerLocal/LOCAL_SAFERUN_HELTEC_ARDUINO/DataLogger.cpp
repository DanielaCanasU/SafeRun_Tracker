#include "DataLogger.h"

DataRecord dataBuffer[MAX_RECORDS];
size_t dataCount = 0;
size_t dataIndex = 0;

void clearAllRecords() {
    dataCount = 0;
    dataIndex = 0;
}

bool saveDataRecord(const SensorData& loraData, float localLat, float localLon, uint8_t type) {
    DataRecord &rec = dataBuffer[dataIndex];
    rec.timestamp = millis();
    rec.lat_local = localLat;
    rec.lon_local = localLon;
    rec.lat_remote = loraData.latitude;
    rec.lon_remote = loraData.longitude;
    rec.state = (uint8_t)loraData.state;
    rec.sensor1 = (uint8_t)loraData.sensor1;
    rec.sensor2 = (uint8_t)loraData.sensor2;
    rec.sensor3 = (uint8_t)loraData.sensor3;
    rec.sensor4 = (uint8_t)loraData.sensor4;
    rec.rssi = loraData.rssi;
    rec.snr = loraData.snr;
    rec.type = type;
    rec.accelX = loraData.accelX;
    rec.accelY = loraData.accelY;
    rec.accelZ = loraData.accelZ;
    dataIndex = (dataIndex + 1) % MAX_RECORDS;
    if (dataCount < MAX_RECORDS) dataCount++;
    return true;
}

bool saveManualRecord(const SensorData& loraData, float localLat, float localLon) {
    return saveDataRecord(loraData, localLat, localLon, 1);
}

uint16_t getRecordCount() { return dataCount; }

String generateCSV() {
    String csv = "timestamp,lat_local,lon_local,lat_remote,lon_remote,state,sensor1,sensor2,sensor3,sensor4,rssi,snr,type,accelX,accelY,accelZ\n";
    size_t startIdx = (dataCount >= MAX_RECORDS) ? dataIndex : 0;
    size_t count = (dataCount >= MAX_RECORDS) ? MAX_RECORDS : dataCount;
    for (size_t i = 0; i < count; i++) {
        size_t idx = (startIdx + i) % MAX_RECORDS;
        const DataRecord &rec = dataBuffer[idx];
        csv += String(rec.timestamp) + ",";
        csv += String(rec.lat_local, 6) + ",";
        csv += String(rec.lon_local, 6) + ",";
        csv += String(rec.lat_remote, 6) + ",";
        csv += String(rec.lon_remote, 6) + ",";
        csv += String(rec.state) + ",";
        csv += String(rec.sensor1) + ",";
        csv += String(rec.sensor2) + ",";
        csv += String(rec.sensor3) + ",";
        csv += String(rec.sensor4) + ",";
        csv += String(rec.rssi) + ",";
        csv += String(rec.snr) + ",";
        csv += String(rec.type) + ",";
        csv += String(rec.accelX, 2) + ",";
        csv += String(rec.accelY, 2) + ",";
        csv += String(rec.accelZ, 2) + "\n";
    }
    return csv;
}

uint16_t getMaxRecords() { return MAX_RECORDS; }

