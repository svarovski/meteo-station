#ifndef RTC_DATA_H
#define RTC_DATA_H

#include <Arduino.h>
#include "SensorRecord.h"

#define RTC_MAGIC 0x12345678
#define RTC_BUFFER_SIZE 128

// RTC RAM structure (persists during deep sleep)
class RTCData {
public:
    uint32_t magic;
    uint32_t lastSync;
    uint16_t recordCount;
    uint16_t romWriteIndex;
    uint16_t romRecordCount;
    uint16_t padding;
    SensorRecord buffer[RTC_BUFFER_SIZE];
    
    RTCData();
    
    bool load();
    void save();
    bool isValid() const;
    void initialize();
    
    bool addRecord(const SensorRecord& record);
    bool isBufferFull() const;
    void clearBuffer();
    
    void print() const;
};

#endif
