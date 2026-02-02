#include <unity.h>
#include "../src/Config.h"
#include <EEPROM.h>

static Config testConfig;

static void setUp(void) {
    // Initialize EEPROM for testing
    EEPROM.begin(512);
}

static void tearDown(void) {
    // Clean up after each test
}

void test_config_default_values(void) {
    testConfig.setDefaults();
    
    TEST_ASSERT_EQUAL(1800, testConfig.interval);
    TEST_ASSERT_EQUAL(8086, testConfig.influxPort);
    TEST_ASSERT_EQUAL_STRING("environment", testConfig.influxMeasurement);
    TEST_ASSERT_EQUAL(0, testConfig.timeOffset);
}

void test_config_magic_validation(void) {
    testConfig.setDefaults();
    
    // Before setting magic, should be invalid
    TEST_ASSERT_FALSE(testConfig.isValid());
    
    // Set magic number
    testConfig.magic = CONFIG_MAGIC;
    TEST_ASSERT_TRUE(testConfig.isValid());
}

void test_config_save_and_load(void) {
    // Create config with test values
    Config saveConfig;
    saveConfig.setDefaults();
    strcpy(saveConfig.ssid, "TestSSID");
    strcpy(saveConfig.password, "TestPassword");
    saveConfig.interval = 3600;
    saveConfig.save();
    
    // Load into new config
    Config loadConfig;
    bool loaded = loadConfig.load();
    
    TEST_ASSERT_TRUE(loaded);
    TEST_ASSERT_EQUAL_STRING("TestSSID", loadConfig.ssid);
    TEST_ASSERT_EQUAL_STRING("TestPassword", loadConfig.password);
    TEST_ASSERT_EQUAL(3600, loadConfig.interval);
}

void test_config_time_offset_update(void) {
    testConfig.setDefaults();
    
    // Test time offset calculation
    uint32_t testTime = 1704067200; // 2024-01-01 00:00:00 UTC
    testConfig.updateTimeOffset(testTime);
    
    // Should round down to 65536-minute boundary
    uint32_t expectedOffset = (testTime / 65536) * 65536;
    TEST_ASSERT_EQUAL(expectedOffset, testConfig.timeOffset);
}

void test_config_time_offset_string(void) {
    testConfig.setDefaults();
    testConfig.timeOffset = 1704067200; // 2024-01-01 00:00:00 UTC
    
    String timeStr = testConfig.getTimeOffsetString();
    
    // Should contain year 2024
    TEST_ASSERT_TRUE(timeStr.indexOf("2024") >= 0);
}

void test_config_load_invalid(void) {
    // Clear EEPROM at config address
    for (size_t i = 0; i < sizeof(Config); i++) {
        EEPROM.write(CONFIG_ADDR + i, 0);
    }
    EEPROM.commit();
    
    Config loadConfig;
    bool loaded = loadConfig.load();
    
    // Should fail to load and set defaults
    TEST_ASSERT_FALSE(loaded);
    TEST_ASSERT_EQUAL(1800, loadConfig.interval); // Default value
}

void setup() {
    delay(2000); // Wait for serial
    
    UNITY_BEGIN();
    
    RUN_TEST(test_config_default_values);
    RUN_TEST(test_config_magic_validation);
    RUN_TEST(test_config_save_and_load);
    RUN_TEST(test_config_time_offset_update);
    RUN_TEST(test_config_time_offset_string);
    RUN_TEST(test_config_load_invalid);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
