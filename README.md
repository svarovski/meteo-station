# ESP8266 Remote Temperature/Humidity Logger

Low-power IoT sensor for remote environmental monitoring with InfluxDB integration.

## Features

- ✅ Ultra-low power consumption via deep sleep
- ✅ Configurable measurement intervals
- ✅ Local data buffering (RTC RAM + EEPROM)
- ✅ InfluxDB data upload on demand
- ✅ Web-based configuration portal
- ✅ Battery voltage monitoring
- ✅ NTP time synchronization
- ✅ Efficient 4-byte sensor records

## Hardware Requirements

### Components
- **ESP8266** (ESP-12E/F or NodeMCU/Wemos D1 Mini)
- **AHT10** Temperature/Humidity Sensor
- **18650 Li-ion Battery** (3000-3500mAh recommended)
- **AMS1117-3.3V** Voltage Regulator
- **Resistors**: 1x 1MΩ, 2x 4.7kΩ (I2C pull-ups)
- **Capacitor**: 1x 100nF (ADC filtering)
- **Button**: Momentary push button
- **Optional**: 18650 battery holder, TP4056 charging module

### Wiring Diagram

```
18650 Battery (+) ────┬─── AMS1117 VIN
                      │
                      └─── 1MΩ ───┬─── ESP8266 A0
                                   │
                                   └─── 100nF ─── GND

AMS1117 VOUT (3.3V) ──┬─── ESP8266 VCC
                      └─── ESP8266 CH_PD (EN)

GND ──────────────────┬─── ESP8266 GND
                      ├─── AMS1117 GND
                      ├─── AHT10 GND
                      └─── Button (one side)

ESP8266 GPIO16 (D0) ──┬─── AHT10 VCC
                      └─── (Connect to RST for deep sleep wake)

ESP8266 GPIO4 (D2) ───┬─── AHT10 SDA
                      └─── 4.7kΩ ─── 3.3V

ESP8266 GPIO5 (D1) ───┬─── AHT10 SCL
                      └─── 4.7kΩ ─── 3.3V

ESP8266 RST ──────────┴─── Button (other side)
                      └─── 10kΩ ─── 3.3V (pull-up)
```

### Important Notes on Hardware

1. **Deep Sleep Wake**: Connect GPIO16 (D0) to RST with a diode or resistor (220Ω-1kΩ) to allow wake from deep sleep while still enabling button reset.

