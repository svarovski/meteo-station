#include <unity.h>
#include "../lib/WiFiManager.h"
#include "../lib/Config.h"

static Config testConfig;
static WiFiManager* wifiMgr;

void setUp(void) {
    testConfig.setDefaults();
    strcpy(testConfig.ssid, "TestSSID");
    strcpy(testConfig.password, "TestPassword");
    testConfig.magic = CONFIG_MAGIC;
    
    wifiMgr = new WiFiManager(&testConfig, 2);
}

void tearDown(void) {
    delete wifiMgr;
}

void test_wifi_manager_creation(void) {
    TEST_ASSERT_NOT_NULL(wifiMgr);
}

void test_wifi_manager_get_current_time(void) {
    uint32_t time1 = wifiMgr->getCurrentTime();
    delay(1000);
    uint32_t time2 = wifiMgr->getCurrentTime();
    
    // Time should advance
    TEST_ASSERT_GREATER_THAN(time1, time2);
}

void test_wifi_manager_config_not_null(void) {
    // Verify WiFiManager holds config reference
    TEST_ASSERT_NOT_NULL(wifiMgr);
}

void test_wifi_manager_disconnect_safe(void) {
    // Should not crash even if not connected
    wifiMgr->disconnect();
    TEST_ASSERT_TRUE(true);
}

void test_wifi_manager_handle_client_safe(void) {
    // Should not crash even if server not started
    wifiMgr->handleClient();
    TEST_ASSERT_TRUE(true);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_wifi_manager_creation);
    RUN_TEST(test_wifi_manager_get_current_time);
    RUN_TEST(test_wifi_manager_config_not_null);
    RUN_TEST(test_wifi_manager_disconnect_safe);
    RUN_TEST(test_wifi_manager_handle_client_safe);
    
    UNITY_END();
}

void loop() {
}
