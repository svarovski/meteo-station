#ifndef SENSOR_RECORD_H
#define SENSOR_RECORD_H

#include <Arduino.h>

// Compact 4-byte sensor record
struct __attribute__((packed)) SensorRecord {
    uint16_t timestamp;   // MINUTES since timeOffset (max ~45 days range)
    int8_t temperature;   // (actual_temp + 100) = -100°C to +155°C
    uint8_t humidity;     // 0-100%
    
    // Factory method to create record from float values
    // timestampSeconds is in seconds, will be converted to minutes
    static SensorRecord create(float temp, float hum, uint32_t timestampSeconds, uint32_t timeOffsetSeconds);
    
    // Decode to float values
    float getTemperature() const;
    float getHumidity() const;
    
    // Get timestamp in seconds
    uint32_t getTimestampSeconds(uint32_t timeOffsetSeconds) const;
    
    // Validation
    bool isValid() const;
    
    // InfluxDB line protocol
    String toInfluxLine(const char* measurement, uint32_t timeOffsetSeconds) const;
};

#endif
