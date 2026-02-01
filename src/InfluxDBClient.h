#ifndef INFLUXDB_CLIENT_H
#define INFLUXDB_CLIENT_H

#include <Arduino.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "Config.h"
#include "SensorRecord.h"

class InfluxDBClient {
private:
    InfluxDBClient* client;
    Point* sensorPoint;
    Config* config;
    bool initialized;
    
public:
    InfluxDBClient();
    ~InfluxDBClient();
    
    // Initialize with configuration
    bool begin(Config* cfg);
    
    // Validate connection
    bool validateConnection();
    
    // Write single sensor record
    bool writeSensorRecord(const SensorRecord& record, uint32_t timeOffset);
    
    // Write battery voltage
    bool writeBatteryVoltage(float voltage);
    
    // Flush buffered writes
    bool flush();
    
    // Get last error message
    String getLastError() const;
};

#endif
