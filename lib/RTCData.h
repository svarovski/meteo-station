#ifndef RTC_DATA_H
#define RTC_DATA_H

#ifdef NATIVE
#include "../test/native_mocks/Arduino.h"
#else
#include <Arduino.h>
#endif

#include "SensorRecord.h"

#define RTC_BUFFER_SIZE 128
#define RTC_MAGIC 0x5A5A5A5A

class RTCData {
public:
    uint32_t magic;
    uint16_t recordCount;
    uint16_t romWriteIndex;
    uint16_t romRecordCount;
    uint32_t lastSync;
    SensorRecord buffer[RTC_BUFFER_SIZE];
    
    RTCData();
    
    void initialize();
    bool isValid() const;
    void save();
    bool load();  // Changed to return bool
    
    bool addRecord(const SensorRecord& record);  // Changed to return bool
    bool isBufferFull() const;
    void clearBuffer();
};

#endif
