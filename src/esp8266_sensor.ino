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

// Pin definitions for Wemos D1 Mini
#define AHT_POWER_PIN 12  // D6 - Powers AHT10 sensor (separate from wake pin)
#define BATTERY_PIN A0     // A0 - ADC for battery voltage (has built-in divider on Wemos)
#define LED_PIN 2          // D4 - Built-in LED (active LOW)
#define WAKE_PIN 16        // D0 - Must connect to RST for timer wake

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
// Wemos D1 Mini has 512 bytes RTC RAM available
struct RTCData {
  uint32_t magic;           // Magic number to verify valid RTC data
  uint32_t lastSync;        // Last NTP sync timestamp
  uint16_t recordCount;     // Number of records in buffer
  uint16_t romWriteIndex;   // Next ROM write position (in records, not bytes)
  uint16_t romRecordCount;  // Total records currently in ROM
  uint16_t padding;         // Alignment padding
  SensorRecord buffer[RTC_BUFFER_SIZE];  // 128 records = 512 bytes
};

// Constants
#define CONFIG_MAGIC 0xABCD1234
#define RTC_MAGIC 0x12345678
#define EEPROM_SIZE 4096          // Wemos D1 Mini has 4KB EEPROM
#define CONFIG_ADDR 0
#define ROM_DATA_START 512        // Start address for sensor data in EEPROM
#define ROM_DATA_SIZE 3584        // 3584 bytes = 896 records (3.5KB for data)
#define MAX_ROM_RECORDS (ROM_DATA_SIZE / sizeof(SensorRecord))
#define RTC_BUFFER_SIZE 128       // 128 records in RAM before ROM write
#define BUTTON_LONG_PRESS 5000    // 5 seconds for config mode
#define AP_SSID_PREFIX "sensor-"
#define NTP_SERVER "pool.ntp.org"

// Storage capacity:
// - RTC RAM: 128 records
// - EEPROM: 896 records  
// - Total: 1024 records
// At 30-minute intervals = 21 days (3 weeks)
// At 45-minute intervals = 32 days (1 month+)

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
bool sendInfluxBatch(WiFiClient& client, String& postData);
void syncNTP();
float readBatteryVoltage();
void deepSleep(uint32_t seconds);

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nWemos D1 Mini Sensor Starting...");
  
  // Initialize pins
  pinMode(AHT_POWER_PIN, OUTPUT);
  digitalWrite(AHT_POWER_PIN, LOW); // AHT10 off initially
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED off (active LOW)
  
  pinMode(WAKE_PIN, OUTPUT);
  digitalWrite(WAKE_PIN, LOW);
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Load configuration
  loadConfig();
  
  // Load RTC data
  loadRTCData();
  
  // Check reset reason to determine wake source
  rst_info *resetInfo = ESP.getResetInfoPtr();
  bool timerWake = (resetInfo->reason == REASON_DEEP_SLEEP_AWAKE);
  bool buttonWake = (resetInfo->reason == REASON_EXT_SYS_RST);
  
  Serial.printf("Reset reason: %d (%s)\n", resetInfo->reason, 
                timerWake ? "Timer" : buttonWake ? "Button/External" : "Power-on");
  
  // If button wake, check for long press (config mode)
  if (buttonWake || resetInfo->reason == REASON_DEFAULT_RST) {
    // Check if user is holding button for config mode
    // Read GPIO0 state (boot button on Wemos acts as config trigger)
    pinMode(0, INPUT_PULLUP);
    delay(100);
    
    if (digitalRead(0) == LOW) {
      // Button held during boot - wait to see if long press
      unsigned long pressStart = millis();
      while (digitalRead(0) == LOW && (millis() - pressStart) < BUTTON_LONG_PRESS) {
        delay(10);
      }
      
      if (millis() - pressStart >= BUTTON_LONG_PRESS) {
        // Long press - enter config mode
        Serial.println("Long press detected - entering config mode");
        digitalWrite(LED_PIN, LOW); // LED ON during config
        enterConfigMode();
        return; // Never returns
      }
    }
    
    // Check if configured
    if (config.magic != CONFIG_MAGIC) {
      Serial.println("Not configured - entering config mode");
      digitalWrite(LED_PIN, LOW); // LED ON during config
      enterConfigMode();
      return;
    }
    
    // Short button press or external reset - sync and upload mode
    Serial.println("Button wake - sync and upload mode");
    syncAndUpload(); // Handles LED blinking internally
    deepSleep(config.interval);
    return;
  }
  
  // Timer wake - just take measurement (no WiFi)
  if (timerWake) {
    Serial.println("Timer wake - measurement mode");
    digitalWrite(LED_PIN, LOW); // LED ON
    performMeasurement();
    digitalWrite(LED_PIN, HIGH); // LED OFF
    deepSleep(config.interval);
    return;
  }
  
  // First boot or unknown wake reason
  if (config.magic != CONFIG_MAGIC) {
    Serial.println("First boot - entering config mode");
    digitalWrite(LED_PIN, LOW); // LED ON during config
    enterConfigMode();
    return;
  }
  
  // Configured, take first measurement
  Serial.println("First measurement after power-on");
  digitalWrite(LED_PIN, LOW); // LED ON
  performMeasurement();
  digitalWrite(LED_PIN, HIGH); // LED OFF
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
  Serial.printf("Buffered record %d/%d\n", rtcData.recordCount, RTC_BUFFER_SIZE);
  
  // Check if buffer is full
  if (rtcData.recordCount >= RTC_BUFFER_SIZE) {
    Serial.println("Buffer full, writing to ROM...");
    writeBufferToROM();
  }
  
  saveRTCData();
}

