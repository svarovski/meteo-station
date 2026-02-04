#include <unity.h>
#include "../../src/SensorManager.h"

void setUp(void) {
}

void tearDown(void) {
}

void test_sensor_manager_creation(void) {
    SensorManager sensor(12);
    TEST_ASSERT_TRUE(true); // Constructor doesn't crash
}

void test_sensor_manager_validate_readings_valid(void) {
    SensorManager sensor(12);
    
    TEST_ASSERT_TRUE(sensor.validateReadings(20.0, 50.0));
    TEST_ASSERT_TRUE(sensor.validateReadings(0.0, 100.0));
    TEST_ASSERT_TRUE(sensor.validateReadings(-40.0, 0.0));
    TEST_ASSERT_TRUE(sensor.validateReadings(85.0, 100.0));
}

void test_sensor_manager_validate_readings_invalid_temp(void) {
    SensorManager sensor(12);
    
    TEST_ASSERT_FALSE(sensor.validateReadings(-50.0, 50.0));  // Too cold
    TEST_ASSERT_FALSE(sensor.validateReadings(100.0, 50.0));  // Too hot
    TEST_ASSERT_FALSE(sensor.validateReadings(NAN, 50.0));    // NaN
}

void test_sensor_manager_validate_readings_invalid_humidity(void) {
    SensorManager sensor(12);
    
    TEST_ASSERT_FALSE(sensor.validateReadings(20.0, -10.0));  // Negative
    TEST_ASSERT_FALSE(sensor.validateReadings(20.0, 150.0));  // Too high
    TEST_ASSERT_FALSE(sensor.validateReadings(20.0, NAN));    // NaN
}

void test_sensor_manager_create_record(void) {
    SensorManager sensor(12);
    
    SensorRecord record = sensor.createRecord(22.5, 65.0, 3600, 0);
    
    TEST_ASSERT_EQUAL(60, record.timestamp);  // 3600 seconds = 60 minutes
    TEST_ASSERT_FLOAT_WITHIN(0.5, 22.5, record.getTemperature());
    TEST_ASSERT_FLOAT_WITHIN(0.5, 65.0, record.getHumidity());
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_sensor_manager_creation);
    RUN_TEST(test_sensor_manager_validate_readings_valid);
    RUN_TEST(test_sensor_manager_validate_readings_invalid_temp);
    RUN_TEST(test_sensor_manager_validate_readings_invalid_humidity);
    RUN_TEST(test_sensor_manager_create_record);
    
    UNITY_END();
}

void loop() {
}
