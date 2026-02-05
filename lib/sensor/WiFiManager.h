#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include "Config.h"

class WiFiManager {
private:
    Config* config;
    uint8_t ledPin;
    ESP8266WebServer* server;
    String apSSID;
    
    void blinkLED();
    String loadHTMLFile(const char* filename);
    String replaceVariables(String html);
    
public:
    WiFiManager(Config* cfg, uint8_t led);
    ~WiFiManager();
    
    bool connect();
    void disconnect();
    
    bool syncNTP();
    uint32_t getCurrentTime() const;
    
    // Config mode / AP mode
    void startConfigMode();
    void handleClient();
    
    // Web handlers
    void handleRoot();
    void handleSave();
};

#endif
