#include "SensorRecord.h"

SensorRecord SensorRecord::create(float temp, float hum, uint32_t timestamp, uint32_t timeOffset) {
    SensorRecord record;
    record.timestamp = (timestamp - timeOffset) & 0xFFFF;
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

uint32_t SensorRecord::getTimestamp(uint32_t timeOffset) const {
    return timeOffset + timestamp;
}

bool SensorRecord::isValid() const {
    // Check if values are within reasonable ranges
    float temp = getTemperature();
    float hum = getHumidity();
    
    return (temp >= -100 && temp <= 155 && hum >= 0 && hum <= 100);
}

String SensorRecord::toInfluxLine(const char* measurement, uint32_t timeOffset) const {
    String line = String(measurement) + " ";
    line += "temperature=" + String(getTemperature(), 1) + ",";
    line += "humidity=" + String(getHumidity(), 1) + " ";
    line += String(getTimestamp(timeOffset)) + "000000000\n";
    return line;
}
