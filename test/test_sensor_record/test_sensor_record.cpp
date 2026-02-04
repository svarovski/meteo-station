#include <unity.h>
#include "../src/SensorRecord.h"

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_sensor_record_create(void) {
    float temp = 22.5;
    float hum = 65.0;
    uint32_t timestampSeconds = 3600; // 1 hour in seconds
    uint32_t offsetSeconds = 0;
    
    SensorRecord record = SensorRecord::create(temp, hum, timestampSeconds, offsetSeconds);
    
    // Timestamp should be 60 minutes (3600 seconds / 60)
    TEST_ASSERT_EQUAL(60, record.timestamp);
    
    // Temperature: 22.5 + 100 = 122.5 → 122 (truncated to int8)
    TEST_ASSERT_EQUAL(122, record.temperature);
    
    // Humidity: 65
    TEST_ASSERT_EQUAL(65, record.humidity);
}

void test_sensor_record_get_temperature(void) {
    SensorRecord record;
    record.temperature = 122; // 22°C (122 - 100)
    
    float temp = record.getTemperature();
    
    TEST_ASSERT_FLOAT_WITHIN(0.1, 22.0, temp);
}

void test_sensor_record_get_humidity(void) {
    SensorRecord record;
    record.humidity = 65;
    
    float hum = record.getHumidity();
    
    TEST_ASSERT_FLOAT_WITHIN(0.1, 65.0, hum);
}

void test_sensor_record_get_timestamp_seconds(void) {
    SensorRecord record;
    record.timestamp = 60; // 60 minutes stored
    
    uint32_t offsetSeconds = 0;
    uint32_t timestampSeconds = record.getTimestampSeconds(offsetSeconds);
    
    // 60 minutes = 3600 seconds
    TEST_ASSERT_EQUAL(3600, timestampSeconds);
}

void test_sensor_record_timestamp_with_offset(void) {
    // Create record at 2 hours with 1 hour offset
    uint32_t timestampSeconds = 7200; // 2 hours
    uint32_t offsetSeconds = 3600;    // 1 hour offset
    
    SensorRecord record = SensorRecord::create(20.0, 50.0, timestampSeconds, offsetSeconds);
    
    // Difference: 7200 - 3600 = 3600 seconds = 60 minutes
    TEST_ASSERT_EQUAL(60, record.timestamp);
    
    // Reconstruct should give original timestamp
    uint32_t reconstructed = record.getTimestampSeconds(offsetSeconds);
    TEST_ASSERT_EQUAL(timestampSeconds, reconstructed);
}

void test_sensor_record_temperature_range(void) {
    // Test minimum temperature
    SensorRecord minRecord = SensorRecord::create(-100.0, 50.0, 0, 0);
    TEST_ASSERT_EQUAL(0, minRecord.temperature); // -100 + 100 = 0
    TEST_ASSERT_FLOAT_WITHIN(0.1, -100.0, minRecord.getTemperature());
    
    // Test maximum temperature
    SensorRecord maxRecord = SensorRecord::create(155.0, 50.0, 0, 0);
    TEST_ASSERT_EQUAL(255, maxRecord.temperature); // 155 + 100 = 255
    TEST_ASSERT_FLOAT_WITHIN(0.1, 155.0, maxRecord.getTemperature());
}

void test_sensor_record_humidity_range(void) {
    // Test minimum humidity
    SensorRecord minRecord = SensorRecord::create(20.0, 0.0, 0, 0);
    TEST_ASSERT_EQUAL(0, minRecord.humidity);
    
    // Test maximum humidity
    SensorRecord maxRecord = SensorRecord::create(20.0, 100.0, 0, 0);
    TEST_ASSERT_EQUAL(100, maxRecord.humidity);
}

void test_sensor_record_is_valid(void) {
    // Valid record
    SensorRecord validRecord = SensorRecord::create(22.5, 65.0, 0, 0);
    TEST_ASSERT_TRUE(validRecord.isValid());
    
    // Invalid temperature (manually set out of range)
    SensorRecord invalidTemp;
    invalidTemp.temperature = 255; // Will decode to 155°C (still valid)
    invalidTemp.humidity = 50;
    TEST_ASSERT_TRUE(invalidTemp.isValid());
    
    // Valid humidity range
    SensorRecord validHum;
    validHum.temperature = 120; // 20°C
    validHum.humidity = 100;
    TEST_ASSERT_TRUE(validHum.isValid());
}

void test_sensor_record_influx_line_protocol(void) {
    SensorRecord record = SensorRecord::create(22.5, 65.0, 3600, 0);
    
    String line = record.toInfluxLine("environment", 0);
    
    // Should contain measurement name
    TEST_ASSERT_TRUE(line.indexOf("environment") >= 0);
    
    // Should contain temperature field
    TEST_ASSERT_TRUE(line.indexOf("temperature=22") >= 0);
    
    // Should contain humidity field
    TEST_ASSERT_TRUE(line.indexOf("humidity=65") >= 0);
    
    // Should contain timestamp in nanoseconds (3600 + 9 zeros)
    TEST_ASSERT_TRUE(line.indexOf("3600000000000") >= 0);
}

void test_sensor_record_minutes_overflow(void) {
    // Test maximum timestamp (65535 minutes = ~45 days)
    uint32_t maxMinutes = 65535;
    uint32_t maxSeconds = maxMinutes * 60;
    
    SensorRecord record = SensorRecord::create(20.0, 50.0, maxSeconds, 0);
    
    // Should wrap at 16-bit boundary
    TEST_ASSERT_EQUAL(65535, record.timestamp);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_sensor_record_create);
    RUN_TEST(test_sensor_record_get_temperature);
    RUN_TEST(test_sensor_record_get_humidity);
    RUN_TEST(test_sensor_record_get_timestamp_seconds);
    RUN_TEST(test_sensor_record_timestamp_with_offset);
    RUN_TEST(test_sensor_record_temperature_range);
    RUN_TEST(test_sensor_record_humidity_range);
    RUN_TEST(test_sensor_record_is_valid);
    RUN_TEST(test_sensor_record_influx_line_protocol);
    RUN_TEST(test_sensor_record_minutes_overflow);
    
    UNITY_END();
}

void loop() {
    // Nothing to do
}
