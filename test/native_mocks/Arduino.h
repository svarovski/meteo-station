#ifndef ARDUINO_H_MOCK
#define ARDUINO_H_MOCK

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>

// Basic types
typedef uint8_t byte;
typedef bool boolean;

// String class mock - minimal implementation for testing
class String {
private:
    char* buffer;
    size_t len;
    
public:
    String() : buffer(nullptr), len(0) {
        buffer = (char*)malloc(1);
        buffer[0] = '\0';
    }
    
    String(const char* str) {
        len = str ? strlen(str) : 0;
        buffer = (char*)malloc(len + 1);
        if (str) strcpy(buffer, str);
        else buffer[0] = '\0';
    }
    
    String(int val) {
        buffer = (char*)malloc(32);
        snprintf(buffer, 32, "%d", val);
        len = strlen(buffer);
    }
    
    String(unsigned int val) {
        buffer = (char*)malloc(32);
        snprintf(buffer, 32, "%u", val);
        len = strlen(buffer);
    }
    
    String(long val) {
        buffer = (char*)malloc(32);
        snprintf(buffer, 32, "%ld", val);
        len = strlen(buffer);
    }
    
    String(unsigned long val) {
        buffer = (char*)malloc(32);
        snprintf(buffer, 32, "%lu", val);
        len = strlen(buffer);
    }
    
    String(float val, int decimals = 2) {
        buffer = (char*)malloc(32);
        snprintf(buffer, 32, "%.*f", decimals, val);
        len = strlen(buffer);
    }
    
    String(const String& other) {
        len = other.len;
        buffer = (char*)malloc(len + 1);
        strcpy(buffer, other.buffer);
    }
    
    ~String() { 
        if (buffer) free(buffer); 
    }
    
    String& operator=(const String& other) {
        if (this != &other) {
            if (buffer) free(buffer);
            len = other.len;
            buffer = (char*)malloc(len + 1);
            strcpy(buffer, other.buffer);
        }
        return *this;
    }
    
    const char* c_str() const { return buffer ? buffer : ""; }
    size_t length() const { return len; }
    
    int indexOf(const char* str) const {
        if (!buffer) return -1;
        const char* pos = strstr(buffer, str);
        return pos ? (pos - buffer) : -1;
    }
    
    void replace(const char* find, const char* replace) {
        // Not needed for tests
    }
    
    String& operator+=(const String& other) {
        size_t newLen = len + other.len;
        char* newBuf = (char*)malloc(newLen + 1);
        strcpy(newBuf, buffer);
        strcat(newBuf, other.buffer);
        free(buffer);
        buffer = newBuf;
        len = newLen;
        return *this;
    }
    
    String& operator+=(const char* str) {
        if (!str) return *this;
        size_t newLen = len + strlen(str);
        char* newBuf = (char*)malloc(newLen + 1);
        strcpy(newBuf, buffer);
        strcat(newBuf, str);
        free(buffer);
        buffer = newBuf;
        len = newLen;
        return *this;
    }
    
    friend String operator+(const String& lhs, const String& rhs) {
        String result(lhs);
        result += rhs;
        return result;
    }
    
    friend String operator+(const String& lhs, const char* rhs) {
        String result(lhs);
        result += rhs;
        return result;
    }
    
    friend String operator+(const char* lhs, const String& rhs) {
        String result(lhs);
        result += rhs;
        return result;
    }
};

// Serial mock
class SerialMock {
public:
    void begin(int baud) {}
    void print(const char* str) {}
    void println(const char* str) {}
    void println() {}
    void printf(const char* format, ...) {}
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

// Pin modes
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
