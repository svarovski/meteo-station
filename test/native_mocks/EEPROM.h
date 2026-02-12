#ifndef EEPROM_H_MOCK
#define EEPROM_H_MOCK

#include <stdint.h>
#include <string.h>

class EEPROMMock {
private:
    uint8_t data[4096];
    
public:
    EEPROMMock() { memset(data, 0, sizeof(data)); }
    
    void begin(size_t size) {}
    
    template<typename T>
    T& get(int address, T& t) {
        memcpy(&t, &data[address], sizeof(T));
        return t;
    }
    
    template<typename T>
    const T& put(int address, const T& t) {
        memcpy(&data[address], &t, sizeof(T));
        return t;
    }
    
    void commit() {}
    
    uint8_t read(int address) { return data[address]; }
    void write(int address, uint8_t value) { data[address] = value; }
};

extern EEPROMMock EEPROM;

#endif
