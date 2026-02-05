#include "Config.h"

Config::Config() {
    setDefaults();
}

void Config::setDefaults() {
    memset(this, 0, sizeof(Config));
    interval = 1800; // 30 minutes default
    influxPort = 8086;
    strcpy(influxMeasurement, "environment");
    timeOffset = 0; // Will be set on first sync or device start
}

void Config::updateTimeOffset(uint32_t currentTime) {
    // Round down to nearest 65536 minutes boundary
    // This gives us ~45 days range with 16-bit minute timestamps
    timeOffset = (currentTime / 65536) * 65536;
    Serial.printf("Time offset updated to: %u (%s)\n", (unsigned int)timeOffset, 
                  getTimeOffsetString().c_str());
}

String Config::getTimeOffsetString() const {
    time_t t = timeOffset;
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", gmtime(&t));
    return String(buffer);
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
