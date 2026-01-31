# Corrections Applied & Additional Recommendations

## Changes Made Based on Your Corrections

### 1. Hardware Updates
✅ **Changed voltage regulator from AMS1117 to MCP1700**
- MCP1700 has only 178mV dropout (vs 1.2V for AMS1117)
- Ultra-low 1.6µA quiescent current
- Perfect for 18650 battery range (2.7V-4.2V)

✅ **Updated for Wemos D1 Mini board**
- Built-in voltage divider on A0 pin (no external divider needed)
- Built-in I2C pull-ups (external pull-ups optional)
- USB-to-serial built in (CH340 or CP2102)

✅ **Moved AHT10 power to GPIO12 (D6)**
- Separated from wake pin (GPIO16/D0)
- GPIO16/D0 now only used for deep sleep wake (connected to RST)
- Cleaner design, no pin conflicts

✅ **Removed charging module references**
- Documentation now assumes battery swapping
- Focus on battery monitoring for replacement timing

### 2. Software Updates

✅ **Extended storage capacity for 2-week to 1-month operation**
- Increased EEPROM allocation: 3584 bytes (896 records)
- Increased RTC buffer: 512 bytes (128 records)
- **Total capacity: 1024 measurements**
  - 30-minute intervals = 21 days (3 weeks)
  - 45-minute intervals = 32 days (1 month+)
  - 60-minute intervals = 42 days (6 weeks)

✅ **WiFi only on button press**
- Timer wake: measurement only, no WiFi
- Button wake: WiFi connection + data upload
- Deep sleep uses WAKE_RF_DISABLED for power savings

✅ **LED status indicators**
- **Measurement**: LED ON while taking reading
- **WiFi upload**: LED blinks every 0.5 seconds
- **Config mode**: LED solid ON
- **Deep sleep**: LED OFF
- LED is active-LOW on Wemos D1 Mini (GPIO2/D4)

✅ **Improved wake detection**
- Uses ESP.getResetInfoPtr() to distinguish timer vs button wake
- Timer wake (REASON_DEEP_SLEEP_AWAKE): measurement only
- Button wake (REASON_EXT_SYS_RST): WiFi upload
- Long button press (5+ seconds): config mode

✅ **Removed static IP configuration**
- Dynamic IP (DHCP) only
- Simpler, more compatible with different networks

✅ **Better data handling**
- Batch uploads (4KB chunks) for large datasets
- Tracks ROM record count separately from write index
- Warns when ROM storage is nearly full

## Additional Recommendations

### 1. Battery Voltage Calibration (Important!)

The Wemos D1 Mini A0 voltage divider needs calibration for accurate readings:

```cpp
float readBatteryVoltage() {
  int adcValue = analogRead(BATTERY_PIN);
  
  // CALIBRATION PROCEDURE:
  // 1. Charge battery to 4.20V (verify with multimeter)
  // 2. Note ADC reading (should be near 1023)
  // 3. Discharge to 3.00V
  // 4. Note ADC reading  
  // 5. Calculate: factor = (V_max - V_min) / (ADC_max - ADC_min)
  
  // Example calibration:
  // 4.20V → ADC 1023
  // 3.00V → ADC 732
  // Factor = (4.20-3.00)/(1023-732) = 0.00412
  
  float voltage = adcValue * 0.00412; // Adjust based on YOUR measurements
  
  return voltage;
}
```

### 2. Low Battery Protection

Add automatic interval adjustment when battery is low:

```cpp
void performMeasurement() {
  // ... existing sensor reading code ...
  
  float voltage = readBatteryVoltage();
  
  // Protect battery from deep discharge
  if (voltage < 3.0) {
    Serial.println("CRITICAL: Battery very low!");
    config.interval = 3600; // 1 hour minimum
  } else if (voltage < 3.3) {
    Serial.println("WARNING: Battery low");
    config.interval = min(config.interval * 2, 3600); // Double interval, max 1hr
  }
  
  // ... rest of measurement code ...
}
```

### 3. Sensor Validation

Prevent storing invalid readings:

```cpp
void performMeasurement() {
  // ... power on sensor and read ...
  
  // Validate readings
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Sensor read failed!");
    digitalWrite(AHT_POWER_PIN, LOW);
    return; // Skip this measurement
  }
  
  // Sanity check ranges
  if (temperature < -40 || temperature > 85) {
    Serial.println("Temperature out of valid range, skipping");
    digitalWrite(AHT_POWER_PIN, LOW);
    return;
  }
  
  if (humidity < 0 || humidity > 100) {
    Serial.println("Humidity out of valid range, skipping");
    digitalWrite(AHT_POWER_PIN, LOW);
    return;
  }
  
  // ... store valid record ...
}
```

### 4. Upload Retry Logic

Make uploads more reliable:

```cpp
void syncAndUpload() {
  // ... WiFi connection code ...
  
  const int maxAttempts = 3;
  bool uploadSuccess = false;
  
  for (int attempt = 1; attempt <= maxAttempts && !uploadSuccess; attempt++) {
    Serial.printf("Upload attempt %d/%d\n", attempt, maxAttempts);
    
    if (uploadToInfluxDB()) {
      uploadSuccess = true;
      Serial.println("Upload successful!");
    } else {
      Serial.printf("Upload failed on attempt %d\n", attempt);
      if (attempt < maxAttempts) {
        delay(5000); // Wait 5s before retry
      }
    }
  }
  
  if (!uploadSuccess) {
    Serial.println("All upload attempts failed. Data retained for next try.");
  }
  
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}
```

