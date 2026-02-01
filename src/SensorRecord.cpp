#include "SensorRecord.h"

SensorRecord SensorRecord::create(float temp, float hum, uint32_t timestampSeconds, uint32_t timeOffsetSeconds) {
    SensorRecord record;
    
    // Convert seconds to minutes for storage
    uint32_t timestampMinutes = timestampSeconds / 60;
    uint32_t offsetMinutes = timeOffsetSeconds / 60;
    
    record.timestamp = (timestampMinutes - offsetMinutes) & 0xFFFF;
    record.temperature = constrain(temp + 100, 0, 255);
    record.humidity = constrain(hum, 0, 100);
    return record;
}

float SensorRecord::getTemperature() const {
    return temperature - 100.0;
}

float SensorRecord::getHumidity() const {
    return humidity;
}

uint32_t SensorRecord::getTimestampSeconds(uint32_t timeOffsetSeconds) const {
    // Convert stored minutes back to seconds
    uint32_t offsetMinutes = timeOffsetSeconds / 60;
    uint32_t absoluteMinutes = offsetMinutes + timestamp;
    return absoluteMinutes * 60;
}

bool SensorRecord::isValid() const {
    // Check if values are within reasonable ranges
    float temp = getTemperature();
    float hum = getHumidity();
    
    return (temp >= -100 && temp <= 155 && hum >= 0 && hum <= 100);
}

String SensorRecord::toInfluxLine(const char* measurement, uint32_t timeOffsetSeconds) const {
    String line = String(measurement) + " ";
    line += "temperature=" + String(getTemperature(), 1) + ",";
    line += "humidity=" + String(getHumidity(), 1) + " ";
    line += String(getTimestampSeconds(timeOffsetSeconds)) + "000000000\n";
    return line;
}
