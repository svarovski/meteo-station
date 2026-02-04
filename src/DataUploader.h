#ifndef DATA_UPLOADER_H
#define DATA_UPLOADER_H

#include <Arduino.h>
#include "Config.h"
#include "RTCData.h"
#include "InfluxDBWrapper.h"

class DataUploader {
private:
    Config* config;
    RTCData* rtcData;
    InfluxDBWrapper influxClient;
    
    bool uploadROMRecords();
    bool uploadRAMRecords();
    void addBatteryReading(float voltage);
    
public:
    DataUploader(Config* cfg, RTCData* rtc);
    
    bool uploadAllData(float batteryVoltage);
    void clearData();
};

#endif