### 5. Recommended Measurement Intervals

| Use Case | Interval | Battery Life* | Storage Duration |
|----------|----------|---------------|------------------|
| Indoor stable | 60 min | 24+ months | 42 days |
| General purpose | 30 min | 18 months | 21 days |
| Outdoor variable | 15 min | 12 months | 10 days |
| Detailed tracking | 5 min | 6 months | 3.5 days |

*Based on 3000mAh 18650, ~50µA sleep current

**Recommendation**: Start with 30-minute intervals for good balance of data density and battery life.

### 6. Reduce Wemos Deep Sleep Current (Optional Hardware Mod)

The Wemos D1 Mini typically draws 50-80µA in deep sleep due to:
- CH340 USB chip (~30µA)
- Power LED (if always on)
- Onboard components

**To reduce to ~20µA:**
1. **Remove Power LED**: Desolder small red LED near USB (saves ~5-10µA)
2. **Disconnect CH340**: Cut trace to USB chip (advanced, saves ~30µA)
3. **Use ESP-12F directly**: Bare module for true low power (complex)

For most users, 50-80µA is acceptable and still gives 12-24 month battery life.

### 7. Field Deployment Best Practices

**Pre-Deployment:**
- ✅ Calibrate battery voltage ADC
- ✅ Test 5+ measurement cycles
- ✅ Verify sleep current <100µA with multimeter
- ✅ Test button wake and upload
- ✅ Configure with final settings
- ✅ Fully charge battery

**Installation:**
- 📍 Mount in stable location (avoid direct sun)
- 🔘 Ensure RST button accessible
- 📝 Document MAC address, location, date
- 📸 Take installation photo
- ✅ Verify first upload within 24 hours

**Monitoring:**
- 📊 Set Grafana alert for >48hr data gap
- 🔋 Monitor battery voltage trends
- ⚠️ Replace battery when <3.3V
- 🗓️ Plan quarterly maintenance check

### 8. Troubleshooting Guide

**Problem: Device won't wake from deep sleep**
- ✅ Check D0 (GPIO16) → RST connection (220Ω resistor)
- ✅ Verify 10kΩ pull-up on RST to 3.3V
- ✅ Test with LED on GPIO16 to see pulse
- ✅ Ensure sleep duration >15 seconds

**Problem: High sleep current (>100µA)**
- ✅ Measure at battery terminals with multimeter
- ✅ Verify WiFi.mode(WIFI_OFF) before sleep
- ✅ Check for floating GPIO pins
- ✅ Consider Wemos power LED removal

**Problem: Sensor returns NaN**
- ✅ Check AHT10 power on D6/GPIO12
- ✅ Verify I2C connections (D1=SCL, D2=SDA)
- ✅ Add 4.7kΩ pull-ups if not present
- ✅ Increase power-on delay to 200ms

**Problem: InfluxDB upload fails**
- ✅ Ping server from same network
- ✅ Check port 8086 not blocked by firewall
- ✅ Verify database name (case-sensitive)
- ✅ Test with curl command first
- ✅ Try without authentication

**Problem: Battery drains too fast**
- ✅ Measure actual sleep current
- ✅ Check measurement interval setting
- ✅ Verify deep sleep is actually happening
- ✅ Look for ROM full condition (auto-upload attempts)

### 9. Performance Targets

**Expected Performance (Wemos D1 Mini + MCP1700):**
- Deep sleep current: 50-80µA
- Measurement duration: 3-5 seconds
- WiFi connection time: 5-10 seconds
- Upload time (1024 records): 10-15 seconds
- Battery life (3000mAh, 30-min interval): 18+ months

**If not meeting targets:**
- Sleep current >100µA → Hardware issue (check connections)
- Measurement >10s → Sensor issue (check I2C)
- WiFi connection >20s → Network issue (check signal strength)
- Battery <12 months → Interval too frequent or sleep current high

### 10. Future Enhancement Ideas

**Add Barometric Pressure** (BMP280/BME280):
```cpp
#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp;

void performMeasurement() {
  // ... existing code ...
  float pressure = bmp.readPressure() / 100.0; // hPa
  // Add to sensor record or upload separately
}
```

**Add External Temperature Probe** (DS18B20):
```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_PIN 13 // D7
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);

void performMeasurement() {
  ds18b20.requestTemperatures();
  float extTemp = ds18b20.getTempCByIndex(0);
  // Great for outdoor sensor with indoor display
}
```

**Add Local OLED Display**:
```cpp
#include <Adafruit_SSD1306.h>
#define OLED_RESET -1
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.printf("Temp: %.1f C\n", temp);
  display.printf("Hum:  %.1f %%\n", hum);
  display.printf("Batt: %.2fV\n", voltage);
  display.printf("Next: %dm\n", config.interval/60);
  display.display();
}
```

## Summary

All your corrections have been applied:
✅ MCP1700 voltage regulator
✅ Wemos D1 Mini specific features  
✅ AHT10 on separate GPIO12 power pin
✅ 2-week to 1-month storage capacity
✅ WiFi only on button press
✅ RF disabled during timer sleep
✅ LED status indicators for all modes
✅ No static IP requirement

The system is now optimized for long-term autonomous operation with minimal power consumption and maximum data storage!
