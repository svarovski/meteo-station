#include "SensorManager.h"
#include "SensorRecord.h"
#include <Wire.h>
#include <Adafruit_AHTX0.h>

SensorManager::SensorManager(uint8_t pin) : powerPin(pin), aht(nullptr) {
}

SensorManager::~SensorManager() {
    if (aht) {
        delete aht;
    }
}

bool SensorManager::begin() {
    pinMode(powerPin, OUTPUT);
    powerOff();
    aht = new Adafruit_AHTX0();
    return true;
}

void SensorManager::powerOn() {
    digitalWrite(powerPin, HIGH);
    delay(100); // Wait for sensor to stabilize
}

void SensorManager::powerOff() {
    digitalWrite(powerPin, LOW);
}

bool SensorManager::takeMeasurement(float& temperature, float& humidity) {
    if (!aht) {
        Serial.println("AHT10 not initialized!");
        return false;
    }
    
    powerOn();
    Wire.begin();
    
    if (!aht->begin()) {
        Serial.println("Failed to initialize AHT10!");
        powerOff();
        return false;
    }
    
    sensors_event_t humidity_event, temp_event;
    aht->getEvent(&humidity_event, &temp_event);
    
    temperature = temp_event.temperature;
    humidity = humidity_event.relative_humidity;
    
    powerOff();
    
    return validateReadings(temperature, humidity);
}

bool SensorManager::validateReadings(float temp, float hum) const {
    if (isnan(temp) || isnan(hum)) {
        Serial.println("Invalid sensor readings (NaN)");
        return false;
    }
    
    if (temp < -40 || temp > 85) {
        Serial.println("Temperature out of valid range");
        return false;
    }
    
    if (hum < 0 || hum > 100) {
        Serial.println("Humidity out of valid range");
        return false;
    }
    
    return true;
}

SensorRecord SensorManager::createRecord(float temp, float hum, uint32_t timestampSeconds, uint32_t offsetSeconds) const {
    return SensorRecord::create(temp, hum, timestampSeconds, offsetSeconds);
}
