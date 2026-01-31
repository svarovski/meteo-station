# Project Structure and Setup Guide

## File Organization

```
project/
├── platformio.ini          # PlatformIO configuration
├── data/                   # LittleFS filesystem (HTML files)
│   ├── config.html        # Configuration page
│   └── success.html       # Success page
├── src/                    # Source files
│   ├── sensor_main.cpp    # Main application
│   ├── Config.h           # Configuration class
│   ├── Config.cpp
│   ├── SensorRecord.h     # Sensor data record class
│   ├── SensorRecord.cpp
│   ├── RTCData.h          # RTC memory management class
│   └── RTCData.cpp
└── README.md
```

## Setup Instructions

### 1. Install PlatformIO

If using VS Code:
```bash
# Install PlatformIO IDE extension from VS Code marketplace
```

If using CLI:
```bash
pip install platformio
```

### 2. Create Project Structure

```bash
# Create project directory
mkdir sensor-project
cd sensor-project

# Initialize PlatformIO project
pio init --board d1_mini

# Create data directory for HTML files
mkdir data
```

### 3. Copy Files

Copy all files to appropriate locations:
- `platformio.ini` → project root
- `sensor_main.cpp` → `src/`
- `Config.h`, `Config.cpp` → `src/`
- `SensorRecord.h`, `SensorRecord.cpp` → `src/`
- `RTCData.h`, `RTCData.cpp` → `src/`
- `config.html`, `success.html` → `data/`

### 4. Upload Filesystem (HTML Files)

**Important:** Upload filesystem BEFORE uploading code!

```bash
# Upload LittleFS filesystem with HTML files
pio run --target uploadfs

# Or in VS Code: PlatformIO > Upload File System Image
```

### 5. Upload Code

```bash
# Compile and upload
pio run --target upload

# Or in VS Code: PlatformIO > Upload
```

### 6. Monitor Serial Output

```bash
pio device monitor

# Or in VS Code: PlatformIO > Serial Monitor
```

## Key Changes from Previous Version

### 1. Object-Oriented Design

**Config Class:**
- Encapsulates all configuration data
- Methods: `load()`, `save()`, `isValid()`, `setDefaults()`, `print()`
- Self-contained EEPROM management

**SensorRecord Class:**
- Compact 4-byte data structure
- Methods: `create()`, `getTemperature()`, `getHumidity()`, `toInfluxLine()`
- Validation and conversion built-in

**RTCData Class:**
- RTC memory management
- Methods: `load()`, `save()`, `addRecord()`, `isBufferFull()`
- Handles initialization and persistence

### 2. External HTML Files

**Benefits:**
- Easier to edit HTML (syntax highlighting, formatting)
- No need to escape quotes or deal with C++ string concatenation
- Can edit HTML without recompiling code
- Cleaner separation of concerns

**How it works:**
1. HTML files stored in `data/` folder
2. Uploaded to LittleFS filesystem on ESP8266
3. Loaded at runtime with `LittleFS.open()`
4. Variables replaced with `String.replace()`

### 3. Function Ordering

Functions ordered logically to minimize forward declarations:
1. Utility functions (no dependencies)
2. Sensor functions (use utilities)
3. Storage functions (use sensor data)
4. Measurement functions (use sensor + storage)
5. Network functions (independent)
6. InfluxDB functions (use network)
7. Web server functions (use filesystem)
8. Setup and loop

### 4. File Extension

Changed from `.ino` to `.cpp`:
- `.ino` is Arduino IDE specific
- `.cpp` is standard C++ (better for PlatformIO)
- PlatformIO automatically handles Arduino framework

## Library Dependencies

All automatically installed via `platformio.ini`:

```ini
lib_deps = 
    adafruit/Adafruit AHTX0@^2.0.5
    adafruit/Adafruit BusIO@^1.16.1
    adafruit/Adafruit Unified Sensor@^1.1.14
```

## Memory Usage

### Flash Memory:
- Code: ~280KB
- HTML files: ~3KB (in LittleFS)
- Total: ~283KB / 4MB (7%)

### RAM Usage:
- Config object: ~300 bytes
- RTCData object: ~600 bytes (includes 128-record buffer)
- WiFi/HTTP buffers: ~5KB
- Stack: ~2KB
- Total: ~8KB / 40KB (20%)

## Troubleshooting

### "LittleFS mount failed"
**Cause:** Filesystem not uploaded
**Solution:** 
```bash
pio run --target uploadfs
```

### "File not found: /config.html"
**Cause:** HTML files not in data/ folder or filesystem not uploaded
**Solution:**
1. Verify files in `data/config.html` and `data/success.html`
2. Run `pio run --target uploadfs`

### Compilation errors about missing classes
**Cause:** Files not in src/ directory
**Solution:** Ensure all `.h` and `.cpp` files are in `src/` folder

### Can't upload filesystem
**Cause:** Serial monitor open or wrong board
**Solution:**
1. Close serial monitor
2. Check `platformio.ini` has correct board (`d1_mini`)

## VS Code PlatformIO Tasks

Common tasks in VS Code sidebar (PlatformIO icon):

- **Build** - Compile code
- **Upload** - Upload code to board
- **Upload File System Image** - Upload HTML files
- **Serial Monitor** - View serial output
- **Clean** - Clean build files

## Calibration

After first upload, calibrate battery voltage:

1. Measure actual battery voltage with multimeter
2. Note ADC reading from serial monitor
3. Update in `sensor_main.cpp`:

```cpp
float readBatteryVoltage() {
    int adcValue = analogRead(BATTERY_PIN);
    
    // ADJUST THIS FACTOR based on calibration
    float voltage = (adcValue / 1024.0) * 4.2;  // Change 4.2 to match your readings
    
    return voltage;
}
```

## Configuration via Web Interface

1. Power on device
2. Hold FLASH button during boot for >5 seconds (or first boot)
3. Connect to WiFi network: `sensor-XXXXXX`
4. Open browser to `http://192.168.4.1`
5. Fill in configuration
6. Click "Save Configuration & Restart"

## Adding New Sensors

Example: Add BMP280 pressure sensor

1. Add library to `platformio.ini`:
```ini
lib_deps = 
    ...existing...
    adafruit/Adafruit BMP280 Library@^2.6.8
```

2. Modify `SensorRecord.h` to add pressure field:
```cpp
struct SensorRecord {
    uint16_t timestamp;
    int8_t temperature;
    uint8_t humidity;
    uint8_t pressure;  // Add this (5 bytes total now)
    ...
};
```

3. Update `toInfluxLine()` in `SensorRecord.cpp`
4. Read pressure in `performMeasurement()`

## Best Practices

1. **Always upload filesystem first** before code
2. **Test HTML changes** by uploading filesystem only
3. **Use Serial Monitor** to debug issues
4. **Back up your data/** folder (contains your HTML)
5. **Version control** your src/ and data/ folders

## Next Steps

See other documentation files:
- `README.md` - Hardware setup and features
- `PIN_MAPPING.md` - Wiring diagrams
- `CORRECTIONS_AND_RECOMMENDATIONS.md` - Optimization tips
