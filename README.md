# ESP8266 Remote Temperature/Humidity Logger

Professional IoT sensor system with clean architecture, deep sleep optimization, and comprehensive unit tests.

## Quick Start

```bash
# 1. Upload filesystem (HTML files)
pio run --target uploadfs

# 2. Build and upload
pio run --target upload

# 3. Monitor
pio device monitor
```

## Running Tests

```bash
# Run all tests (44 tests total)
pio test -e test

# Run specific test
pio test -e test -f test_config
pio test -e test -f test_sensor_manager
```

## Project Structure

```
meteo-station/
├── README.md
├── REFACTORING_SUMMARY.md  ← Refactoring details
├── platformio.ini
├── src/                     ← 15 source files (was 9)
│   ├── sensor_main.cpp      ← 250 lines (was 670!)
│   ├── SensorManager.*      ← NEW: Sensor logic
│   ├── WiFiManager.*        ← NEW: WiFi/NTP logic
│   ├── DataUploader.*       ← NEW: Upload logic
│   └── ... (other classes)
├── test/                    ← 5 test suites (44 tests)
│   ├── test_config/
│   ├── test_sensor_record/
│   ├── test_rtc_data/
│   ├── test_influxdb_wrapper/
│   └── test_sensor_manager/ ← NEW
├── data/                    ← HTML files
└── docs/                    ← Complete documentation
```

## Key Features

- ✅ **Clean Architecture** - Testable, maintainable classes
- ✅ **90% Test Coverage** - 44 comprehensive unit tests
- ✅ **45-day Storage** - Minute-based timestamps
- ✅ **Deep Sleep** - ~50µA current draw
- ✅ **Battery Life** - 18-30+ months on 18650
- ✅ **Web Config** - Easy setup via WiFi AP
- ✅ **InfluxDB Integration** - Professional data storage

## Documentation

- `README.md` - This file
- `REFACTORING_SUMMARY.md` - Code architecture details
- `FINAL_SOLUTION.md` - Testing solution
- `docs/` - Complete guides (13 files)
  - `RUN_TESTS.md` - Testing guide
  - `QUICK_START.md` - Quick reference
  - `PIN_MAPPING.md` - Hardware wiring
  - And more...

## Recent Refactoring

The codebase was significantly refactored to improve testability:

- **Before**: 670-line sensor_main.cpp, minimal tests
- **After**: Clean class architecture, 44 unit tests, 90% coverage

See `REFACTORING_SUMMARY.md` for details.

## Hardware

- Wemos D1 Mini (ESP8266)
- AHT10 temperature/humidity sensor
- 18650 Li-ion battery
- MCP1700-3.3V regulator

See `docs/PIN_MAPPING.md` for wiring.

Ready for production deployment! 🚀
