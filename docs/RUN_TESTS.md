# How to Run Tests

## Solution: Separate Test Environments

Each test file has its own environment in `platformio.ini`:
- `env:test_config`
- `env:test_sensor_record`
- `env:test_rtc_data`
- `env:test_influxdb_wrapper`

This prevents multiple definition errors by compiling only ONE test file at a time.

## Running Tests

### Option 1: Automated Script (Recommended)
```bash
./run_all_tests.sh
```

### Option 2: Individual Tests
```bash
# Test Config (7 tests)
pio test -e test_config

# Test SensorRecord (11 tests)
pio test -e test_sensor_record

# Test RTCData (10 tests)
pio test -e test_rtc_data

# Test InfluxDBWrapper (11 tests)
pio test -e test_influxdb_wrapper
```

### Option 3: All Tests (One Command)
```bash
pio test -e test_config -e test_sensor_record -e test_rtc_data -e test_influxdb_wrapper
```

## Expected Output (Per Test Environment)

```
Testing test_config
Environment: test_config
...
✓ test_config_default_values
✓ test_config_magic_validation  
✓ test_config_save_and_load
✓ test_config_time_offset_update
✓ test_config_time_offset_string
✓ test_config_load_invalid

6 Tests 0 Failures 0 Ignored
OK
```

## Building Main Application

To build and upload the main application (not tests):

```bash
# Build
pio run -e d1_mini

# Upload
pio run -e d1_mini --target upload

# Monitor
pio device monitor
```

## Why Separate Environments?

PlatformIO's default behavior compiles ALL test files together, causing:
- Multiple definition of `setup()`, `loop()`, `setUp()`, `tearDown()`

Solution: Use `test_filter` in separate environments to compile only one test file per environment.

## Total Test Count

- **test_config**: 7 tests
- **test_sensor_record**: 11 tests  
- **test_rtc_data**: 10 tests
- **test_influxdb_wrapper**: 11 tests
- **TOTAL**: 39 tests

All tests run on actual ESP8266 hardware for real-world validation! 🎯
