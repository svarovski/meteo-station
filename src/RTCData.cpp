#include "RTCData.h"
#include <user_interface.h>

RTCData::RTCData() {
    initialize();
}

void RTCData::initialize() {
    magic = RTC_MAGIC;
    lastSync = 0;
    recordCount = 0;
    romWriteIndex = 0;
    romRecordCount = 0;
    padding = 0;
    memset(buffer, 0, sizeof(buffer));
}

bool RTCData::load() {
    system_rtc_mem_read(0, this, sizeof(RTCData));
    
    if (!isValid()) {
        Serial.println("Initializing RTC data");
        initialize();
        return false;
    }
    
    Serial.printf("RTC data loaded: %d buffered records\n", recordCount);
    return true;
}

void RTCData::save() {
    system_rtc_mem_write(0, this, sizeof(RTCData));
}

bool RTCData::isValid() const {
    return magic == RTC_MAGIC;
}

bool RTCData::addRecord(const SensorRecord& record) {
    if (recordCount >= RTC_BUFFER_SIZE) {
        return false;
    }
    
    buffer[recordCount++] = record;
    return true;
}

bool RTCData::isBufferFull() const {
    return recordCount >= RTC_BUFFER_SIZE;
}

void RTCData::clearBuffer() {
    recordCount = 0;
}

void RTCData::print() const {
    Serial.println("=== RTC Data ===");
    Serial.printf("Records in buffer: %d/%d\n", recordCount, RTC_BUFFER_SIZE);
    Serial.printf("Records in ROM: %d\n", romRecordCount);
    Serial.printf("Last sync: %lu\n", lastSync);
}
