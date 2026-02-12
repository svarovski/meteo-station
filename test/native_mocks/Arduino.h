#ifndef ARDUINO_H_MOCK
#define ARDUINO_H_MOCK

// Mock Arduino functions for native testing
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

// Basic types
typedef uint8_t byte;
typedef bool boolean;

// String class mock
class String {
private:
    char* buffer;
    size_t len;
    
public:
    String() : buffer(nullptr), len(0) {}
    String(const char* str) {
        len = strlen(str);
        buffer = (char*)malloc(len + 1);
        strcpy(buffer, str);
    }
    String(int val) {
        buffer = (char*)malloc(32);
        snprintf(buffer, 32, "%d", val);
        len = strlen(buffer);
    }
    String(float val, int decimals = 2) {
        buffer = (char*)malloc(32);
        snprintf(buffer, 32, "%.*f", decimals, val);
        len = strlen(buffer);
    }
    ~String() { if (buffer) free(buffer); }
    
    const char* c_str() const { return buffer ? buffer : ""; }
    size_t length() const { return len; }
    
    int indexOf(const char* str) const {
        if (!buffer) return -1;
        const char* pos = strstr(buffer, str);
        return pos ? (pos - buffer) : -1;
    }
    
    void replace(const char* find, const char* replace) {
        // Simplified replace
    }
    
    String& operator+=(const char* str) {
        size_t newLen = len + strlen(str);
        char* newBuf = (char*)malloc(newLen + 1);
        if (buffer) strcpy(newBuf, buffer);
        strcat(newBuf, str);
        if (buffer) free(buffer);
        buffer = newBuf;
        len = newLen;
        return *this;
    }
};

// Serial mock
class SerialMock {
public:
    void begin(int baud) {}
    void print(const char* str) { printf("%s", str); }
    void println(const char* str) { printf("%s\n", str); }
    void println() { printf("\n"); }
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
    void flush() {}
};

extern SerialMock Serial;

// Arduino functions
inline void delay(unsigned long ms) {}
inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }

// Math
inline long constrain(long x, long a, long b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

// Pin modes (dummy)
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0

inline void pinMode(uint8_t pin, uint8_t mode) {}
inline void digitalWrite(uint8_t pin, uint8_t val) {}
inline int digitalRead(uint8_t pin) { return LOW; }
inline int analogRead(uint8_t pin) { return 0; }

#endif
