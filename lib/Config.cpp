#include "Config.h"
#include <EEPROM.h>
#include <time.h>

Config::Config() {
    setDefaults();
}

void Config::setDefaults() {
    memset(ssid, 0, sizeof(ssid));
    memset(password, 0, sizeof(password));
    interval = 1800;
    memset(influxServer, 0, sizeof(influxServer));
    influxPort = 8086;
    memset(influxDb, 0, sizeof(influxDb));
    memset(influxUser, 0, sizeof(influxUser));
    memset(influxPass, 0, sizeof(influxPass));
    memset(influxMeasurement, 0, sizeof(influxMeasurement));
    strcpy(influxMeasurement, "environment");
    timeOffset = 0;
    magic = 0;
}

void Config::updateTimeOffset(uint32_t currentTime) {
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
    return isValid();
}

void Config::save() {
    magic = CONFIG_MAGIC;
    EEPROM.put(CONFIG_ADDR, *this);
    EEPROM.commit();
}

bool Config::isValid() const {
    return magic == CONFIG_MAGIC;
}

void Config::print() const {
    Serial.println("Configuration:");
    Serial.printf("  SSID: %s\n", ssid);
    Serial.printf("  Interval: %d seconds\n", interval);
    Serial.printf("  InfluxDB: %s:%d\n", influxServer, influxPort);
    Serial.printf("  Database: %s\n", influxDb);
    Serial.printf("  Measurement: %s\n", influxMeasurement);
    Serial.printf("  Time offset: %s\n", getTimeOffsetString().c_str());
}
