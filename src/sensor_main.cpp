/*
 * ESP8266 Remote Temperature/Humidity Logger
 * Wemos D1 Mini with AHT10 sensor
 * 
 * Features:
 * - Deep sleep with configurable wake intervals  
 * - Data buffering in RTC RAM with periodic EEPROM storage
 * - Button wake for WiFi sync and data upload to InfluxDB
 * - Web-based configuration portal with LittleFS HTML files
 * - Battery voltage monitoring
 * - LED status indicators
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <time.h>
#include <Adafruit_AHTX0.h>
#include <LittleFS.h>
#include "Config.h"
#include "SensorRecord.h"
#include "RTCData.h"

// ============================================================================
// Pin Definitions (Wemos D1 Mini)
// ============================================================================
#define AHT_POWER_PIN 12  // D6 - Switchable AHT10 power
#define BATTERY_PIN A0    // A0 - Battery voltage (built-in divider)
#define LED_PIN 2         // D4 - Built-in LED (active LOW)
#define WAKE_PIN 16       // D0 - Must connect to RST for timer wake

// ============================================================================
// EEPROM Memory Map
// ============================================================================
#define EEPROM_SIZE 4096
#define ROM_DATA_START 512
#define ROM_DATA_SIZE 3584
#define MAX_ROM_RECORDS (ROM_DATA_SIZE / sizeof(SensorRecord))

// ============================================================================
// Configuration Constants
// ============================================================================
#define BUTTON_LONG_PRESS 5000
#define AP_SSID_PREFIX "sensor-"
#define NTP_SERVER "pool.ntp.org"

// ============================================================================
// Global Objects
// ============================================================================
Config config;
RTCData rtcData;
Adafruit_AHTX0 aht;
ESP8266WebServer server(80);
String apSSID;

// ============================================================================
// Utility Functions
// ============================================================================

void blinkLED() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    
    if (millis() - lastBlink > 500) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastBlink = millis();
    }
}

String base64_encode(String input) {
    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String output = "";
    int val = 0, valb = -6;
    
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output += base64_chars[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        output += base64_chars[((val << 8) >> (valb + 8)) & 0x3F];
    }
    
    while (output.length() % 4) {
        output += '=';
    }
    
    return output;
}

// ============================================================================
// Sensor Functions
// ============================================================================

void powerOnSensor() {
    digitalWrite(AHT_POWER_PIN, HIGH);
    delay(100);
}

void powerOffSensor() {
    digitalWrite(AHT_POWER_PIN, LOW);
}

bool initializeSensor() {
    powerOnSensor();
    Wire.begin();
    
    if (!aht.begin()) {
        Serial.println("Failed to initialize AHT10!");
        return false;
    }
    
    return true;
}

bool validateSensorReadings(float temp, float hum) {
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

uint32_t getCurrentTimestamp() {
    uint32_t currentTime = millis() / 1000;
    
    if (rtcData.lastSync > 0) {
        time_t now = time(nullptr);
        currentTime = now;
    }
    
    return currentTime;
}

float readBatteryVoltage() {
    int adcValue = analogRead(BATTERY_PIN);
    
    // Wemos D1 Mini has built-in voltage divider
    // Calibrate this factor based on actual measurements
    float voltage = (adcValue / 1024.0) * 4.2;
    
    Serial.printf("ADC: %d, Voltage: %.2fV\n", adcValue, voltage);
    
    return voltage;
}

// ============================================================================
// Storage Functions
// ============================================================================

void writeBufferToROM() {
    uint16_t availableSpace = MAX_ROM_RECORDS - rtcData.romWriteIndex;
    uint16_t recordsToWrite = min((int)rtcData.recordCount, (int)availableSpace);
    
    if (recordsToWrite == 0) {
        Serial.println("ROM full! Cannot write more records.");
        Serial.println("Please upload data to InfluxDB soon!");
        return;
    }
    
    Serial.printf("Writing %d records to ROM at index %d\n", 
                  recordsToWrite, rtcData.romWriteIndex);
    
    for (uint16_t i = 0; i < recordsToWrite; i++) {
        uint16_t addr = ROM_DATA_START + (rtcData.romWriteIndex + i) * sizeof(SensorRecord);
        EEPROM.put(addr, rtcData.buffer[i]);
    }
    
    EEPROM.commit();
    
    rtcData.romWriteIndex += recordsToWrite;
    rtcData.romRecordCount = rtcData.romWriteIndex;
    rtcData.clearBuffer();
    
    Serial.printf("ROM write complete. Records in ROM: %d/%d\n", 
                  rtcData.romRecordCount, MAX_ROM_RECORDS);
    
    if (rtcData.romRecordCount >= MAX_ROM_RECORDS) {
        Serial.println("WARNING: ROM storage full! Data upload needed!");
    }
}

void clearStoredData() {
    rtcData.romWriteIndex = 0;
    rtcData.romRecordCount = 0;
    rtcData.clearBuffer();
    rtcData.save();
}

// ============================================================================
// Measurement Functions
// ============================================================================

void performMeasurement() {
    Serial.println("=== Taking Measurement ===");
    
    if (!initializeSensor()) {
        powerOffSensor();
        return;
    }
    
    sensors_event_t humidity_event, temp_event;
    aht.getEvent(&humidity_event, &temp_event);
    
    float temperature = temp_event.temperature;
    float humidity = humidity_event.relative_humidity;
    
    Serial.printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
    
    powerOffSensor();
    
    if (!validateSensorReadings(temperature, humidity)) {
        return;
    }
    
    uint32_t currentTime = getCurrentTimestamp();
    SensorRecord record = SensorRecord::create(temperature, humidity, 
                                                currentTime, config.timeOffset);
    
    if (!rtcData.isValid()) {
        rtcData.initialize();
    }
    
    rtcData.addRecord(record);
    Serial.printf("Buffered record %d/%d\n", rtcData.recordCount, RTC_BUFFER_SIZE);
    
    if (rtcData.isBufferFull()) {
        Serial.println("Buffer full, writing to ROM...");
        writeBufferToROM();
    }
    
    rtcData.save();
}

// ============================================================================
// Network Functions
// ============================================================================

bool connectWiFi() {
    unsigned long lastBlink = 0;
    bool ledState = false;
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid, config.password);
    
    Serial.printf("Connecting to %s", config.ssid);
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(250);
        Serial.print(".");
        
        if (millis() - lastBlink > 500) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? LOW : HIGH);
            lastBlink = millis();
        }
        
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi!");
        return false;
    }
    
    return true;
}

void syncNTP() {
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
        rtcData.lastSync = now;
        Serial.printf("Time synced: %s", ctime(&now));
        
        config.timeOffset = (now / 65536) * 65536;
        config.save();
    } else {
        Serial.println("NTP sync failed!");
    }
}

// ============================================================================
// InfluxDB Upload Functions
// ============================================================================

bool sendInfluxBatch(WiFiClient& client, String& postData) {
    String request = "POST /write?db=" + String(config.influxDb) + " HTTP/1.1\r\n";
    request += "Host: " + String(config.influxServer) + "\r\n";
    
    if (strlen(config.influxUser) > 0) {
        String auth = String(config.influxUser) + ":" + String(config.influxPass);
        request += "Authorization: Basic " + base64_encode(auth) + "\r\n";
    }
    
    request += "Content-Type: application/x-www-form-urlencoded\r\n";
    request += "Content-Length: " + String(postData.length()) + "\r\n";
    request += "Connection: close\r\n\r\n";
    request += postData;
    
    client.print(request);
    
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println("Timeout!");
            return false;
        }
    }
    
    String response = "";
    while (client.available()) {
        response += client.readStringUntil('\r');
    }
    
    Serial.println("Response: " + response);
    
    return (response.indexOf("204") > 0);
}

bool uploadROMRecords(WiFiClient& client, String& postData, int& totalRecords) {
    for (uint16_t i = 0; i < rtcData.romRecordCount && i < MAX_ROM_RECORDS; i++) {
        SensorRecord record;
        EEPROM.get(ROM_DATA_START + i * sizeof(SensorRecord), record);
        
        postData += record.toInfluxLine(config.influxMeasurement, config.timeOffset);
        totalRecords++;
        
        if (postData.length() > 4000) {
            if (!sendInfluxBatch(client, postData)) {
                return false;
            }
            postData = "";
        }
    }
    return true;
}

bool uploadRAMRecords(WiFiClient& client, String& postData, int& totalRecords) {
    for (uint16_t i = 0; i < rtcData.recordCount; i++) {
        postData += rtcData.buffer[i].toInfluxLine(config.influxMeasurement, 
                                                    config.timeOffset);
        totalRecords++;
        
        if (postData.length() > 4000) {
            if (!sendInfluxBatch(client, postData)) {
                return false;
            }
            postData = "";
        }
    }
    return true;
}

void addBatteryReading(String& postData) {
    float batteryVoltage = readBatteryVoltage();
    time_t now = time(nullptr);
    
    postData += String(config.influxMeasurement) + " ";
    postData += "battery_voltage=" + String(batteryVoltage, 2) + " ";
    postData += String(now) + "000000000\n";
}

bool uploadAllRecords(WiFiClient& client) {
    String postData = "";
    int totalRecords = 0;
    bool allSuccess = true;
    
    if (!uploadROMRecords(client, postData, totalRecords)) {
        allSuccess = false;
    }
    
    if (!uploadRAMRecords(client, postData, totalRecords)) {
        allSuccess = false;
    }
    
    addBatteryReading(postData);
    totalRecords++;
    
    Serial.printf("Uploading %d records...\n", totalRecords);
    
    if (postData.length() > 0) {
        if (!sendInfluxBatch(client, postData)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

void uploadToInfluxDB() {
    Serial.println("Uploading to InfluxDB...");
    Serial.printf("ROM records: %d, RAM records: %d\n", 
                  rtcData.romRecordCount, rtcData.recordCount);
    
    WiFiClient client;
    
    if (!client.connect(config.influxServer, config.influxPort)) {
        Serial.println("Connection to InfluxDB failed!");
        return;
    }
    
    bool success = uploadAllRecords(client);
    
    if (success) {
        clearStoredData();
        Serial.println("All data uploaded and cleared!");
    }
    
    client.stop();
}

void syncAndUpload() {
    Serial.println("=== Sync and Upload Mode ===");
    
    if (!connectWiFi()) {
        digitalWrite(LED_PIN, HIGH);
        return;
    }
    
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    blinkLED();
    syncNTP();
    
    blinkLED();
    uploadToInfluxDB();
    
    digitalWrite(LED_PIN, HIGH);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

// ============================================================================
// Web Server Functions
// ============================================================================

String loadHTMLFile(const char* filename) {
    if (!LittleFS.exists(filename)) {
        Serial.printf("File not found: %s\n", filename);
        return "<html><body><h1>Error: File not found</h1></body></html>";
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.printf("Failed to open: %s\n", filename);
        return "<html><body><h1>Error: Cannot open file</h1></body></html>";
    }
    
    String content = file.readString();
    file.close();
    return content;
}

String replaceVariables(String html) {
    html.replace("%DEVICE_ID%", apSSID);
    html.replace("%SSID%", config.ssid);
    html.replace("%PASSWORD%", config.password);
    html.replace("%INTERVAL%", String(config.interval > 0 ? config.interval : 1800));
    html.replace("%SERVER%", config.influxServer);
    html.replace("%PORT%", String(config.influxPort > 0 ? config.influxPort : 8086));
    html.replace("%DATABASE%", config.influxDb);
    html.replace("%USER%", config.influxUser);
    html.replace("%DBPASS%", config.influxPass);
    html.replace("%MEASUREMENT%", 
                 strlen(config.influxMeasurement) > 0 ? config.influxMeasurement : "environment");
    return html;
}

void handleRoot() {
    String html = loadHTMLFile("/config.html");
    html = replaceVariables(html);
    server.send(200, "text/html", html);
}

void handleSave() {
    Serial.println("Saving configuration...");
    
    strncpy(config.ssid, server.arg("ssid").c_str(), sizeof(config.ssid) - 1);
    strncpy(config.password, server.arg("password").c_str(), sizeof(config.password) - 1);
    config.interval = server.arg("interval").toInt();
    strncpy(config.influxServer, server.arg("server").c_str(), sizeof(config.influxServer) - 1);
    config.influxPort = server.arg("port").toInt();
    strncpy(config.influxDb, server.arg("database").c_str(), sizeof(config.influxDb) - 1);
    strncpy(config.influxUser, server.arg("user").c_str(), sizeof(config.influxUser) - 1);
    strncpy(config.influxPass, server.arg("dbpass").c_str(), sizeof(config.influxPass) - 1);
    strncpy(config.influxMeasurement, server.arg("measurement").c_str(), 
            sizeof(config.influxMeasurement) - 1);
    
    config.save();
    
    String html = loadHTMLFile("/success.html");
    server.send(200, "text/html", html);
    
    delay(5000);
    ESP.restart();
}

void enterConfigMode() {
    Serial.println("=== Configuration Mode ===");
    
    uint8_t mac[6];
    WiFi.macAddress(mac);
    apSSID = String(AP_SSID_PREFIX) + 
             String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
    
    Serial.printf("Creating AP: %s\n", apSSID.c_str());
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str());
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
    
    Serial.println("Web server started");
    
    while (true) {
        server.handleClient();
        yield();
    }
}

// ============================================================================
// Sleep Function
// ============================================================================

void deepSleep(uint32_t seconds) {
    Serial.printf("Entering deep sleep for %d seconds (RF disabled)\n", seconds);
    Serial.flush();
    
    rtcData.save();
    
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);
    
    ESP.deepSleep(seconds * 1000000ULL, WAKE_RF_DISABLED);
}

// ============================================================================
// Setup and Loop
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nWemos D1 Mini Sensor Starting...");
    
    // Initialize pins
    pinMode(AHT_POWER_PIN, OUTPUT);
    digitalWrite(AHT_POWER_PIN, LOW);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    
    pinMode(WAKE_PIN, OUTPUT);
    digitalWrite(WAKE_PIN, LOW);
    
    // Initialize EEPROM and LittleFS
    EEPROM.begin(EEPROM_SIZE);
    
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed!");
        // Continue anyway, will show error in web UI
    }
    
    // Load configuration and RTC data
    config.load();
    rtcData.load();
    
    // Determine wake reason
    rst_info *resetInfo = ESP.getResetInfoPtr();
    bool timerWake = (resetInfo->reason == REASON_DEEP_SLEEP_AWAKE);
    bool buttonWake = (resetInfo->reason == REASON_EXT_SYS_RST);
    
    Serial.printf("Reset reason: %d (%s)\n", resetInfo->reason, 
                  timerWake ? "Timer" : buttonWake ? "Button/External" : "Power-on");
    
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
    
    // Timer wake - just measurement
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
    // Should never reach here except in config mode
    server.handleClient();
}
