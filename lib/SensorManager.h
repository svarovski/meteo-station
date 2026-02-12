#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Adafruit_AHTX0.h>
#include "Config.h"
#include "SensorRecord.h"

class SensorManager {
private:
    Adafruit_AHTX0 aht;
    uint8_t powerPin;
    
public:
    SensorManager(uint8_t powerPin);
    
    bool begin();
    void powerOn();
    void powerOff();
    
    bool takeMeasurement(float& temperature, float& humidity);
    bool validateReadings(float temp, float hum) const;
    
    SensorRecord createRecord(float temp, float hum, uint32_t timestampSeconds, uint32_t offsetSeconds) const;
};

#endif
