#include <unity.h>
#include "../src/RTCData.h"
#include "../src/SensorRecord.h"

static RTCData testRtcData;

void setUp(void) {
    // Reset RTC data before each test
    testRtcData.initialize();
}

void tearDown(void) {
    // Cleanup
}

void test_rtc_data_initialization(void) {
    RTCData rtc;
    
    TEST_ASSERT_EQUAL(RTC_MAGIC, rtc.magic);
    TEST_ASSERT_EQUAL(0, rtc.lastSync);
    TEST_ASSERT_EQUAL(0, rtc.recordCount);
    TEST_ASSERT_EQUAL(0, rtc.romWriteIndex);
    TEST_ASSERT_EQUAL(0, rtc.romRecordCount);
}

void test_rtc_data_is_valid(void) {
    testRtcData.initialize();
    TEST_ASSERT_TRUE(testRtcData.isValid());
    
    // Corrupt magic
    testRtcData.magic = 0xDEADBEEF;
    TEST_ASSERT_FALSE(testRtcData.isValid());
}

void test_rtc_data_add_record(void) {
    SensorRecord record = SensorRecord::create(22.5, 65.0, 3600, 0);
    
    bool added = testRtcData.addRecord(record);
    
    TEST_ASSERT_TRUE(added);
    TEST_ASSERT_EQUAL(1, testRtcData.recordCount);
    
    // Verify record was stored correctly
    TEST_ASSERT_EQUAL(record.timestamp, testRtcData.buffer[0].timestamp);
    TEST_ASSERT_EQUAL(record.temperature, testRtcData.buffer[0].temperature);
    TEST_ASSERT_EQUAL(record.humidity, testRtcData.buffer[0].humidity);
}

void test_rtc_data_add_multiple_records(void) {
    for (int i = 0; i < 10; i++) {
        SensorRecord record = SensorRecord::create(20.0 + i, 50.0 + i, i * 60, 0);
        bool added = testRtcData.addRecord(record);
        TEST_ASSERT_TRUE(added);
    }
    
    TEST_ASSERT_EQUAL(10, testRtcData.recordCount);
}

void test_rtc_data_buffer_full(void) {
    // Initially not full
    TEST_ASSERT_FALSE(testRtcData.isBufferFull());
    
    // Fill buffer
    for (int i = 0; i < RTC_BUFFER_SIZE; i++) {
        SensorRecord record = SensorRecord::create(20.0, 50.0, i * 60, 0);
        testRtcData.addRecord(record);
    }
    
    // Now should be full
    TEST_ASSERT_TRUE(testRtcData.isBufferFull());
    TEST_ASSERT_EQUAL(RTC_BUFFER_SIZE, testRtcData.recordCount);
    
    // Try to add one more
    SensorRecord extraRecord = SensorRecord::create(25.0, 60.0, 9999, 0);
    bool added = testRtcData.addRecord(extraRecord);
    
    TEST_ASSERT_FALSE(added); // Should fail when full
    TEST_ASSERT_EQUAL(RTC_BUFFER_SIZE, testRtcData.recordCount); // Count unchanged
}

void test_rtc_data_clear_buffer(void) {
    // Add some records
    for (int i = 0; i < 5; i++) {
        SensorRecord record = SensorRecord::create(20.0, 50.0, i * 60, 0);
        testRtcData.addRecord(record);
    }
    
    TEST_ASSERT_EQUAL(5, testRtcData.recordCount);
    
    // Clear buffer
    testRtcData.clearBuffer();
    
    TEST_ASSERT_EQUAL(0, testRtcData.recordCount);
    TEST_ASSERT_FALSE(testRtcData.isBufferFull());
}

void test_rtc_data_save_and_load(void) {
    // Add test data
    testRtcData.lastSync = 1234567890;
    testRtcData.romWriteIndex = 10;
    testRtcData.romRecordCount = 50;
    
    SensorRecord record = SensorRecord::create(23.5, 70.0, 7200, 0);
    testRtcData.addRecord(record);
    
    // Save to RTC memory
    testRtcData.save();
    
    // Create new instance and load
    RTCData loadedRtc;
    bool loaded = loadedRtc.load();
    
    TEST_ASSERT_TRUE(loaded);
    TEST_ASSERT_EQUAL(1234567890, loadedRtc.lastSync);
    TEST_ASSERT_EQUAL(10, loadedRtc.romWriteIndex);
    TEST_ASSERT_EQUAL(50, loadedRtc.romRecordCount);
    TEST_ASSERT_EQUAL(1, loadedRtc.recordCount);
    
    // Verify record data
    TEST_ASSERT_EQUAL(record.timestamp, loadedRtc.buffer[0].timestamp);
    TEST_ASSERT_EQUAL(record.temperature, loadedRtc.buffer[0].temperature);
    TEST_ASSERT_EQUAL(record.humidity, loadedRtc.buffer[0].humidity);
}

void test_rtc_data_load_invalid(void) {
    RTCData rtc;
    
    // Manually corrupt RTC memory by writing invalid magic
    rtc.magic = 0x00000000;
    rtc.save();
    
    // Try to load
    RTCData loadedRtc;
    bool loaded = loadedRtc.load();
    
    // Should fail and initialize with defaults
    TEST_ASSERT_FALSE(loaded);
    TEST_ASSERT_EQUAL(RTC_MAGIC, loadedRtc.magic);
    TEST_ASSERT_EQUAL(0, loadedRtc.recordCount);
}

void test_rtc_data_rom_indices(void) {
    testRtcData.romWriteIndex = 25;
    testRtcData.romRecordCount = 100;
    
    TEST_ASSERT_EQUAL(25, testRtcData.romWriteIndex);
    TEST_ASSERT_EQUAL(100, testRtcData.romRecordCount);
}

void test_rtc_data_buffer_size_constant(void) {
    // Verify buffer size matches constant
    TEST_ASSERT_EQUAL(128, RTC_BUFFER_SIZE);
    
    // Verify buffer can hold that many records
    SensorRecord testBuffer[RTC_BUFFER_SIZE];
    TEST_ASSERT_EQUAL(sizeof(testRtcData.buffer), sizeof(testBuffer));
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_rtc_data_initialization);
    RUN_TEST(test_rtc_data_is_valid);
    RUN_TEST(test_rtc_data_add_record);
    RUN_TEST(test_rtc_data_add_multiple_records);
    RUN_TEST(test_rtc_data_buffer_full);
    RUN_TEST(test_rtc_data_clear_buffer);
    RUN_TEST(test_rtc_data_save_and_load);
    RUN_TEST(test_rtc_data_load_invalid);
    RUN_TEST(test_rtc_data_rom_indices);
    RUN_TEST(test_rtc_data_buffer_size_constant);
    
    UNITY_END();
}

void loop() {
    // Nothing to do
}
