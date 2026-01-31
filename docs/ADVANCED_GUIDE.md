# Advanced Optimizations & Configurations

## Power Optimization Strategies

### 1. WiFi Static IP (Saves ~2-3 seconds per connection)

Add to `syncAndUpload()` function:
```cpp
void syncAndUpload() {
  // Configure static IP (modify for your network)
  IPAddress local_IP(192, 168, 1, 100);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("Static IP config failed");
  }
  
  WiFi.begin(config.ssid, config.password);
  // ... rest of function
}
```

### 2. RF Disabled Deep Sleep (Saves ~50µA)

Modify `deepSleep()` function:
```cpp
void deepSleep(uint32_t seconds) {
  Serial.printf("Entering deep sleep for %d seconds (RF disabled)\n", seconds);
  Serial.flush();
  
  saveRTCData();
  
  // Disable RF to save power
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  
  ESP.deepSleep(seconds * 1000000ULL, WAKE_RF_DISABLED);
}
```

Then explicitly enable WiFi when needed:
```cpp
void syncAndUpload() {
  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
  // ... rest of function
}
```

### 3. Lower CPU Frequency During Measurement

```cpp
void performMeasurement() {
  // Reduce CPU to 80MHz (from 160MHz if set)
  system_update_cpu_freq(80);
  
  // ... sensor reading code ...
  
  // Can stay at 80MHz, saves power
}
```

### 4. Optimize I2C Speed

```cpp
void performMeasurement() {
  // Standard I2C is 100kHz, AHT10 supports up to 400kHz
  Wire.begin();
  Wire.setClock(100000); // Use 100kHz for reliability, lower power
  
  // For faster readings (if needed):
  // Wire.setClock(400000); // 400kHz - faster but slightly more power
}
```

### 5. Batch ROM Writes (Already Implemented)

The code already uses buffering, but you can tune buffer size:
```cpp
// In RTCData structure, adjust buffer size:
SensorRecord buffer[32];  // Smaller = more frequent ROM writes (wears flash)
SensorRecord buffer[128]; // Larger = less ROM writes but uses more RAM

// Sweet spot: 64 records (current setting)
// At 5min intervals: writes every 5.3 hours
```

## Alternative Data Encoding Schemes

### Option 1: Higher Temperature Precision (5 bytes total)

```cpp
struct SensorRecord {
  uint16_t timestamp;    // Seconds since offset
  int16_t temperature;   // (actual_temp * 10) = 0.1°C precision
  uint8_t humidity;      // 0-100%
};
```

**Trade-off**: 25% more storage, 10x better temperature precision

### Option 2: Ultra-Compact (3 bytes total)

```cpp
struct SensorRecord {
  uint16_t timestamp;    // Seconds since offset
  uint8_t data;          // Upper 4 bits: temp, Lower 4 bits: humidity
};

// Encoding:
record.data = ((temp + 10) & 0xF) << 4 | ((humidity / 7) & 0xF);
// Temp range: -10°C to +5°C (16 values)
// Humidity: 0-100% (14 steps ~7% resolution)
```

**Trade-off**: Less precision, more storage capacity

### Option 3: Delta Encoding (for stable environments)

```cpp
struct SensorRecord {
  uint16_t timestamp;
  int8_t tempDelta;      // Change from previous reading
  int8_t humidityDelta;  // Change from previous reading
};

// Store baseline in RTC:
float baselineTemp = 20.0;
float baselineHumidity = 50.0;
```

**Trade-off**: Better for stable environments, complex for large changes

## Memory Usage Optimization

### Check Available RAM
```cpp
void setup() {
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("RTC used: %d bytes\n", sizeof(rtcData));
  Serial.printf("EEPROM used: %d bytes\n", sizeof(config) + ROM_DATA_SIZE);
}
```

### Reduce String Usage
```cpp
// Instead of:
String postData = "";
postData += "field=" + String(value) + "\n";

// Use:
char postData[1024];
snprintf(postData, sizeof(postData), "field=%.1f\n", value);
```

### PROGMEM for HTML (Saves ~2KB RAM)
```cpp
const char HTML_TEMPLATE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
...
</html>
)=====";

void handleRoot() {
  String html = FPSTR(HTML_TEMPLATE);
  // Replace placeholders
  server.send(200, "text/html", html);
}
```

## Alternative Upload Protocols

### 1. MQTT Instead of InfluxDB

**Pros**: Lower overhead, better for cellular/LoRa future expansion
**Cons**: Needs MQTT broker setup

