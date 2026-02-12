#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

#define CONFIG_MAGIC 0xABCD1234
#define CONFIG_ADDR 0

class Config {
public:
    char ssid[32];
    char password[64];
    uint16_t interval;
    char influxServer[64];
    uint16_t influxPort;
    char influxDb[32];
    char influxUser[32];
    char influxPass[64];
    char influxMeasurement[32];
    uint32_t timeOffset;
    uint32_t magic;
    
    Config();
    
    bool load();
    void save();
    bool isValid() const;
    void setDefaults();
    void print() const;
    
    // Time offset management
    void updateTimeOffset(uint32_t currentTime);
    String getTimeOffsetString() const;
};

#endif
