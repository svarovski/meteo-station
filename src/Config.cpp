#include "Config.h"

Config::Config() {
    setDefaults();
}

void Config::setDefaults() {
    memset(this, 0, sizeof(Config));
    interval = 1800; // 30 minutes default
    influxPort = 8086;
    strcpy(influxMeasurement, "environment");
    timeOffset = 1704067200; // 2024-01-01 00:00:00 UTC
}

bool Config::load() {
    EEPROM.get(CONFIG_ADDR, *this);
    
    if (!isValid()) {
        Serial.println("No valid configuration found");
        setDefaults();
        return false;
    }
    
    Serial.println("Configuration loaded");
    return true;
}

void Config::save() {
    magic = CONFIG_MAGIC;
    EEPROM.put(CONFIG_ADDR, *this);
    EEPROM.commit();
    Serial.println("Configuration saved");
}

bool Config::isValid() const {
    return magic == CONFIG_MAGIC;
}

void Config::print() const {
    Serial.println("=== Configuration ===");
    Serial.printf("SSID: %s\n", ssid);
    Serial.printf("Interval: %d seconds\n", interval);
    Serial.printf("InfluxDB: %s:%d\n", influxServer, influxPort);
    Serial.printf("Database: %s\n", influxDb);
    Serial.printf("Measurement: %s\n", influxMeasurement);
}