```cpp
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqtt(espClient);

void uploadToMQTT() {
  mqtt.setServer(config.mqttServer, 1883);
  mqtt.connect("esp8266-sensor");
  
  char topic[64];
  snprintf(topic, sizeof(topic), "sensors/%s/temperature", WiFi.macAddress().c_str());
  
  char payload[32];
  snprintf(payload, sizeof(payload), "%.1f", temperature);
  mqtt.publish(topic, payload);
  
  mqtt.disconnect();
}
```

### 2. HTTP POST to Custom Server

```cpp
void uploadToHTTP() {
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, "http://yourserver.com/api/sensor");
  http.addHeader("Content-Type", "application/json");
  
  String json = "{\"temp\":" + String(temp) + ",\"humidity\":" + String(hum) + "}";
  int httpCode = http.POST(json);
  
  http.end();
}
```

### 3. ThingSpeak / Adafruit IO

```cpp
void uploadToThingSpeak() {
  WiFiClient client;
  
  String url = "/update?api_key=" + String(thingSpeakAPIKey);
  url += "&field1=" + String(temperature);
  url += "&field2=" + String(humidity);
  
  client.connect("api.thingspeak.com", 80);
  client.print("GET " + url + " HTTP/1.1\r\n");
  client.print("Host: api.thingspeak.com\r\n");
  client.print("Connection: close\r\n\r\n");
}
```

## Alternative Wake Mechanisms

### 1. Timer-Only Wake (No Button)

Remove button from RST, use only GPIO16→RST connection:
```cpp
void setup() {
  // No button checking needed
  if (config.magic != CONFIG_MAGIC) {
    enterConfigMode();
  } else {
    performMeasurement();
    deepSleep(config.interval);
  }
}
```

**Reconfiguration**: Short GPIO0 to GND during power-up to enter config mode

### 2. External RTC Wake (DS3231 with Alarm)

```cpp
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  rtc.begin();
  
  // Set alarm for next wake
  DateTime next = rtc.now() + TimeSpan(config.interval);
  rtc.setAlarm1(next, DS3231_A1_Hour);
  
  // Connect RTC INT pin to ESP RST
  deepSleep(0); // Sleep indefinitely, RTC will wake
}
```

**Pros**: Precise timing, works without WiFi sync
**Cons**: Extra component, complexity

### 3. Interrupt-Based Wake (Motion Sensor)

```cpp
#define PIR_PIN 14

void setup() {
  if (digitalRead(PIR_PIN) == HIGH) {
    // Motion detected, take measurement
    performMeasurement();
  }
  
  // Set interrupt for next wake
  gpio_pin_wakeup_enable(GPIO_ID_PIN(PIR_PIN), GPIO_PIN_INTR_HILEVEL);
  deepSleep(0); // Sleep until motion
}
```

## Data Visualization Alternatives

### 1. Local Web Dashboard

Add to config mode:
```cpp
server.on("/data", HTTP_GET, []() {
  String json = "[";
  
  for (int i = 0; i < rtcData.recordCount; i++) {
    SensorRecord& r = rtcData.buffer[i];
    float temp = r.temperature - 100.0;
    
    json += "{\"ts\":" + String(r.timestamp) + ",";
    json += "\"temp\":" + String(temp) + ",";
    json += "\"hum\":" + String(r.humidity) + "}";
    
    if (i < rtcData.recordCount - 1) json += ",";
  }
  
  json += "]";
  
  server.send(200, "application/json", json);
});

// Add chart.js visualization in HTML
```

### 2. SD Card Logging

```cpp
#include <SD.h>

#define CS_PIN 15

void writeToSD() {
  SD.begin(CS_PIN);
  File file = SD.open("sensor.csv", FILE_WRITE);
  
  if (file) {
    file.printf("%d,%.1f,%.1f\n", timestamp, temp, humidity);
    file.close();
  }
}
```

### 3. OLED Real-Time Display

```cpp
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.printf("Temp: %.1f C\n", temperature);
  display.printf("Hum:  %.1f %%\n", humidity);
  display.printf("Batt: %.2fV\n", readBatteryVoltage());
  display.printf("Next: %ds\n", config.interval);
  
  display.display();
}
```

## Multi-Sensor Configuration

### Adding Second Sensor (e.g., Light Level)

```cpp
#define LIGHT_PIN A0  // Use analog pin or separate GPIO

struct SensorRecord {
  uint16_t timestamp;
  int8_t temperature;
  uint8_t humidity;
  uint8_t light;  // 0-100% light level (5 bytes total)
};

void performMeasurement() {
  // ... existing temp/humidity code ...
  
  int lightReading = analogRead(LIGHT_PIN);
  uint8_t lightPercent = map(lightReading, 0, 1023, 0, 100);
  
  record.light = lightPercent;
}
```

