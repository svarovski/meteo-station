#include "RTCData.h"

#ifndef NATIVE
extern "C" {
#include "user_interface.h"
}
#endif

RTCData::RTCData() {
    initialize();
}

void RTCData::initialize() {
    magic = RTC_MAGIC;
    recordCount = 0;
    romWriteIndex = 0;
    romRecordCount = 0;
    lastSync = 0;
    memset(buffer, 0, sizeof(buffer));
}

bool RTCData::isValid() const {
    return magic == RTC_MAGIC;
}

void RTCData::save() {
#ifdef NATIVE
    // In native mode, data persists in memory
#else
    system_rtc_mem_write(64, this, sizeof(RTCData));
#endif
}

bool RTCData::load() {
#ifdef NATIVE
    // In native mode, use constructor initialization
    if (!isValid()) {
        initialize();
    }
    return isValid();
#else
    system_rtc_mem_read(64, this, sizeof(RTCData));
    if (!isValid()) {
        Serial.println("RTC data invalid, initializing...");
        initialize();
        save();
        return false;
    }
    return true;
#endif
}

bool RTCData::addRecord(const SensorRecord& record) {
    if (recordCount < RTC_BUFFER_SIZE) {
        buffer[recordCount] = record;
        recordCount++;
        return true;
    }
    return false;
}

bool RTCData::isBufferFull() const {
    return recordCount >= RTC_BUFFER_SIZE;
}

void RTCData::clearBuffer() {
    recordCount = 0;
    memset(buffer, 0, sizeof(buffer));
}
