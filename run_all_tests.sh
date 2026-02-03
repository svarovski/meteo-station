#!/bin/bash

# Script to run all tests individually using separate test environments
# Each test environment compiles only ONE test file

set -e  # Exit on first error

echo "========================================"
echo "Running ESP8266 Sensor Tests"
echo "========================================"
echo ""

echo "[1/4] Testing Config..."
pio test -e test_config
echo "✓ Config tests passed"
echo ""

echo "[2/4] Testing SensorRecord..."
pio test -e test_sensor_record
echo "✓ SensorRecord tests passed"
echo ""

echo "[3/4] Testing RTCData..."
pio test -e test_rtc_data
echo "✓ RTCData tests passed"
echo ""

echo "[4/4] Testing InfluxDBWrapper..."
pio test -e test_influxdb_wrapper
echo "✓ InfluxDBWrapper tests passed"
echo ""

echo "========================================"
echo "ALL TESTS PASSED! ✓"
echo "Total: 39 tests across 4 test files"
echo "========================================"
