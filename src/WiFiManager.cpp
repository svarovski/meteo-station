#include "WiFiManager.h"
#include <time.h>

#define NTP_SERVER "pool.ntp.org"

WiFiManager::WiFiManager(Config* cfg, uint8_t led) : config(cfg), ledPin(led) {
}

bool WiFiManager::connect() {
    unsigned long lastBlink = 0;
    bool ledState = false;
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config->ssid, config->password);
    
    Serial.printf("Connecting to %s", config->ssid);
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(250);
        Serial.print(".");
        
        // Blink LED every 500ms
        if (millis() - lastBlink > 500) {
            ledState = !ledState;
            digitalWrite(ledPin, ledState ? LOW : HIGH);
            lastBlink = millis();
        }
        
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi!");
        return false;
    }
    
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    return true;
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

bool WiFiManager::syncNTP() {
    Serial.println("Syncing time with NTP...");
    
    configTime(0, 0, NTP_SERVER);
    
    int attempts = 0;
    time_t now = time(nullptr);
    while (now < 1000000000 && attempts < 20) {
        delay(500);
        now = time(nullptr);
        attempts++;
    }
    
    if (now > 1000000000) {
        Serial.printf("Time synced: %s", ctime(&now));
        config->updateTimeOffset(now);
        config->save();
        return true;
    } else {
        Serial.println("NTP sync failed!");
        return false;
    }
}

uint32_t WiFiManager::getCurrentTime() const {
    time_t now = time(nullptr);
    return (now > 1000000000) ? now : (millis() / 1000);
}

void WiFiManager::blinkLED() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    
    if (millis() - lastBlink > 500) {
        ledState = !ledState;
        digitalWrite(ledPin, ledState ? LOW : HIGH);
        lastBlink = millis();
    }
}
