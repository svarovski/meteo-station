#include <unity.h>

#ifdef NATIVE
#include "../native_mocks/Arduino.h"
#include "../native_mocks/EEPROM.h"
#endif

#include "../lib/Config.h"

#ifndef NATIVE
#include <EEPROM.h>
#endif

static Config testConfig;

void setUp(void) {
    EEPROM.begin(512);
}

void tearDown(void) {
}

void test_config_default_values(void) {
    Config config;
    config.setDefaults();
    
    TEST_ASSERT_EQUAL(1800, config.interval);
    TEST_ASSERT_EQUAL(8086, config.influxPort);
    TEST_ASSERT_EQUAL_STRING("environment", config.influxMeasurement);
}

void test_config_magic_validation(void) {
    testConfig.setDefaults();
    testConfig.magic = CONFIG_MAGIC;
    
    TEST_ASSERT_TRUE(testConfig.isValid());
    
    testConfig.magic = 0;
    TEST_ASSERT_FALSE(testConfig.isValid());
}

void test_config_save_and_load(void) {
    testConfig.setDefaults();
    strcpy(testConfig.ssid, "TestNetwork");
    strcpy(testConfig.password, "TestPass123");
    testConfig.interval = 3600;
    testConfig.save();
    
    Config loadedConfig;
    loadedConfig.load();
    
    TEST_ASSERT_TRUE(loadedConfig.isValid());
    TEST_ASSERT_EQUAL_STRING("TestNetwork", loadedConfig.ssid);
    TEST_ASSERT_EQUAL_STRING("TestPass123", loadedConfig.password);
    TEST_ASSERT_EQUAL(3600, loadedConfig.interval);
}

void test_config_time_offset_update(void) {
    testConfig.setDefaults();
    
    uint32_t testTime = 1704067200;
    testConfig.updateTimeOffset(testTime);
    
    TEST_ASSERT_TRUE(testConfig.timeOffset > 0);
    TEST_ASSERT_EQUAL(0, testConfig.timeOffset % 65536);
}

void test_config_time_offset_string(void) {
    testConfig.setDefaults();
    testConfig.timeOffset = 1704067200;
    
    String timeStr = testConfig.getTimeOffsetString();
    
    TEST_ASSERT_TRUE(timeStr.length() > 0);
}

void test_config_load_invalid(void) {
    Config invalidConfig;
    invalidConfig.magic = 0x12345678;
    EEPROM.put(CONFIG_ADDR, invalidConfig);
    
    Config loaded;
    loaded.load();
    
    TEST_ASSERT_FALSE(loaded.isValid());
}

void setup() {
    delay(2000);
    
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
}