void syncAndUpload() {
  Serial.println("=== Sync and Upload Mode ===");
  
  // Blink LED every 0.5s during WiFi operations
  unsigned long lastBlink = 0;
  bool ledState = false;
  
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid, config.password);
  
  Serial.printf("Connecting to %s", config.ssid);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    delay(250);
    Serial.print(".");
    
    // Blink LED
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
    digitalWrite(LED_PIN, HIGH); // LED off
    return;
  }
  
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Continue blinking during operations
  auto blinkLED = [&]() {
    if (millis() - lastBlink > 500) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? LOW : HIGH);
      lastBlink = millis();
    }
  };
  
  // Sync time with NTP
  syncNTP();
  blinkLED();
  
  // Upload data to InfluxDB
  uploadToInfluxDB();
  
  // Turn off LED
  digitalWrite(LED_PIN, HIGH);
  
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
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
  Serial.printf("ROM records: %d, RAM records: %d\n", rtcData.romRecordCount, rtcData.recordCount);
  
  WiFiClient client;
  
  if (!client.connect(config.influxServer, config.influxPort)) {
    Serial.println("Connection to InfluxDB failed!");
    return;
  }
  
  // Build line protocol data
  String postData = "";
  int totalRecords = 0;
  
  // Read from ROM (up to romRecordCount)
  for (uint16_t i = 0; i < rtcData.romRecordCount && i < MAX_ROM_RECORDS; i++) {
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
    
    // Send in batches if too large (>4KB)
    if (postData.length() > 4000) {
      if (!sendInfluxBatch(client, postData)) {
        return;
      }
      postData = "";
    }
  }
  
  // Add buffered records from RAM
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
    
    if (postData.length() > 4000) {
      if (!sendInfluxBatch(client, postData)) {
        return;
      }
      postData = "";
    }
  }
  
  // Add current battery voltage
  float batteryVoltage = readBatteryVoltage();
  time_t now = time(nullptr);
  postData += String(config.influxMeasurement) + " ";
  postData += "battery_voltage=" + String(batteryVoltage, 2) + " ";
  postData += String(now) + "000000000\n";
  
  Serial.printf("Uploading %d records...\n", totalRecords + 1);
  
  // Send final batch
  if (postData.length() > 0) {
    if (sendInfluxBatch(client, postData)) {
      // Clear ROM and RAM data on successful upload
      rtcData.romWriteIndex = 0;
      rtcData.romRecordCount = 0;
      rtcData.recordCount = 0;
      saveRTCData();
      Serial.println("All data uploaded and cleared!");
    }
  }
  
  client.stop();
}

bool sendInfluxBatch(WiFiClient& client, String& postData) {
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
      return false;
    }
  }
  
  // Read response
  String response = "";
  while (client.available()) {
    response += client.readStringUntil('\r');
  }
  
  Serial.println("Response: " + response);
  
  if (response.indexOf("204") > 0) {
    Serial.println("Batch upload successful!");
    return true;
  } else {
    Serial.println("Batch upload failed!");
    return false;
  }
}

void writeBufferToROM() {
  // Calculate how many records we can write
  uint16_t availableSpace = MAX_ROM_RECORDS - rtcData.romWriteIndex;
  uint16_t recordsToWrite = min((int)rtcData.recordCount, (int)availableSpace);
  
  if (recordsToWrite == 0) {
    Serial.println("ROM full! Cannot write more records.");
    Serial.println("Please upload data to InfluxDB soon!");
    return;
  }
  
  Serial.printf("Writing %d records to ROM at index %d\n", recordsToWrite, rtcData.romWriteIndex);
  
  for (uint16_t i = 0; i < recordsToWrite; i++) {
    uint16_t addr = ROM_DATA_START + (rtcData.romWriteIndex + i) * sizeof(SensorRecord);
    EEPROM.put(addr, rtcData.buffer[i]);
  }
  
  EEPROM.commit();
  
  rtcData.romWriteIndex += recordsToWrite;
  rtcData.romRecordCount = rtcData.romWriteIndex; // Track total records in ROM
  rtcData.recordCount = 0; // Clear buffer
  
  Serial.printf("ROM write complete. Records in ROM: %d/%d\n", 
                rtcData.romRecordCount, MAX_ROM_RECORDS);
  
  if (rtcData.romRecordCount >= MAX_ROM_RECORDS) {
    Serial.println("WARNING: ROM storage full! Data upload needed!");
  }
}

float readBatteryVoltage() {
  // Wemos D1 Mini has built-in voltage divider on A0
  // The divider is 220k/(100k+220k) = ~3.2V max input for 1V ADC
  // For direct battery connection (no external divider needed):
  // ADC reads 0-1023 for 0-3.2V approximately
  
  int adcValue = analogRead(BATTERY_PIN);
  
  // Wemos D1 Mini voltage divider allows measuring up to ~4.2V
  // Formula: V = ADC * (3.2 / 1024)
  // But actual calibration may vary, typical is:
  float voltage = (adcValue / 1024.0) * 4.2;
  
  // For more accurate readings, calibrate with known voltages:
  // Measure actual battery voltage with multimeter
  // Adjust multiplier: voltage = adcValue * CALIBRATION_FACTOR
  // Example: if 4.2V battery shows ADC=1023, and 3.0V shows ADC=732
  // then factor = 4.2/1023 = 0.0041
  
  Serial.printf("ADC: %d, Voltage: %.2fV\n", adcValue, voltage);
  
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
  Serial.printf("Entering deep sleep for %d seconds (RF disabled)\n", seconds);
  Serial.flush();
  
  // Save RTC data before sleep
  saveRTCData();
  
  // Disable WiFi to save power
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  
  // Connect D0 (GPIO16) to RST to enable wake
  // Deep sleep with RF disabled saves ~50µA
  ESP.deepSleep(seconds * 1000000ULL, WAKE_RF_DISABLED);
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
