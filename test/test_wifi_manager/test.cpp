#include <unity.h>
#include "../lib/Config.h"
#include <EEPROM.h>

// Note: We can't fully test WiFiManager without network hardware
// These tests verify the Config integration which WiFiManager uses

static Config testConfig;

void setUp(void) {
    EEPROM.begin(512);
    testConfig.setDefaults();
    strcpy(testConfig.ssid, "TestSSID");
    strcpy(testConfig.password, "TestPassword");
    testConfig.magic = CONFIG_MAGIC;
}

void tearDown(void) {
}

void test_wifi_manager_config_setup(void) {
    // Verify config is properly set up for WiFiManager
    TEST_ASSERT_TRUE(testConfig.isValid());
    TEST_ASSERT_EQUAL_STRING("TestSSID", testConfig.ssid);
}

void test_wifi_manager_time_offset(void) {
    uint32_t testTime = 1704067200; // 2024-01-01
    testConfig.updateTimeOffset(testTime);
    
    // Should round to 65536 boundary
    TEST_ASSERT_TRUE(testConfig.timeOffset > 0);
}

void test_wifi_manager_time_offset_string(void) {
    testConfig.timeOffset = 1704067200;
    String timeStr = testConfig.getTimeOffsetString();
    
    TEST_ASSERT_TRUE(timeStr.length() > 0);
    TEST_ASSERT_TRUE(timeStr.indexOf("2024") >= 0);
}

void test_wifi_manager_config_persistence(void) {
    testConfig.save();
    
    Config loadedConfig;
    loadedConfig.load();
    
    TEST_ASSERT_TRUE(loadedConfig.isValid());
    TEST_ASSERT_EQUAL_STRING(testConfig.ssid, loadedConfig.ssid);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_wifi_manager_config_setup);
    RUN_TEST(test_wifi_manager_time_offset);
    RUN_TEST(test_wifi_manager_time_offset_string);
    RUN_TEST(test_wifi_manager_config_persistence);
    
    UNITY_END();
}

void loop() {
}
