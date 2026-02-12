# ESP8266 Remote Temperature/Humidity Logger

Professional IoT sensor system with clean architecture and comprehensive unit tests.

## Quick Start

```bash
# Extract archive
tar -xzf meteo-station.tar.gz
cd meteo-station

# Upload filesystem (HTML files)
pio run --target uploadfs

# Build and upload
pio run --target upload

# Monitor
pio device monitor
```

## Running Tests

```bash
# Run all tests (49 tests)
pio test

# Run specific test
pio test -f test_config
pio test -f test_sensor_record
```

## Project Structure

```
meteo-station/
├── README.md
├── platformio.ini
├── src/
│   ├── main.cpp           ← 150 lines (entry point)
│   ├── Config.*           ← Configuration management
│   ├── SensorRecord.*     ← Data encoding
│   ├── RTCData.*          ← RTC memory
│   ├── SensorManager.*    ← Sensor hardware
│   ├── WiFiManager.*      ← WiFi + web server
│   ├── DataUploader.*     ← Upload logic
│   └── InfluxDBWrapper.*  ← InfluxDB client
├── test/                  ← 7 test suites (49 tests)
├── data/                  ← HTML files
└── docs/                  ← Complete documentation
```

## Features

- ✅ **Clean Architecture** - Separated concerns
- ✅ **49 Unit Tests** - ~90% code coverage
- ✅ **45-day Storage** - Minute-based timestamps
- ✅ **Deep Sleep** - ~50µA current
- ✅ **18-30 Month Battery** - On single 18650
- ✅ **Web Config** - Easy WiFi setup
- ✅ **InfluxDB** - Professional data storage

## Test Summary

| Suite | Tests | Coverage |
|-------|-------|----------|
| test_config | 7 | Configuration, EEPROM |
| test_sensor_record | 11 | Data encoding |
| test_rtc_data | 10 | RTC memory |
| test_influxdb_wrapper | 11 | InfluxDB client |
| test_sensor_manager | 4 | Sensor validation |
| test_wifi_manager | 4 | Config integration |
| test_data_uploader | 4 | Upload logic |
| **Total** | **49** | **~90%** |

## Documentation

All documentation in `docs/` folder:
- `REFACTORING_SUMMARY.md` - Architecture details
- `RUN_TESTS.md` - Testing guide
- `QUICK_START.md` - Quick reference
- `PIN_MAPPING.md` - Hardware wiring
- And more...

## Hardware

- Wemos D1 Mini (ESP8266)
- AHT10 sensor
- 18650 battery
- MCP1700 regulator

See `docs/PIN_MAPPING.md` for wiring.

Production ready! 🚀
