#include <unity.h>
#include "../../lib/sensor/SensorRecord.h"

void setUp(void) {
}

void tearDown(void) {
}

void test_sensor_record_create(void) {
    float temp = 22.5;
    float hum = 65.0;
    uint32_t timestampSeconds = 3600;
    uint32_t offsetSeconds = 0;
    
    SensorRecord record = SensorRecord::create(temp, hum, timestampSeconds, offsetSeconds);
    
    TEST_ASSERT_EQUAL(60, record.timestamp);
    TEST_ASSERT_FLOAT_WITHIN(1.0, 22.0, record.getTemperature());  // Truncation
    TEST_ASSERT_FLOAT_WITHIN(1.0, 65.0, record.getHumidity());
}

void test_sensor_record_get_temperature(void) {
    SensorRecord record;
    record.temperature = 122;
    
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
    record.timestamp = 60;
    
    uint32_t offsetSeconds = 0;
    uint32_t timestampSeconds = record.getTimestampSeconds(offsetSeconds);
    
    TEST_ASSERT_EQUAL(3600, timestampSeconds);
}

void test_sensor_record_timestamp_with_offset(void) {
    uint32_t timestampSeconds = 7200;
    uint32_t offsetSeconds = 3600;
    
    SensorRecord record = SensorRecord::create(20.0, 50.0, timestampSeconds, offsetSeconds);
    
    TEST_ASSERT_EQUAL(60, record.timestamp);
    
    uint32_t reconstructed = record.getTimestampSeconds(offsetSeconds);
    TEST_ASSERT_EQUAL(timestampSeconds, reconstructed);
}

void test_sensor_record_temperature_range(void) {
    // Test minimum temperature
    SensorRecord minRecord = SensorRecord::create(-100.0, 50.0, 0, 0);
    TEST_ASSERT_EQUAL(0, minRecord.temperature);  // -100 + 100 = 0
    TEST_ASSERT_FLOAT_WITHIN(0.1, -100.0, minRecord.getTemperature());
    
    // Test maximum temperature - constrain clips to 255
    SensorRecord maxRecord = SensorRecord::create(155.0, 50.0, 0, 0);
    // 155 + 100 = 255, which is correct
    TEST_ASSERT_EQUAL(255, (uint8_t)maxRecord.temperature);
    TEST_ASSERT_FLOAT_WITHIN(1.0, 155.0, maxRecord.getTemperature());
}

void test_sensor_record_humidity_range(void) {
    SensorRecord minRecord = SensorRecord::create(20.0, 0.0, 0, 0);
    TEST_ASSERT_EQUAL(0, minRecord.humidity);
    
    SensorRecord maxRecord = SensorRecord::create(20.0, 100.0, 0, 0);
    TEST_ASSERT_EQUAL(100, maxRecord.humidity);
}

void test_sensor_record_is_valid(void) {
    // Valid record
    SensorRecord validRecord = SensorRecord::create(22.5, 65.0, 0, 0);
    TEST_ASSERT_TRUE(validRecord.isValid());
    
    // Edge cases - still valid
    SensorRecord edge1 = SensorRecord::create(-100.0, 0.0, 0, 0);
    TEST_ASSERT_TRUE(edge1.isValid());
    
    SensorRecord edge2 = SensorRecord::create(155.0, 100.0, 0, 0);
    TEST_ASSERT_TRUE(edge2.isValid());
}

void test_sensor_record_influx_line_protocol(void) {
    SensorRecord record = SensorRecord::create(22.5, 65.0, 3600, 0);
    
    String line = record.toInfluxLine("environment", 0);
    
    TEST_ASSERT_TRUE(line.indexOf("environment") >= 0);
    TEST_ASSERT_TRUE(line.indexOf("temperature=") >= 0);
    TEST_ASSERT_TRUE(line.indexOf("humidity=") >= 0);
    TEST_ASSERT_TRUE(line.indexOf("3600000000000") >= 0);
}

void test_sensor_record_minutes_overflow(void) {
    uint32_t maxMinutes = 65535;
    uint32_t maxSeconds = maxMinutes * 60;
    
    SensorRecord record = SensorRecord::create(20.0, 50.0, maxSeconds, 0);
    
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
}
