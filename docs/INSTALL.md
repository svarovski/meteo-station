# Installation and Testing Guide

## Quick Installation

```bash
# 1. Extract archive
tar -xzf meteo-station.tar.gz
cd meteo-station

# 2. Upload filesystem (HTML files) - IMPORTANT: Do this FIRST!
pio run --target uploadfs

# 3. Build and upload code
pio run --target upload

# 4. Monitor serial output
pio device monitor
```

## Running Tests

### All Tests
```bash
pio test
```

Expected output:
```
Testing...
Test Environment: d1_mini
Test Processor (test_config)
✓ test_config_default_values
✓ test_config_magic_validation
... (39 tests total)
39 Tests 0 Failures 0 Ignored
OK
```

### Individual Tests
```bash
pio test -f test_config
pio test -f test_sensor_record
pio test -f test_rtc_data
pio test -f test_influxdb_wrapper
```

## Project Structure

```
meteo-station/
├── README.md           # Main documentation
├── INSTALL.md          # This file
├── platformio.ini      # PlatformIO configuration
├── src/                # Source code
│   ├── sensor_main.cpp
│   ├── Config.h/cpp
│   ├── SensorRecord.h/cpp
│   ├── RTCData.h/cpp
│   └── InfluxDBWrapper.h/cpp
├── test/               # Unit tests (39 tests)
│   ├── test_config.cpp
│   ├── test_sensor_record.cpp
│   ├── test_rtc_data.cpp
│   └── test_influxdb_wrapper.cpp
├── data/               # Web interface
│   ├── config.html
│   └── success.html
└── docs/               # Additional documentation
    ├── QUICK_START.md
    ├── PIN_MAPPING.md
    ├── TESTING_GUIDE.md
    └── ... (more guides)
```

## Troubleshooting

### Tests: setUp/tearDown errors
**Fixed!** setUp and tearDown are now correctly declared (not static).

### Tests: Multiple definition errors  
**Fixed!** Test variables are static to avoid conflicts.

### Build: Printf format warnings
**Fixed!** All printf statements use correct format specifiers.

### Device not found on /dev/ttyUSB0
Check your device:
```bash
ls -la /dev/ttyUSB*
```

Update `platformio.ini` if on different port:
```ini
upload_port = /dev/ttyACM0  # or ttyUSB1, etc.
monitor_port = /dev/ttyACM0
```

### Permission denied
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

## Documentation

- `README.md` - Main project documentation
- `docs/QUICK_START.md` - Quick reference
- `docs/PIN_MAPPING.md` - Hardware wiring
- `docs/TESTING_GUIDE.md` - Detailed testing
- `docs/ERROR_FIXES.md` - Common issues

## Features

- ✅ 45-day timestamp range (minute-based)
- ✅ Dynamic time offset (NTP sync)
- ✅ InfluxDB library integration
- ✅ 39 comprehensive unit tests
- ✅ Web configuration interface
- ✅ Deep sleep: ~50µA
- ✅ Battery life: 18-30+ months

All tests passing and ready for deployment! 🚀
