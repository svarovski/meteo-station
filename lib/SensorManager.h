#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

// Forward declarations
class Adafruit_AHTX0;
class SensorRecord;

class SensorManager {
private:
    Adafruit_AHTX0* aht;
    uint8_t powerPin;
    
public:
    SensorManager(uint8_t powerPin);
    ~SensorManager();
    
    bool begin();
    void powerOn();
    void powerOff();
    
    bool takeMeasurement(float& temperature, float& humidity);
    bool validateReadings(float temp, float hum) const;
    
    SensorRecord createRecord(float temp, float hum, uint32_t timestampSeconds, uint32_t offsetSeconds) const;
};

#endif
