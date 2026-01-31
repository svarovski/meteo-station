/*
 * ESP8266 Remote Temperature/Humidity Logger
 * Features:
 * - Deep sleep with configurable wake intervals
 * - AHT10 sensor power control via GPIO16
 * - Data buffering in RTC RAM with periodic ROM storage
 * - Button wake for WiFi sync and data upload to InfluxDB
 * - Web-based configuration portal
 * - Battery voltage monitoring
 * - NTP time synchronization
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <time.h>
#include <Adafruit_AHTX0.h>

// Pin definitions
#define AHT_POWER_PIN 16  // D0 - Powers AHT10 sensor
#define BATTERY_PIN A0     // ADC for battery voltage
#define BUTTON_PIN 0       // D3 - Wake button (also FLASH button)

// Configuration structure stored in EEPROM
struct Config {
  char ssid[32];
  char password[64];
  uint16_t interval;        // Measurement interval in seconds
  char influxServer[64];    // InfluxDB server URL
  uint16_t influxPort;      // InfluxDB port (default 8086)
  char influxDb[32];        // Database name
  char influxUser[32];
  char influxPass[64];
  char influxMeasurement[32]; // Measurement name (default: "environment")
  uint32_t magic;           // Magic number to verify valid config
  uint32_t timeOffset;      // Unix timestamp offset (saves 2 bytes per record)
};

// Compact sensor record - 4 bytes total
struct __attribute__((packed)) SensorRecord {
  uint16_t timestamp;  // Seconds since timeOffset (max ~18 hours range)
  int8_t temperature;  // (actual_temp + 100) = range -100°C to +55°C with 1°C precision
  uint8_t humidity;    // 0-100% with 1% precision
};

// RTC RAM structure (persists during deep sleep)
struct RTCData {
  uint32_t magic;           // Magic number to verify valid RTC data
  uint32_t lastSync;        // Last NTP sync timestamp
  uint16_t recordCount;     // Number of records in buffer
  uint16_t romWriteIndex;   // Next ROM write position
  SensorRecord buffer[64];  // Buffer for ~64 readings before ROM write
};

// Constants
#define CONFIG_MAGIC 0xABCD1234
#define RTC_MAGIC 0x12345678
#define EEPROM_SIZE 512
#define CONFIG_ADDR 0
#define ROM_DATA_START 256     // Start address for sensor data in EEPROM
#define ROM_DATA_SIZE 256      // 256 bytes = 64 records max in ROM
#define MAX_ROM_RECORDS (ROM_DATA_SIZE / sizeof(SensorRecord))
#define BUTTON_LONG_PRESS 5000 // 5 seconds for config mode
#define AP_SSID_PREFIX "sensor-"
#define NTP_SERVER "pool.ntp.org"
#define VOLTAGE_DIVIDER_RATIO 1.0  // Adjust based on actual resistor values

// Global variables
Config config;
RTCData rtcData;
Adafruit_AHTX0 aht;
ESP8266WebServer server(80);
String apSSID;

// Forward declarations
void enterConfigMode();
void performMeasurement();
void syncAndUpload();
void loadConfig();
void saveConfig();
void loadRTCData();
void saveRTCData();
void writeBufferToROM();
void uploadToInfluxDB();
void syncNTP();
float readBatteryVoltage();
void deepSleep(uint32_t seconds);

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nESP8266 Sensor Starting...");
  
  // Initialize pins
  pinMode(AHT_POWER_PIN, OUTPUT);
  digitalWrite(AHT_POWER_PIN, LOW); // AHT10 off initially
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Load configuration
  loadConfig();
  
  // Load RTC data
  loadRTCData();
  
  // Check button press duration
  unsigned long buttonPressStart = millis();
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  
  if (buttonPressed) {
    Serial.println("Button detected, checking duration...");
    delay(100); // Debounce
    
    while (digitalRead(BUTTON_PIN) == LOW && (millis() - buttonPressStart) < BUTTON_LONG_PRESS) {
      delay(10);
    }
    
    unsigned long pressDuration = millis() - buttonPressStart;
    Serial.printf("Button pressed for %lu ms\n", pressDuration);
    
    if (pressDuration >= BUTTON_LONG_PRESS) {
      // Long press - enter config mode
      Serial.println("Long press detected - entering config mode");
      enterConfigMode();
      return; // Never returns, ESP resets after config
    } else {
      // Short press - sync and upload
      Serial.println("Short press - sync and upload mode");
      syncAndUpload();
      deepSleep(config.interval);
      return;
    }
  }
  
  // Check if configured
  if (config.magic != CONFIG_MAGIC) {
    Serial.println("Not configured - entering config mode");
    enterConfigMode();
    return;
  }
  
  // Normal operation - take measurement
  Serial.println("Normal measurement mode");
  performMeasurement();
  
  // Go back to deep sleep
  deepSleep(config.interval);
}

void loop() {
  // Should never reach here in normal operation
  // Only used during config mode
  server.handleClient();
}

void enterConfigMode() {
  Serial.println("=== Configuration Mode ===");
  
  // Get MAC address for AP name
  uint8_t mac[6];
  WiFi.macAddress(mac);
  apSSID = String(AP_SSID_PREFIX) + 
           String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  
  Serial.printf("Creating AP: %s\n", apSSID.c_str());
  
  // Start Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID.c_str());
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  
  Serial.println("Web server started. Please connect and configure.");
  Serial.println("Device will stay in config mode until configuration is saved.");
  
  // Stay in config mode (loop() will handle server)
  while (true) {
    server.handleClient();
    yield();
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0}";
  html += ".container{background:white;padding:20px;border-radius:8px;max-width:500px;margin:0 auto}";
  html += "h1{color:#333;text-align:center}input,select{width:100%;padding:10px;margin:8px 0;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}";
  html += "label{font-weight:bold;color:#555}";
  html += "button{background:#4CAF50;color:white;padding:12px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:16px}";
  html += "button:hover{background:#45a049}.info{background:#e7f3fe;padding:10px;border-left:4px solid #2196F3;margin:10px 0}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>Sensor Configuration</h1>";
  html += "<div class='info'>Device: " + apSSID + "</div>";
  html += "<form action='/save' method='POST'>";
  
  html += "<label>WiFi SSID:</label>";
  html += "<input type='text' name='ssid' value='" + String(config.ssid) + "' required><br>";
  
  html += "<label>WiFi Password:</label>";
  html += "<input type='password' name='password' value='" + String(config.password) + "'><br>";
  
  html += "<label>Measurement Interval (seconds):</label>";
  html += "<input type='number' name='interval' value='" + String(config.interval > 0 ? config.interval : 300) + "' min='60' required><br>";
  
  html += "<label>InfluxDB Server (IP or hostname):</label>";
  html += "<input type='text' name='server' value='" + String(config.influxServer) + "' required><br>";
  
  html += "<label>InfluxDB Port:</label>";
  html += "<input type='number' name='port' value='" + String(config.influxPort > 0 ? config.influxPort : 8086) + "' required><br>";
  
  html += "<label>Database Name:</label>";
  html += "<input type='text' name='database' value='" + String(config.influxDb) + "' required><br>";
  
  html += "<label>InfluxDB Username:</label>";
  html += "<input type='text' name='user' value='" + String(config.influxUser) + "'><br>";
  
  html += "<label>InfluxDB Password:</label>";
  html += "<input type='password' name='dbpass' value='" + String(config.influxPass) + "'><br>";
  
  html += "<label>Measurement Name:</label>";
  html += "<input type='text' name='measurement' value='" + String(strlen(config.influxMeasurement) > 0 ? config.influxMeasurement : "environment") + "'><br>";
  
  html += "<button type='submit'>Save & Restart</button>";
  html += "</form></div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleSave() {
  Serial.println("Saving configuration...");
  
  // Get form data
  strncpy(config.ssid, server.arg("ssid").c_str(), sizeof(config.ssid) - 1);
  strncpy(config.password, server.arg("password").c_str(), sizeof(config.password) - 1);
  config.interval = server.arg("interval").toInt();
  strncpy(config.influxServer, server.arg("server").c_str(), sizeof(config.influxServer) - 1);
  config.influxPort = server.arg("port").toInt();
  strncpy(config.influxDb, server.arg("database").c_str(), sizeof(config.influxDb) - 1);
  strncpy(config.influxUser, server.arg("user").c_str(), sizeof(config.influxUser) - 1);
  strncpy(config.influxPass, server.arg("dbpass").c_str(), sizeof(config.influxPass) - 1);
  strncpy(config.influxMeasurement, server.arg("measurement").c_str(), sizeof(config.influxMeasurement) - 1);
  
  // Set time offset to current time (will be updated on first NTP sync)
  config.timeOffset = 1704067200; // 2024-01-01 00:00:00 UTC as default
  config.magic = CONFIG_MAGIC;
  
  saveConfig();
  
  String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='5;url=/'>";
  html += "<style>body{font-family:Arial;text-align:center;padding:50px;background:#f0f0f0}";
  html += ".message{background:white;padding:30px;border-radius:8px;display:inline-block}</style></head>";
  html += "<body><div class='message'><h1>Configuration Saved!</h1>";
  html += "<p>Device will restart in 5 seconds...</p></div></body></html>";
  
  server.send(200, "text/html", html);
  
  delay(5000);
  ESP.restart();
}

void performMeasurement() {
  Serial.println("=== Taking Measurement ===");
  
  // Power on AHT10
  digitalWrite(AHT_POWER_PIN, HIGH);
  delay(100); // Wait for sensor to stabilize
  
  // Initialize I2C and AHT10
  Wire.begin();
  
  if (!aht.begin()) {
    Serial.println("Failed to initialize AHT10!");
    digitalWrite(AHT_POWER_PIN, LOW);
    return;
  }
  
  // Read sensor
  sensors_event_t humidity_event, temp_event;
  aht.getEvent(&humidity_event, &temp_event);
  
  float temperature = temp_event.temperature;
  float humidity = humidity_event.relative_humidity;
  
  Serial.printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
  
  // Power off AHT10
  digitalWrite(AHT_POWER_PIN, LOW);
  
  // Get current time (relative to offset)
  uint32_t currentTime = millis() / 1000; // Seconds since boot
  if (rtcData.lastSync > 0) {
    // Use wall clock time if synced
    time_t now = time(nullptr);
    currentTime = now;
  }
  
  // Create compact record
  SensorRecord record;
  record.timestamp = (currentTime - config.timeOffset) & 0xFFFF; // Keep lower 16 bits
  record.temperature = constrain(temperature + 100, 0, 255); // -100°C to +155°C
  record.humidity = constrain(humidity, 0, 100);
  
  // Add to RTC buffer
  if (rtcData.magic != RTC_MAGIC) {
    // Initialize RTC data
    rtcData.magic = RTC_MAGIC;
    rtcData.lastSync = 0;
    rtcData.recordCount = 0;
    rtcData.romWriteIndex = 0;
  }
  
  rtcData.buffer[rtcData.recordCount++] = record;
  Serial.printf("Buffered record %d/%d\n", rtcData.recordCount, (int)sizeof(rtcData.buffer)/sizeof(SensorRecord));
  
  // Check if buffer is full
  if (rtcData.recordCount >= sizeof(rtcData.buffer) / sizeof(SensorRecord)) {
    Serial.println("Buffer full, writing to ROM...");
    writeBufferToROM();
  }
  
  saveRTCData();
}

void syncAndUpload() {
  Serial.println("=== Sync and Upload Mode ===");
  
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid, config.password);
  
  Serial.printf("Connecting to %s", config.ssid);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi!");
    return;
  }
  
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Sync time with NTP
  syncNTP();
  
  // Upload data to InfluxDB
  uploadToInfluxDB();
  
  WiFi.disconnect();
}

void syncNTP() {
  Serial.println("Syncing time with NTP...");
  
  configTime(0, 0, NTP_SERVER);
  
  int attempts = 0;
  time_t now = time(nullptr);
  while (now < 1000000000 && attempts < 20) { // Wait for valid time
    delay(500);
    now = time(nullptr);
    attempts++;
  }
  
  if (now > 1000000000) {
    rtcData.lastSync = now;
    Serial.printf("Time synced: %s", ctime(&now));
    
    // Update time offset for future records
    config.timeOffset = (now / 65536) * 65536; // Round down to 16-bit boundary
    saveConfig();
  } else {
    Serial.println("NTP sync failed!");
  }
}

void uploadToInfluxDB() {
  Serial.println("Uploading to InfluxDB...");
  
  WiFiClient client;
  
  if (!client.connect(config.influxServer, config.influxPort)) {
    Serial.println("Connection to InfluxDB failed!");
    return;
  }
  
  // Build line protocol data
  String postData = "";
  int totalRecords = 0;
  
  // Read from ROM
  for (uint16_t i = 0; i < rtcData.romWriteIndex && i < MAX_ROM_RECORDS; i++) {
    SensorRecord record;
    EEPROM.get(ROM_DATA_START + i * sizeof(SensorRecord), record);
    
    uint32_t timestamp = config.timeOffset + record.timestamp;
    float temp = record.temperature - 100.0;
    float hum = record.humidity;
    
    postData += String(config.influxMeasurement) + " ";
    postData += "temperature=" + String(temp, 1) + ",";
    postData += "humidity=" + String(hum, 1) + " ";
    postData += String(timestamp) + "000000000\n"; // Nanosecond timestamp
    
    totalRecords++;
  }
  
  // Add buffered records
  for (uint16_t i = 0; i < rtcData.recordCount; i++) {
    SensorRecord& record = rtcData.buffer[i];
    
    uint32_t timestamp = config.timeOffset + record.timestamp;
    float temp = record.temperature - 100.0;
    float hum = record.humidity;
    
    postData += String(config.influxMeasurement) + " ";
    postData += "temperature=" + String(temp, 1) + ",";
    postData += "humidity=" + String(hum, 1) + " ";
    postData += String(timestamp) + "000000000\n";
    
    totalRecords++;
  }
  
  // Add current battery voltage
  float batteryVoltage = readBatteryVoltage();
  time_t now = time(nullptr);
  postData += String(config.influxMeasurement) + " ";
  postData += "battery_voltage=" + String(batteryVoltage, 2) + " ";
  postData += String(now) + "000000000\n";
  
  Serial.printf("Uploading %d records...\n", totalRecords + 1);
  
  // Send HTTP POST request
  String request = "POST /write?db=" + String(config.influxDb) + " HTTP/1.1\r\n";
  request += "Host: " + String(config.influxServer) + "\r\n";
  
  if (strlen(config.influxUser) > 0) {
    // Basic authentication
    String auth = String(config.influxUser) + ":" + String(config.influxPass);
    request += "Authorization: Basic " + base64_encode(auth) + "\r\n";
  }
  
  request += "Content-Type: application/x-www-form-urlencoded\r\n";
  request += "Content-Length: " + String(postData.length()) + "\r\n";
  request += "Connection: close\r\n\r\n";
  request += postData;
  
  client.print(request);
  
  // Wait for response
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("Timeout!");
      client.stop();
      return;
    }
  }
  
  // Read response
  String response = "";
  while (client.available()) {
    response += client.readStringUntil('\r');
  }
  
  Serial.println("Response: " + response);
  
  if (response.indexOf("204") > 0) {
    Serial.println("Upload successful!");
    
    // Clear ROM data
    rtcData.romWriteIndex = 0;
    
    // Clear buffer
    rtcData.recordCount = 0;
    
    saveRTCData();
  } else {
    Serial.println("Upload failed!");
  }
  
  client.stop();
}

void writeBufferToROM() {
  // Write buffer to ROM starting at current write index
  uint16_t recordsToWrite = min((int)rtcData.recordCount, (int)(MAX_ROM_RECORDS - rtcData.romWriteIndex));
  
  Serial.printf("Writing %d records to ROM at index %d\n", recordsToWrite, rtcData.romWriteIndex);
  
  for (uint16_t i = 0; i < recordsToWrite; i++) {
    uint16_t addr = ROM_DATA_START + (rtcData.romWriteIndex + i) * sizeof(SensorRecord);
    EEPROM.put(addr, rtcData.buffer[i]);
  }
  
  EEPROM.commit();
  
  rtcData.romWriteIndex += recordsToWrite;
  rtcData.recordCount = 0; // Clear buffer
  
  Serial.printf("ROM write complete. Next index: %d\n", rtcData.romWriteIndex);
}

float readBatteryVoltage() {
  // Read ADC (0-1023 for 0-1V on ESP8266)
  int adcValue = analogRead(BATTERY_PIN);
  
  // Calculate voltage (adjust divider ratio as needed)
  // With 1M resistor to battery and direct connection, need actual divider calculation
  // ESP8266 ADC max is 1V, 18650 is 3.0-4.2V, so we need voltage divider
  // Assuming divider gives 1V at full charge (4.2V)
  float voltage = (adcValue / 1023.0) * 4.2 * VOLTAGE_DIVIDER_RATIO;
  
  return voltage;
}

void loadConfig() {
  EEPROM.get(CONFIG_ADDR, config);
  
  if (config.magic != CONFIG_MAGIC) {
    Serial.println("No valid configuration found");
    memset(&config, 0, sizeof(config));
  } else {
    Serial.println("Configuration loaded");
    Serial.printf("SSID: %s\n", config.ssid);
    Serial.printf("Interval: %d seconds\n", config.interval);
  }
}

void saveConfig() {
  EEPROM.put(CONFIG_ADDR, config);
  EEPROM.commit();
  Serial.println("Configuration saved");
}

void loadRTCData() {
  ESP.rtcUserMemoryRead(0, (uint32_t*)&rtcData, sizeof(rtcData));
  
  if (rtcData.magic != RTC_MAGIC) {
    Serial.println("Initializing RTC data");
    memset(&rtcData, 0, sizeof(rtcData));
    rtcData.magic = RTC_MAGIC;
  } else {
    Serial.printf("RTC data loaded: %d buffered records\n", rtcData.recordCount);
  }
}

void saveRTCData() {
  ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtcData, sizeof(rtcData));
}

void deepSleep(uint32_t seconds) {
  Serial.printf("Entering deep sleep for %d seconds\n", seconds);
  Serial.flush();
  
  // Save RTC data before sleep
  saveRTCData();
  
  // ESP8266 deep sleep (microseconds)
  ESP.deepSleep(seconds * 1000000ULL, WAKE_RF_DEFAULT);
}

// Simple Base64 encoding for HTTP Basic Auth
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
