# How to Run Tests

## The Problem with Unity Tests on ESP8266

PlatformIO tries to compile ALL test files together into one firmware, which causes:
- Multiple definition of `setup()` and `loop()` (each test has these)
- Multiple definition of `setUp()` and `tearDown()`
- LittleFS library conflicts

## Solution: Run Each Test Separately

You MUST run each test file individually:

```bash
# Test Config
pio test -f test_config

# Test SensorRecord  
pio test -f test_sensor_record

# Test RTCData
pio test -f test_rtc_data

# Test InfluxDBWrapper
pio test -f test_influxdb_wrapper
```

## Or Use This Script

Create `run_all_tests.sh`:

```bash
#!/bin/bash

echo "Running all tests..."
echo "===================="

pio test -f test_config
if [ $? -ne 0 ]; then echo "test_config FAILED"; exit 1; fi

pio test -f test_sensor_record
if [ $? -ne 0 ]; then echo "test_sensor_record FAILED"; exit 1; fi

pio test -f test_rtc_data
if [ $? -ne 0 ]; then echo "test_rtc_data FAILED"; exit 1; fi

pio test -f test_influxdb_wrapper
if [ $? -ne 0 ]; then echo "test_influxdb_wrapper FAILED"; exit 1; fi

echo "===================="
echo "All tests PASSED! ✓"
```

Then:
```bash
chmod +x run_all_tests.sh
./run_all_tests.sh
```

## Why Can't We Run All Tests Together?

Unity framework on embedded devices requires each test file to have:
- `void setup()` - Initializes Unity and runs tests
- `void loop()` - Empty loop
- `void setUp()` - Called before each test
- `void tearDown()` - Called after each test

When you have multiple test files, you get multiple definitions of these functions.

## Alternative: Native Testing (Future Enhancement)

For running all tests together, you would need to:
1. Use PlatformIO native testing (runs on your computer, not on ESP8266)
2. Mock all ESP8266-specific functions
3. This is more complex but allows running all tests in one go

For now, running tests individually is the standard approach for embedded Unity tests.

## Expected Output (Per Test)

```
Testing test_config
Test Environment: d1_mini
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

Repeat for each test file (4 files total = 39 tests combined).
