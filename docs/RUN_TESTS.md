# Running Tests

## Test Structure

Each test is in its own directory under `test/`:

```
test/
├── test_config/
│   └── test_config.cpp           (7 tests)
├── test_sensor_record/
│   └── test_sensor_record.cpp    (11 tests)
├── test_rtc_data/
│   └── test_rtc_data.cpp         (10 tests)
└── test_influxdb_wrapper/
    └── test_influxdb_wrapper.cpp (11 tests)
```

This structure ensures each test compiles independently, avoiding multiple definition errors.

## Running Tests

### Run All Tests
```bash
pio test
```

PlatformIO will automatically discover and run each test directory.

### Run Specific Test
```bash
pio test -f test_config
pio test -f test_sensor_record
pio test -f test_rtc_data
pio test -f test_influxdb_wrapper
```

### Verbose Output
```bash
pio test -v
pio test -vv   # Even more verbose
pio test -vvv  # Maximum verbosity
```

## Expected Output

```
Testing...
test_config/*
  ✓ test_config_default_values
  ✓ test_config_magic_validation
  ... (5 more tests)
  
test_sensor_record/*
  ✓ test_sensor_record_create
  ✓ test_sensor_record_get_temperature
  ... (9 more tests)

test_rtc_data/*
  ✓ test_rtc_data_initialization
  ... (9 more tests)

test_influxdb_wrapper/*
  ✓ test_influxdb_client_initialization
  ... (10 more tests)

======================================
39 Tests 0 Failures 0 Ignored
SUCCESS
```

## Building Main Application

```bash
# Build only
pio run

# Build and upload
pio run --target upload

# Upload filesystem (HTML files)
pio run --target uploadfs
```

## Test Details

| Test Directory | Tests | What It Tests |
|----------------|-------|---------------|
| test_config | 7 | Configuration management, EEPROM, time offset |
| test_sensor_record | 11 | Data encoding, timestamp conversion, validation |
| test_rtc_data | 10 | RTC memory, buffer management, persistence |
| test_influxdb_wrapper | 11 | InfluxDB client, data upload, error handling |

## Troubleshooting

### Tests fail to compile
```bash
# Clean build
pio test -c

# Or clean everything
rm -rf .pio
pio test
```

### Device not found
Update `platformio.ini`:
```ini
upload_port = /dev/ttyUSB0  # or /dev/ttyACM0, etc.
```

### Permission denied
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

Tests run on actual ESP8266 hardware for complete validation! 🎯
