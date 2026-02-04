#include "DataUploader.h"
#include <EEPROM.h>

#define ROM_DATA_START 512
#define MAX_ROM_RECORDS 896

DataUploader::DataUploader(Config* cfg, RTCData* rtc) 
    : config(cfg), rtcData(rtc) {
}

bool DataUploader::uploadAllData(float batteryVoltage) {
    Serial.println("Uploading to InfluxDB...");
    Serial.printf("ROM records: %d, RAM records: %d\n", 
                  rtcData->romRecordCount, rtcData->recordCount);
    
    if (!influxClient.begin(config)) {
        Serial.println("Failed to initialize InfluxDB client");
        return false;
    }
    
    if (!influxClient.validateConnection()) {
        Serial.println("Failed to connect to InfluxDB");
        return false;
    }
    
    bool success = true;
    
    // Upload ROM records
    if (!uploadROMRecords()) {
        success = false;
    }
    
    // Upload RAM records
    if (!uploadRAMRecords()) {
        success = false;
    }
    
    // Upload battery voltage
    addBatteryReading(batteryVoltage);
    
    if (success) {
        clearData();
        Serial.println("All data uploaded successfully!");
    }
    
    return success;
}

bool DataUploader::uploadROMRecords() {
    for (uint16_t i = 0; i < rtcData->romRecordCount && i < MAX_ROM_RECORDS; i++) {
        SensorRecord record;
        EEPROM.get(ROM_DATA_START + i * sizeof(SensorRecord), record);
        
        if (!influxClient.writeSensorRecord(record, config->timeOffset)) {
            Serial.printf("Failed to upload ROM record %d\n", i);
            return false;
        }
    }
    return true;
}

bool DataUploader::uploadRAMRecords() {
    for (uint16_t i = 0; i < rtcData->recordCount; i++) {
        if (!influxClient.writeSensorRecord(rtcData->buffer[i], config->timeOffset)) {
            Serial.printf("Failed to upload RAM record %d\n", i);
            return false;
        }
    }
    return true;
}

void DataUploader::addBatteryReading(float voltage) {
    influxClient.writeBatteryVoltage(voltage);
}

void DataUploader::clearData() {
    rtcData->romWriteIndex = 0;
    rtcData->romRecordCount = 0;
    rtcData->clearBuffer();
    rtcData->save();
}
