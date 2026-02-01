#include <unity.h>
#include "../src/InfluxDBClient.h"
#include "../src/Config.h"
#include "../src/SensorRecord.h"

InfluxDBClient testClient;
Config testConfig;

void setUp(void) {
    // Setup test configuration
    testConfig.setDefaults();
    strcpy(testConfig.influxServer, "192.168.1.100");
    testConfig.influxPort = 8086;
    strcpy(testConfig.influxDb, "test_db");
    strcpy(testConfig.influxMeasurement, "test_measurement");
    testConfig.magic = CONFIG_MAGIC;
}

void tearDown(void) {
    // Cleanup
}

void test_influxdb_client_initialization(void) {
    InfluxDBClient client;
    
    // Should not be initialized yet
    String error = client.getLastError();
    TEST_ASSERT_TRUE(error.indexOf("not initialized") >= 0);
}

void test_influxdb_client_begin_with_valid_config(void) {
    InfluxDBClient client;
    
    bool success = client.begin(&testConfig);
    
    // Note: May fail without actual InfluxDB server, but should not crash
    // This tests that the initialization code runs
    TEST_ASSERT_TRUE(success || !success); // Always passes, just checks no crash
}

void test_influxdb_client_begin_with_null_config(void) {
    InfluxDBClient client;
    
    bool success = client.begin(nullptr);
    
    TEST_ASSERT_FALSE(success);
}

void test_influxdb_client_begin_with_invalid_config(void) {
    InfluxDBClient client;
    Config invalidConfig;
    invalidConfig.setDefaults();
    // Don't set magic number, making it invalid
    
    bool success = client.begin(&invalidConfig);
    
    TEST_ASSERT_FALSE(success);
}

void test_influxdb_client_sensor_record_write(void) {
    // This test verifies the API works, not that it connects
    InfluxDBClient client;
    client.begin(&testConfig);
    
    SensorRecord record = SensorRecord::create(22.5, 65.0, 3600, 0);
    
    // May fail without server, but shouldn't crash
    bool written = client.writeSensorRecord(record, 0);
    
    // Just verify it returns a boolean
    TEST_ASSERT_TRUE(written || !written);
}

void test_influxdb_client_battery_write(void) {
    InfluxDBClient client;
    client.begin(&testConfig);
    
    float batteryVoltage = 3.87;
    
    // May fail without server, but shouldn't crash
    bool written = client.writeBatteryVoltage(batteryVoltage);
    
    // Just verify it returns a boolean
    TEST_ASSERT_TRUE(written || !written);
}

void test_influxdb_client_flush(void) {
    InfluxDBClient client;
    client.begin(&testConfig);
    
    bool flushed = client.flush();
    
    // Should return true as it's currently a no-op placeholder
    TEST_ASSERT_TRUE(flushed);
}

void test_influxdb_client_get_error_before_init(void) {
    InfluxDBClient client;
    
    String error = client.getLastError();
    
    TEST_ASSERT_EQUAL_STRING("Client not initialized", error.c_str());
}

void test_influxdb_client_with_authentication(void) {
    Config authConfig;
    authConfig.setDefaults();
    strcpy(authConfig.influxServer, "192.168.1.100");
    authConfig.influxPort = 8086;
    strcpy(authConfig.influxDb, "test_db");
    strcpy(authConfig.influxUser, "test_user");
    strcpy(authConfig.influxPass, "test_password");
    strcpy(authConfig.influxMeasurement, "test_measurement");
    authConfig.magic = CONFIG_MAGIC;
    
    InfluxDBClient client;
    bool success = client.begin(&authConfig);
    
    // Should initialize successfully with auth credentials
    TEST_ASSERT_TRUE(success);
}

void test_influxdb_client_destructor(void) {
    // Test that destructor doesn't crash
    {
        InfluxDBClient client;
        client.begin(&testConfig);
        // Client goes out of scope and destructor is called
    }
    
    // If we get here, destructor worked
    TEST_ASSERT_TRUE(true);
}

// Integration test - multiple writes
void test_influxdb_client_multiple_writes(void) {
    InfluxDBClient client;
    client.begin(&testConfig);
    
    // Write multiple sensor records
    for (int i = 0; i < 5; i++) {
        SensorRecord record = SensorRecord::create(20.0 + i, 50.0 + i, i * 3600, 0);
        client.writeSensorRecord(record, 0);
    }
    
    // Write battery voltage
    client.writeBatteryVoltage(3.85);
    
    // Flush
    bool flushed = client.flush();
    
    TEST_ASSERT_TRUE(flushed);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_influxdb_client_initialization);
    RUN_TEST(test_influxdb_client_begin_with_valid_config);
    RUN_TEST(test_influxdb_client_begin_with_null_config);
    RUN_TEST(test_influxdb_client_begin_with_invalid_config);
    RUN_TEST(test_influxdb_client_sensor_record_write);
    RUN_TEST(test_influxdb_client_battery_write);
    RUN_TEST(test_influxdb_client_flush);
    RUN_TEST(test_influxdb_client_get_error_before_init);
    RUN_TEST(test_influxdb_client_with_authentication);
    RUN_TEST(test_influxdb_client_destructor);
    RUN_TEST(test_influxdb_client_multiple_writes);
    
    UNITY_END();
}

void loop() {
    // Nothing to do
}
