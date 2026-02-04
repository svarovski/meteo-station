# ESP8266 Remote Temperature/Humidity Logger

Complete IoT sensor system with deep sleep, data buffering, and InfluxDB integration.

## Quick Start

```bash
# 1. Upload filesystem (HTML files)
pio run --target uploadfs

# 2. Build and upload code
pio run --target upload

# 3. Monitor serial output
pio device monitor
```

## Running Tests

Each test is in its own directory under `test/`:

```bash
# Run all tests
pio test

# Run specific test
pio test -f test_config
pio test -f test_sensor_record
pio test -f test_rtc_data
pio test -f test_influxdb_wrapper
```

**Total: 39 tests across 4 test suites**

## Documentation

- `README.md` - This file (quick start)
- `docs/` - Complete documentation
  - `RUN_TESTS.md` - Testing guide
  - `QUICK_START.md` - Quick reference
  - `PIN_MAPPING.md` - Hardware wiring
  - And 9 more guides...

## Features

- ✅ 45-day data storage (minute-based timestamps)
- ✅ Dynamic time offset (NTP sync)
- ✅ 39 comprehensive unit tests
- ✅ Web configuration interface
- ✅ Deep sleep: ~50µA
- ✅ Battery life: 18-30+ months

For complete documentation, see files in `docs/` directory.