### Multiple I2C Sensors

```cpp
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp; // Add pressure sensor

void performMeasurement() {
  // Power on all sensors
  digitalWrite(AHT_POWER_PIN, HIGH);
  digitalWrite(BMP_POWER_PIN, HIGH);
  delay(100);
  
  // Read AHT10
  sensors_event_t hum, temp;
  aht.getEvent(&hum, &temp);
  
  // Read BMP280
  float pressure = bmp.readPressure() / 100.0; // hPa
  
  // Store all data
  record.temperature = temp.temperature + 100;
  record.humidity = hum.relative_humidity;
  record.pressure = (pressure - 950) * 2; // 950-1077 hPa in 1 byte
  
  // Power off
  digitalWrite(AHT_POWER_PIN, LOW);
  digitalWrite(BMP_POWER_PIN, LOW);
}
```

## Cellular/LoRa Adaptation

### 1. Replace WiFi with LoRa (RFM95)

```cpp
#include <LoRa.h>

void sendViaLoRa() {
  LoRa.begin(915E6); // 915 MHz for US
  
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&record, sizeof(record));
  LoRa.endPacket();
  
  LoRa.sleep(); // Low power mode
}
```

**Power**: ~14mA TX, ~10µA sleep (much better than WiFi)

### 2. GSM Module (SIM800L)

```cpp
#include <TinyGsmClient.h>

TinyGsm modem(Serial);

void uploadViaGSM() {
  modem.restart();
  modem.gprsConnect("internet"); // APN
  
  // Send HTTP POST with modem
  
  modem.gprsDisconnect();
  modem.sleepEnable(); // Low power
}
```

## Production Deployment Checklist

### Pre-Deployment
- [ ] Flash with production firmware (tested for 7+ days)
- [ ] Configure with actual WiFi and InfluxDB details
- [ ] Test full battery cycle (charge to 4.2V, measure to 3.0V cutoff)
- [ ] Verify deep sleep current <100µA with multimeter
- [ ] Test button wake and long-press config
- [ ] Calibrate ADC for accurate battery readings
- [ ] Apply conformal coating to PCB (if outdoor)
- [ ] Install in weatherproof enclosure
- [ ] Label with device ID and installation date

### Post-Deployment
- [ ] Check first data upload within 24 hours
- [ ] Monitor battery voltage trends
- [ ] Verify measurement intervals are consistent
- [ ] Set up Grafana/Chronograf alerts for missed uploads
- [ ] Document installation location and conditions
- [ ] Schedule quarterly maintenance (battery check, sensor cleaning)

### Remote Monitoring
Add to InfluxDB uploads:
```cpp
// Send diagnostic data
postData += String(config.influxMeasurement) + " ";
postData += "rssi=" + String(WiFi.RSSI()) + ",";
postData += "free_heap=" + String(ESP.getFreeHeap()) + ",";
postData += "uptime=" + String(millis() / 1000) + ",";
postData += "battery_voltage=" + String(batteryVoltage, 2) + ",";
postData += "rom_index=" + String(rtcData.romWriteIndex) + " ";
postData += String(now) + "000000000\n";
```

## Firmware Update Strategy

### OTA (Over-The-Air) Updates

```cpp
#include <ArduinoOTA.h>

void setupOTA() {
  ArduinoOTA.setHostname("sensor-esp8266");
  ArduinoOTA.setPassword("your-ota-password");
  
  ArduinoOTA.begin();
}

// In config mode loop():
void loop() {
  server.handleClient();
  ArduinoOTA.handle(); // Allow OTA updates during config
}
```

**Usage**: 
1. Enter config mode (long button press)
2. Upload via Arduino IDE: Sketch → Upload Using → Network → sensor-esp8266

### Version Tracking

```cpp
#define FIRMWARE_VERSION "1.0.2"

// Send to InfluxDB
postData += String(config.influxMeasurement) + "_info ";
postData += "firmware_version=\"" + String(FIRMWARE_VERSION) + "\" ";
```

---

**Performance Tuning Summary**:
- **Basic setup**: ~2mA average, ~60 days battery life
- **With static IP**: ~1.5mA average, ~80 days
- **With RF disabled sleep**: ~0.8mA average, ~150 days  
- **With HT7333 + optimizations**: ~0.5mA average, ~250 days (8 months!)

Choose optimizations based on your deployment needs and complexity tolerance.
