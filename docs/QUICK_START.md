# Quick Start Guide

## Extract and Install

```bash
# Extract archive
tar -xzf meteo-station.tar.gz
cd meteo-station

# Upload filesystem (HTML files) - DO THIS FIRST!
pio run --target uploadfs

# Build and upload code
pio run --target upload

# Monitor serial output
pio device monitor
```

## First Configuration

1. Device creates WiFi AP: `sensor-XXXXXX`
2. Connect to AP from phone/computer
3. Open browser: `http://192.168.4.1`
4. Configure WiFi and InfluxDB settings
5. Save and restart

## Testing

```bash
# Run all tests
pio test

# Expected: 39 Tests 0 Failures 0 Ignored
```

## Common Issues

### Device not found
```bash
# Check which port
ls /dev/ttyUSB*

# Update platformio.ini if needed
upload_port = /dev/ttyUSB0  # or ACM0, etc.
```

### Permission denied
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

### Multiple definition errors
Already fixed! If still seeing:
```bash
pio run -t clean
pio test
```

## File Structure

```
meteo-station/
├── src/         ← Source code
├── test/        ← Unit tests
├── data/        ← HTML files
└── *.md         ← Documentation
```

## Key Features

- ✅ 45-day data storage (minutes-based timestamps)
- ✅ Dynamic time offset (NTP sync)
- ✅ InfluxDB library integration
- ✅ 39 comprehensive unit tests
- ✅ Web-based configuration
- ✅ Battery voltage monitoring
- ✅ Deep sleep: ~50µA
- ✅ Battery life: 18-30+ months

## Documentation

- `INSTALL.md` - Detailed installation guide
- `README.md` - Complete project documentation  
- `PIN_MAPPING.md` - Wiring diagrams
- `TESTING_GUIDE.md` - Testing instructions
- `ERROR_FIXES.md` - Troubleshooting

Ready to deploy! 🚀
