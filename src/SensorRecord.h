#ifndef SENSOR_RECORD_H
#define SENSOR_RECORD_H

#include <Arduino.h>

// Compact 4-byte sensor record
struct __attribute__((packed)) SensorRecord {
    uint16_t timestamp;   // Seconds since timeOffset (max ~18 hours)
    int8_t temperature;   // (actual_temp + 100) = -100°C to +155°C
    uint8_t humidity;     // 0-100%
    
    // Factory method to create record from float values
    static SensorRecord create(float temp, float hum, uint32_t timestamp, uint32_t timeOffset);
    
    // Decode to float values
    float getTemperature() const;
    float getHumidity() const;
    uint32_t getTimestamp(uint32_t timeOffset) const;
    
    // Validation
    bool isValid() const;
    
    // InfluxDB line protocol
    String toInfluxLine(const char* measurement, uint32_t timeOffset) const;
};

#endif
