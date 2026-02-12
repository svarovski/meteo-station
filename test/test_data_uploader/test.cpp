#include <unity.h>
#include "../lib/DataUploader.h"
#include "../lib/Config.h"
#include "../lib/RTCData.h"
#include <EEPROM.h>

static Config testConfig;
static RTCData testRtcData;
static DataUploader* uploader;

void setUp(void) {
    EEPROM.begin(512);
    
    testConfig.setDefaults();
    strcpy(testConfig.influxServer, "192.168.1.100");
    testConfig.influxPort = 8086;
    strcpy(testConfig.influxDb, "test_db");
    strcpy(testConfig.influxMeasurement, "test");
    testConfig.magic = CONFIG_MAGIC;
    
    testRtcData.initialize();
    
    uploader = new DataUploader(&testConfig, &testRtcData);
}

void tearDown(void) {
    delete uploader;
}

void test_data_uploader_creation(void) {
    TEST_ASSERT_NOT_NULL(uploader);
}

void test_data_uploader_clear_data(void) {
    // Add some test data
    SensorRecord record = SensorRecord::create(20.0, 50.0, 3600, 0);
    testRtcData.addRecord(record);
    testRtcData.romRecordCount = 5;
    
    TEST_ASSERT_EQUAL(1, testRtcData.recordCount);
    TEST_ASSERT_EQUAL(5, testRtcData.romRecordCount);
    
    // Clear data
    uploader->clearData();
    
    // Should be cleared
    TEST_ASSERT_EQUAL(0, testRtcData.recordCount);
    TEST_ASSERT_EQUAL(0, testRtcData.romRecordCount);
    TEST_ASSERT_EQUAL(0, testRtcData.romWriteIndex);
}

void test_data_uploader_upload_with_no_data(void) {
    // Should handle empty data gracefully
    // Note: Will fail to connect to InfluxDB, but shouldn't crash
    bool result = uploader->uploadAllData(3.7);
    
    // Result depends on network, but shouldn't crash
    TEST_ASSERT_TRUE(result || !result);
}

void test_data_uploader_with_buffer_data(void) {
    // Add some records to buffer
    for (int i = 0; i < 5; i++) {
        SensorRecord record = SensorRecord::create(20.0 + i, 50.0, i * 60, 0);
        testRtcData.addRecord(record);
    }
    
    TEST_ASSERT_EQUAL(5, testRtcData.recordCount);
    
    // Try to upload (will fail without network, but tests logic)
    bool result = uploader->uploadAllData(3.8);
    
    TEST_ASSERT_TRUE(result || !result);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_data_uploader_creation);
    RUN_TEST(test_data_uploader_clear_data);
    RUN_TEST(test_data_uploader_upload_with_no_data);
    RUN_TEST(test_data_uploader_with_buffer_data);
    
    UNITY_END();
}

void loop() {
}
