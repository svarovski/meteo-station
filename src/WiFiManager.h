#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Config.h"

class WiFiManager {
private:
    Config* config;
    uint8_t ledPin;
    
    void blinkLED();
    
public:
    WiFiManager(Config* cfg, uint8_t led);
    
    bool connect();
    void disconnect();
    
    bool syncNTP();
    uint32_t getCurrentTime() const;
};

#endif
