#ifndef SENSOR_RECORD_H
#define SENSOR_RECORD_H

#ifdef NATIVE
#include "../test/native_mocks/Arduino.h"
#else
#include <Arduino.h>
#endif

class SensorRecord {
public:
    uint16_t timestamp;    // Minutes since timeOffset (16-bit = ~45 days)
    int8_t temperature;    // Temp + 100 (range: -100 to +155°C)
    uint8_t humidity;      // 0-100%
    
    static SensorRecord create(float temp, float hum, uint32_t timestampSeconds, uint32_t timeOffsetSeconds);
    
    float getTemperature() const;
    float getHumidity() const;
    uint32_t getTimestampSeconds(uint32_t timeOffsetSeconds) const;
    
    bool isValid() const;
    String toInfluxLine(const char* measurement, uint32_t timeOffsetSeconds) const;
};

#endif