2. **Voltage Divider for Battery**: The current design assumes A0 is connected via 1MΩ resistor. You'll need an additional resistor to ground to create a proper divider:
   - **Recommended**: 1MΩ from Battery+ to A0, 330kΩ from A0 to GND
   - This gives ~3.15V max at A0 when battery is 4.2V (safe for ESP8266's 1V ADC max with internal divider)

3. **AMS1117 Dropout**: AMS1117 requires ~1.2V dropout, so minimum battery voltage is ~4.5V. For 18650 (2.7-4.2V range):
   - **Better option**: Use **HT7333** or **MCP1700** (LDO with ~178mV dropout)
   - Or use a boost converter for lower battery operation

4. **Power Consumption**:
   - Deep sleep: ~20µA (ESP8266 alone)
   - AHT10 sleeping: ~0.25µA
   - **Estimated battery life** (3000mAh): 
     - 5-minute intervals: ~6-8 months
     - 15-minute intervals: ~12-18 months

## Software Setup

### Arduino IDE Configuration

1. **Install ESP8266 Board**:
   - File → Preferences → Additional Boards Manager URLs:
   - Add: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
   - Tools → Board → Boards Manager → Search "ESP8266" → Install

2. **Install Required Libraries**:
   ```
   Tools → Manage Libraries:
   - Adafruit AHTX0 (by Adafruit)
   - Adafruit BusIO (dependency)
   - Adafruit Unified Sensor (dependency)
   ```

3. **Board Settings**:
   - Board: "Generic ESP8266 Module" or your specific board
   - Flash Size: "4MB (FS:2MB OTA:~1019KB)"
   - CPU Frequency: "80 MHz" (lower power than 160 MHz)
   - Upload Speed: "115200"

### First Time Setup

1. **Upload the sketch** to ESP8266
2. **On first boot**: Device creates WiFi AP named `sensor-XXXXXX` (MAC address)
3. **Connect to this AP** from phone/computer
4. **Open browser** to `http://192.168.4.1`
5. **Configure**:
   - WiFi credentials
   - Measurement interval (minimum 60 seconds recommended)
   - InfluxDB server details
6. **Save** - device will restart and begin operation

### Configuration Portal

Access by:
- First boot (unconfigured device)
- Holding button for >5 seconds during startup

Configuration includes:
- WiFi SSID and password
- Measurement interval (seconds)
- InfluxDB server, port, database name
- InfluxDB credentials (optional)
- Measurement name (default: "environment")

## Operation Modes

### Normal Operation (Automatic)
1. Wake from deep sleep
2. Power on AHT10 sensor
3. Take measurement
4. Store in RTC RAM buffer (4 bytes/record)
5. When buffer full (64 records), write to EEPROM
6. Return to deep sleep

### Manual Sync (Button Press)
1. **Short press** (<5 seconds):
   - Wake device
   - Connect to configured WiFi
   - Sync time via NTP
   - Upload all buffered data to InfluxDB
   - Send battery voltage
   - Return to sleep

2. **Long press** (>5 seconds):
   - Enter configuration mode
   - Create WiFi AP for reconfiguration

## Data Format

### Compact 4-byte Record Structure
```c
struct SensorRecord {
  uint16_t timestamp;   // Seconds since timeOffset (0-65535 range)
  int8_t temperature;   // (actual_temp + 100), range: -100°C to +55°C
  uint8_t humidity;     // 0-100% (integer percentage)
}
```

**Storage capacity**:
- RTC RAM: 64 records (256 bytes)
- EEPROM: 64 records (256 bytes)
- **Total**: 128 measurements before upload required

**Example**: At 5-minute intervals, ~10.6 hours of autonomous operation

### InfluxDB Line Protocol

Data sent as:
```
environment temperature=22.5,humidity=45.0 1640995200000000000
environment battery_voltage=3.87 1640995200000000000
```

## Power Optimization Tips

1. **Interval Selection**:
   - Indoor stable: 15-30 minutes
   - Outdoor/variable: 5-10 minutes
   - Testing: 1-2 minutes

2. **WiFi Optimization**:
   - Upload only when needed (button press)
   - Batch data uploads reduce WiFi wake cycles
   - Use static IP to avoid DHCP delay (modify code)

3. **Component Selection**:
   - Use HT7333 instead of AMS1117 (lower dropout)
   - Add bulk capacitor (100µF) to battery for stability
   - Quality 18650 cell (Panasonic, Samsung, LG)

4. **Code Modifications for Lower Power**:
   ```cpp
   // In deepSleep() function, use:
   ESP.deepSleep(seconds * 1000000ULL, WAKE_RF_DISABLED);
   // Then enable WiFi only when needed
   ```

## Troubleshooting

### Device Won't Wake
- **Check**: GPIO16 to RST connection
- **Solution**: Ensure proper connection, test with LED on GPIO16

### Configuration Portal Not Accessible
- **Check**: AP created? (look for `sensor-XXXXXX` network)
- **Solution**: Hard reset, ensure button not stuck

### Sensor Readings Fail
- **Check**: I2C pull-ups (4.7kΩ required)
- **Check**: AHT10 power from GPIO16
- **Solution**: Test with separate 3.3V power to AHT10

### InfluxDB Upload Fails
- **Check**: Server reachable (ping from same network)
- **Check**: Database exists, credentials correct
- **Solution**: Test with curl:
  ```bash
  curl -i -XPOST 'http://YOUR_SERVER:8086/write?db=YOUR_DB' \
    --data-binary 'environment temperature=22.5,humidity=45.0'
  ```

### Battery Drains Quickly
- **Check**: Deep sleep current (should be <50µA total)
- **Check**: AMS1117 quiescent current (~5mA - too high!)
- **Solution**: Replace with HT7333 or MCP1700

### Time Sync Issues
- **Check**: NTP server accessible
- **Solution**: Change NTP_SERVER to local server or `time.google.com`

## Advanced Modifications

### 1. Add Multiple Sensors
```cpp
// In performMeasurement(), add:
sensors_event_t pressure_event;
bmp.getEvent(&pressure_event);

// In InfluxDB upload:
postData += "pressure=" + String(pressure_event.pressure) + ",";
```

### 2. LED Status Indicator
```cpp
#define LED_PIN 2
pinMode(LED_PIN, OUTPUT);
// Blink on measurement
digitalWrite(LED_PIN, HIGH);
delay(100);
digitalWrite(LED_PIN, LOW);
```

### 3. Adaptive Intervals
```cpp
// Increase interval if battery low
float voltage = readBatteryVoltage();
if (voltage < 3.3) {
  config.interval *= 2; // Double interval when battery low
}
```

### 4. Local Data Display (OLED)
```cpp
// Add Adafruit_SSD1306 library
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// In performMeasurement():
display.clearDisplay();
display.printf("Temp: %.1f C\n", temperature);
display.printf("Humidity: %.1f%%\n", humidity);
display.display();
```

## Battery Life Calculations

**Assumptions**:
- 3000mAh 18650 battery
- Deep sleep: 50µA (ESP + regulator)
- Active: 80mA for 5 seconds
- WiFi sync: 120mA for 30 seconds (once per day)

**5-minute intervals**:
- Measurements/day: 288
- Active time: 1440 seconds (0.4 hours)
- Sleep time: 23.6 hours
- Average current: (0.4 × 80mA + 23.6 × 0.05mA + 0.008 × 120mA) / 24 ≈ 1.4mA
- **Battery life: ~3000mAh / 1.4mA ≈ 89 days**

**15-minute intervals**:
- Average current: ~0.6mA
- **Battery life: ~208 days (7 months)**

## InfluxDB Dashboard Setup

**Grafana Query Example**:
```sql
SELECT mean("temperature") FROM "environment" 
WHERE $timeFilter 
GROUP BY time(5m) fill(linear)
```

**Chronograf Visualization**:
1. Create database: `CREATE DATABASE sensors`
2. Query: `SELECT * FROM environment WHERE time > now() - 24h`
3. Add graph panels for temperature, humidity, battery

## Safety & Compliance

- ⚠️ **18650 Safety**: Use protected cells, never short circuit
- ⚠️ **Charging**: Use proper TP4056 module with protection
- ⚠️ **Voltage**: Verify ADC voltage divider before connecting battery
- ⚠️ **Enclosure**: Use weatherproof case for outdoor deployment
- ⚠️ **FCC/CE**: For commercial use, ensure compliance (if applicable)

## License

This project is provided as-is for educational and personal use.

## Credits

- AHT10 library: Adafruit Industries
- ESP8266 core: ESP8266 Community
- Concept: Community-driven IoT best practices

---

**Need help?** Check the troubleshooting section or open an issue with:
- Serial monitor output
- Hardware photos
- Configuration details
