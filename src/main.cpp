/*
 * ESP8266 Remote Temperature/Humidity Logger
 * Main entry point - minimal glue code
 */

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <LittleFS.h>

#include "Config.h"
#include "SensorRecord.h"
#include "RTCData.h"
#include "SensorManager.h"
#include "WiFiManager.h"
#include "DataUploader.h"

// Pin Definitions
#define AHT_POWER_PIN 12
#define BATTERY_PIN A0
#define LED_PIN 2
#define WAKE_PIN 16

// Configuration
#define EEPROM_SIZE 4096
#define BUTTON_LONG_PRESS 5000

// Global objects
Config config;
RTCData rtcData;
SensorManager sensor(AHT_POWER_PIN);
WiFiManager wifiMgr(&config, LED_PIN);
DataUploader uploader(&config, &rtcData);

// Function prototypes
void performMeasurement();
void syncAndUpload();
void enterConfigMode();
void deepSleep(uint32_t seconds);
float readBatteryVoltage();

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nWemos D1 Mini Sensor Starting...");
    
    // Initialize hardware
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    pinMode(WAKE_PIN, OUTPUT);
    digitalWrite(WAKE_PIN, LOW);
    
    // Initialize storage
    EEPROM.begin(EEPROM_SIZE);
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed!");
    }
    
    // Initialize sensor
    sensor.begin();
    
    // Load configuration and RTC data
    config.load();
    rtcData.load();
    
    // Determine wake reason
    rst_info *resetInfo = ESP.getResetInfoPtr();
    bool timerWake = (resetInfo->reason == REASON_DEEP_SLEEP_AWAKE);
    bool buttonWake = (resetInfo->reason == REASON_EXT_SYS_RST);
    
    Serial.printf("Reset reason: %d\n", resetInfo->reason);
    
    // Handle button wake or config mode
    if (buttonWake || resetInfo->reason == REASON_DEFAULT_RST) {
        pinMode(0, INPUT_PULLUP);
        delay(100);
        
        if (digitalRead(0) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(0) == LOW && (millis() - pressStart) < BUTTON_LONG_PRESS) {
                delay(10);
            }
            
            if (millis() - pressStart >= BUTTON_LONG_PRESS) {
                Serial.println("Long press - entering config mode");
                digitalWrite(LED_PIN, LOW);
                enterConfigMode();
                return;
            }
        }
        
        if (!config.isValid()) {
            Serial.println("Not configured - entering config mode");
            digitalWrite(LED_PIN, LOW);
            enterConfigMode();
            return;
        }
        
        Serial.println("Button wake - sync and upload mode");
        syncAndUpload();
        deepSleep(config.interval);
        return;
    }
    
    // Timer wake - just take measurement
    if (timerWake) {
        Serial.println("Timer wake - measurement mode");
        digitalWrite(LED_PIN, LOW);
        performMeasurement();
        digitalWrite(LED_PIN, HIGH);
        deepSleep(config.interval);
        return;
    }
    
    // First boot
    if (!config.isValid()) {
        Serial.println("First boot - entering config mode");
        digitalWrite(LED_PIN, LOW);
        enterConfigMode();
        return;
    }
    
    Serial.println("First measurement after power-on");
    digitalWrite(LED_PIN, LOW);
    performMeasurement();
    digitalWrite(LED_PIN, HIGH);
    deepSleep(config.interval);
}

void loop() {
    // Only used in config mode
    wifiMgr.handleClient();
}

void performMeasurement() {
    Serial.println("=== Taking Measurement ===");
    
    float temperature, humidity;
    if (!sensor.takeMeasurement(temperature, humidity)) {
        Serial.println("Measurement failed!");
        return;
    }
    
    Serial.printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
    
    uint32_t currentTime = wifiMgr.getCurrentTime();
    SensorRecord record = sensor.createRecord(temperature, humidity, currentTime, config.timeOffset);
    
    if (!rtcData.isValid()) {
        rtcData.initialize();
    }
    
    rtcData.addRecord(record);
    Serial.printf("Buffered record %d/%d\n", rtcData.recordCount, RTC_BUFFER_SIZE);
    
    rtcData.save();
}

void syncAndUpload() {
    Serial.println("=== Sync and Upload Mode ===");
    
    if (!wifiMgr.connect()) {
        digitalWrite(LED_PIN, HIGH);
        return;
    }
    
    wifiMgr.syncNTP();
    
    float batteryVoltage = readBatteryVoltage();
    uploader.uploadAllData(batteryVoltage);
    
    digitalWrite(LED_PIN, HIGH);
    wifiMgr.disconnect();
}

void enterConfigMode() {
    Serial.println("=== Configuration Mode ===");
    
    wifiMgr.startConfigMode();
    
    while (true) {
        wifiMgr.handleClient();
        yield();
    }
}

float readBatteryVoltage() {
    int adcValue = analogRead(BATTERY_PIN);
    float voltage = (adcValue / 1024.0) * 4.2;
    Serial.printf("Battery: %.2fV\n", voltage);
    return voltage;
}

void deepSleep(uint32_t seconds) {
    Serial.printf("Entering deep sleep for %d seconds\n", seconds);
    Serial.flush();
    
    rtcData.save();
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);
    
    ESP.deepSleep(seconds * 1000000ULL, WAKE_RF_DISABLED);
}
